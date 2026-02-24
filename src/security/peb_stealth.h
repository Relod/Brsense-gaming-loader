// =============================================================================
// peb_stealth.h — Resolução Dinâmica de APIs em Arquitetura x64
// =============================================================================
// Objetivo: Evitar a IAT explícita. Nós vasculhamos as tabelas Ldr (Load
// Order) na memória do PEB (Process Environment Block) gerada pelo kernel.
// Isso cega scanners baseados em detecção de Hooks na kernel32.dll

#pragma once

#include <intrin.h>
#include <windows.h>


// Definindo as estruturas internas vitais não documentadas que a Microsoft usa
// no Windows x64. Normalmente elas residem no <winternl.h>, mas as vezes podem
// ter conflitos.
typedef struct _UNICODE_STRING_CUSTOM {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING_CUSTOM, *PUNICODE_STRING_CUSTOM;

// Tabela Ldr que guarda os modulos da memoria.
typedef struct _LDR_DATA_TABLE_ENTRY_CUSTOM {
  LIST_ENTRY InLoadOrderLinks;
  LIST_ENTRY InMemoryOrderLinks;
  LIST_ENTRY InInitializationOrderLinks;
  PVOID DllBase;
  PVOID EntryPoint;
  PVOID SizeOfImage;
  UNICODE_STRING_CUSTOM FullDllName;
  UNICODE_STRING_CUSTOM BaseDllName;
} LDR_DATA_TABLE_ENTRY_CUSTOM, *PLDR_DATA_TABLE_ENTRY_CUSTOM;

typedef struct _PEB_LDR_DATA_CUSTOM {
  ULONG Length;
  UCHAR Initialized;
  PVOID SsHandle;
  LIST_ENTRY InLoadOrderModuleList;
  LIST_ENTRY InMemoryOrderModuleList;
  LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA_CUSTOM, *PPEB_LDR_DATA_CUSTOM;

typedef struct _PEB_CUSTOM {
  UCHAR InheritedAddressSpace;
  UCHAR ReadImageFileExecOptions;
  UCHAR BeingDebugged;
  UCHAR BitField;
  PVOID Mutant;
  PVOID ImageBaseAddress;
  PPEB_LDR_DATA_CUSTOM Ldr;
} PEB_CUSTOM, *PPEB_CUSTOM;

namespace Stealth {

// Helper macro pra deixar os loops em x64 rapidos em memory reads
#pragma intrinsic(__readgsqword)

/// <summary>
/// Faz o traverse manual pela PEB (em 64 bits o segmento é GS no offset 0x60).
/// </summary>
static inline HMODULE GetModuleHandleCustom(const wchar_t *moduleName) {
  // Encontra o bloco do processo atual lendo o segment register puro (Furtivo)
  PPEB_CUSTOM pPEB = (PPEB_CUSTOM)__readgsqword(0x60);
  PPEB_LDR_DATA_CUSTOM pLdr = pPEB->Ldr;
  PLIST_ENTRY pListHead = &pLdr->InLoadOrderModuleList;
  PLIST_ENTRY pCurrent = pListHead->Flink;

  while (pCurrent != pListHead) {
    PLDR_DATA_TABLE_ENTRY_CUSTOM pEntry = CONTAINING_RECORD(
        pCurrent, LDR_DATA_TABLE_ENTRY_CUSTOM, InLoadOrderLinks);

    if (pEntry->BaseDllName.Buffer) {
      // Simplificação extrema da conversão wcscmp in-memory sem chamar crt
      // calls reais
      size_t i = 0;
      bool match = true;
      while (moduleName[i] != L'\0' && pEntry->BaseDllName.Buffer[i] != L'\0') {
        // C++ tolower naive para wchars (Basta olhar ascii difference)
        wchar_t c1 = moduleName[i];
        wchar_t c2 = pEntry->BaseDllName.Buffer[i];
        if (c1 >= L'A' && c1 <= L'Z')
          c1 += 32;
        if (c2 >= L'A' && c2 <= L'Z')
          c2 += 32;

        if (c1 != c2) {
          match = false;
          break;
        }
        i++;
      }

      if (match && moduleName[i] == L'\0' &&
          pEntry->BaseDllName.Buffer[i] == L'\0') {
        return (HMODULE)pEntry->DllBase;
      }
    }
    pCurrent = pCurrent->Flink;
  }

  return nullptr;
}

/// <summary>
/// Escaneia os bytes de uma HMODULE exportando sua NT IMAGE_EXPORT_DIRECTORY
/// </summary>
static inline FARPROC GetProcAddressCustom(HMODULE hModule,
                                           const char *procName) {
  if (!hModule)
    return nullptr;

  BYTE *pBaseAddress = (BYTE *)hModule;
  IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)pBaseAddress;
  if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    return nullptr;

  IMAGE_NT_HEADERS *pNtHeaders =
      (IMAGE_NT_HEADERS *)(pBaseAddress + pDosHeader->e_lfanew);
  IMAGE_EXPORT_DIRECTORY *pExportDir =
      (IMAGE_EXPORT_DIRECTORY
           *)(pBaseAddress + pNtHeaders->OptionalHeader
                                 .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
                                 .VirtualAddress);

  DWORD *pAddressOfFunctions =
      (DWORD *)(pBaseAddress + pExportDir->AddressOfFunctions);
  DWORD *pAddressOfNames = (DWORD *)(pBaseAddress + pExportDir->AddressOfNames);
  WORD *pAddressOfNameOrdinals =
      (WORD *)(pBaseAddress + pExportDir->AddressOfNameOrdinals);

  // Verifica se estão passando o endereco no modo numérico (Ordinal) em vez do
  // nome da func
  if (((uintptr_t)procName >> 16) == 0) {
    WORD ordinal = LOWORD(procName) - (WORD)pExportDir->Base;
    if (ordinal < pExportDir->NumberOfFunctions) {
      return (FARPROC)(pBaseAddress + pAddressOfFunctions[ordinal]);
    }
    return nullptr;
  }

  for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
    char *pFunctionName = (char *)(pBaseAddress + pAddressOfNames[i]);

    // C-String comparer puro
    size_t j = 0;
    bool match = true;
    while (procName[j] != '\0' && pFunctionName[j] != '\0') {
      if (procName[j] != pFunctionName[j]) {
        match = false;
        break;
      }
      j++;
    }

    if (match && procName[j] == '\0' && pFunctionName[j] == '\0') {
      WORD entryIndex = pAddressOfNameOrdinals[i];
      return (FARPROC)(pBaseAddress + pAddressOfFunctions[entryIndex]);
    }
  }

  return nullptr;
}

} // namespace Stealth
