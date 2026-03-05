#include <TlHelp32.h>
#include <Windows.h>


#include "../security/peb_stealth.h"
#include "injector.h"
#include <iostream>
#include <string>

namespace Injector {

static std::string g_lastError;

// =============================================================================
//  Shellcode — runs INSIDE the target process (no CRT, no globals)
// =============================================================================
#pragma runtime_checks("", off)
#pragma optimize("", off)

DWORD __stdcall ShellcodeRun(ManualMapData *pData) {
  if (!pData)
    return 0;

  BYTE *pBase = pData->pBaseAddress;
  auto *pOpt =
      &reinterpret_cast<IMAGE_NT_HEADERS *>(
           pBase +
           reinterpret_cast<IMAGE_DOS_HEADER *>((uintptr_t)pBase)->e_lfanew)
           ->OptionalHeader;

  // ── Relocations ───────────────────────────────────────────────────────────
  auto *pReloc = reinterpret_cast<IMAGE_BASE_RELOCATION *>(
      pBase +
      pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
  BYTE *pLocationDelta = pBase - pOpt->ImageBase;

  if (pLocationDelta &&
      pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
    while (pReloc->VirtualAddress) {
      auto *pDest = reinterpret_cast<WORD *>((uintptr_t)pReloc +
                                             sizeof(IMAGE_BASE_RELOCATION));
      auto numEntries =
          (pReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
      for (DWORD i = 0; i < numEntries; ++i, ++pDest) {
        if (*pDest >> 12 == IMAGE_REL_BASED_DIR64) {
          *reinterpret_cast<ULONG_PTR *>(pBase + pReloc->VirtualAddress +
                                         (*pDest & 0xFFF)) +=
              reinterpret_cast<ULONG_PTR>(pLocationDelta);
        }
      }
      pReloc = reinterpret_cast<IMAGE_BASE_RELOCATION *>(
          reinterpret_cast<uintptr_t>(pReloc) + pReloc->SizeOfBlock);
    }
  }

  // ── Resolve Imports ───────────────────────────────────────────────────────
  auto *pImport = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(
      pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
  if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {

    auto _LoadLibraryA = (HMODULE(__stdcall *)(LPCSTR))pData->pLoadLibraryA;
    auto _GetProcAddress =
        (FARPROC(__stdcall *)(HMODULE, LPCSTR))pData->pGetProcAddress;

    while (pImport->Name) {
      HMODULE hDll =
          _LoadLibraryA(reinterpret_cast<char *>(pBase + pImport->Name));

      auto *pThunk = reinterpret_cast<IMAGE_THUNK_DATA *>(
          pBase + pImport->OriginalFirstThunk);
      auto *pFunc =
          reinterpret_cast<IMAGE_THUNK_DATA *>(pBase + pImport->FirstThunk);

      if (!pThunk)
        pThunk = pFunc;

      for (; pThunk->u1.AddressOfData; ++pThunk, ++pFunc) {
        if (IMAGE_SNAP_BY_ORDINAL(pThunk->u1.Ordinal)) {
          pFunc->u1.Function = (ULONG_PTR)_GetProcAddress(
              hDll, reinterpret_cast<char *>(pThunk->u1.Ordinal & 0xFFFF));
        } else {
          auto *pImportByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(
              pBase + pThunk->u1.AddressOfData);
          pFunc->u1.Function = (ULONG_PTR)_GetProcAddress(
              hDll, reinterpret_cast<char *>(pImportByName->Name));
        }
      }
      ++pImport;
    }
  }

  // ── Exception Table (SEH x64) ────────────────────────────────────────────
  if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size) {
    auto *pFuncTable = reinterpret_cast<IMAGE_RUNTIME_FUNCTION_ENTRY *>(
        pBase +
        pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress);
    auto numFuncs = pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size /
                    sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);

    auto _RtlAddFunctionTable = (BOOLEAN(__stdcall *)(
        PRUNTIME_FUNCTION, DWORD, DWORD64))pData->pRtlAddFunctionTable;

    if (_RtlAddFunctionTable) {
      _RtlAddFunctionTable(pFuncTable, numFuncs,
                           reinterpret_cast<DWORD64>(pBase));
    }
  }

  // ── TLS Callbacks ────────────────────────────────────────────────────────
  if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) {
    auto *pTls = reinterpret_cast<IMAGE_TLS_DIRECTORY *>(
        pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
    auto *pCallbacks =
        reinterpret_cast<PIMAGE_TLS_CALLBACK *>(pTls->AddressOfCallBacks);
    if (pCallbacks) {
      while (*pCallbacks) {
        (*pCallbacks)(reinterpret_cast<PVOID>(pBase), DLL_PROCESS_ATTACH,
                      nullptr);
        ++pCallbacks;
      }
    }
  }

  // ── Call DllMain ──────────────────────────────────────────────────────────
  if (pOpt->AddressOfEntryPoint) {
    auto DllMain = reinterpret_cast<BOOL(__stdcall *)(HMODULE, DWORD, LPVOID)>(
        pBase + pOpt->AddressOfEntryPoint);
    DllMain(reinterpret_cast<HMODULE>(pBase), DLL_PROCESS_ATTACH, nullptr);
  }

  return 0;
}

DWORD __stdcall ShellcodeEnd() { return 0; }

#pragma optimize("", on)
#pragma runtime_checks("", restore)

// =============================================================================
//  Process Utilities
// =============================================================================

DWORD GetProcessIdByName(const std::wstring &processName) {
  PROCESSENTRY32W pe32;
  pe32.dwSize = sizeof(PROCESSENTRY32W);

  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE)
    return 0;

  DWORD pid = 0;
  if (Process32FirstW(hSnapshot, &pe32)) {
    do {
      if (processName == pe32.szExeFile) {
        pid = pe32.th32ProcessID;
        break;
      }
    } while (Process32NextW(hSnapshot, &pe32));
  }

  CloseHandle(hSnapshot);
  return pid;
}

void KillProcessByName(const std::wstring &processName) {
  PROCESSENTRY32W pe32;
  pe32.dwSize = sizeof(PROCESSENTRY32W);
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE)
    return;

  if (Process32FirstW(hSnapshot, &pe32)) {
    do {
      if (processName == pe32.szExeFile) {
        HANDLE hProcess =
            OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
        if (hProcess) {
          TerminateProcess(hProcess, 0);
          CloseHandle(hProcess);
        }
      }
    } while (Process32NextW(hSnapshot, &pe32));
  }
  CloseHandle(hSnapshot);
}

// =============================================================================
//  ManualMap — real manual mapping: maps PE sections, runs shellcode in target
// =============================================================================

bool ManualMap(const std::wstring &processName,
               const std::vector<uint8_t> &dllBytes) {
  g_lastError.clear();

  if (dllBytes.size() < sizeof(IMAGE_DOS_HEADER)) {
    g_lastError = "DLL invalida (muito pequena).";
    return false;
  }

  // ── Validate PE ───────────────────────────────────────────────────────────
  auto *pDos = reinterpret_cast<const IMAGE_DOS_HEADER *>(dllBytes.data());
  if (pDos->e_magic != IMAGE_DOS_SIGNATURE) {
    g_lastError = "DLL invalida (assinatura DOS incorreta).";
    return false;
  }

  auto *pNt = reinterpret_cast<const IMAGE_NT_HEADERS *>(dllBytes.data() +
                                                         pDos->e_lfanew);
  if (pNt->Signature != IMAGE_NT_SIGNATURE) {
    g_lastError = "DLL invalida (assinatura PE incorreta).";
    return false;
  }

#ifdef _WIN64
  if (pNt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
    g_lastError = "DLL nao e x64. O loader e 64-bit.";
    return false;
  }
#else
  if (pNt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
    g_lastError = "DLL nao e x86. O loader e 32-bit.";
    return false;
  }
#endif

  // ── Open target process ───────────────────────────────────────────────────
  DWORD pid = GetProcessIdByName(processName);
  if (pid == 0) {
    g_lastError = "Processo alvo nao encontrado.";
    return false;
  }

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
  if (!hProcess) {
    g_lastError = "OpenProcess falhou.";
    return false;
  }

  auto &optHeader = pNt->OptionalHeader;

  // ── Allocate memory in target for the mapped image ────────────────────────
  BYTE *pTargetBase = reinterpret_cast<BYTE *>(
      VirtualAllocEx(hProcess, nullptr, optHeader.SizeOfImage,
                     MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

  if (!pTargetBase) {
    CloseHandle(hProcess);
    g_lastError = "VirtualAllocEx para imagem falhou.";
    return false;
  }

  // ── Map PE headers ────────────────────────────────────────────────────────
  if (!WriteProcessMemory(hProcess, pTargetBase, dllBytes.data(),
                          optHeader.SizeOfHeaders, nullptr)) {
    VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    g_lastError = "Falha ao escrever headers PE.";
    return false;
  }

  // ── Map PE sections ───────────────────────────────────────────────────────
  auto *pSection = IMAGE_FIRST_SECTION(pNt);
  for (WORD i = 0; i < pNt->FileHeader.NumberOfSections; ++i, ++pSection) {
    if (pSection->SizeOfRawData == 0)
      continue;

    if (!WriteProcessMemory(hProcess, pTargetBase + pSection->VirtualAddress,
                            dllBytes.data() + pSection->PointerToRawData,
                            pSection->SizeOfRawData, nullptr)) {
      VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
      CloseHandle(hProcess);
      g_lastError = "Falha ao mapear secao PE.";
      return false;
    }
  }

  // ── Prepare shellcode data ────────────────────────────────────────────────
  ManualMapData mapData = {};
  mapData.pBaseAddress = pTargetBase;
  mapData.pLoadLibraryA =
      GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
  mapData.pGetProcAddress =
      GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");
  mapData.pRtlAddFunctionTable =
      GetProcAddress(GetModuleHandleA("kernel32.dll"), "RtlAddFunctionTable");

  // ── Allocate + write ManualMapData in target ──────────────────────────────
  BYTE *pDataRemote = reinterpret_cast<BYTE *>(
      VirtualAllocEx(hProcess, nullptr, sizeof(ManualMapData),
                     MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

  if (!pDataRemote) {
    VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    g_lastError = "VirtualAllocEx para dados falhou.";
    return false;
  }

  if (!WriteProcessMemory(hProcess, pDataRemote, &mapData, sizeof(mapData),
                          nullptr)) {
    VirtualFreeEx(hProcess, pDataRemote, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    g_lastError = "WriteProcessMemory para dados falhou.";
    return false;
  }

  // ── Allocate + write shellcode in target ──────────────────────────────────
  SIZE_T shellcodeSize = reinterpret_cast<BYTE *>(ShellcodeEnd) -
                         reinterpret_cast<BYTE *>(ShellcodeRun);

  BYTE *pShellcodeRemote = reinterpret_cast<BYTE *>(
      VirtualAllocEx(hProcess, nullptr, shellcodeSize, MEM_COMMIT | MEM_RESERVE,
                     PAGE_EXECUTE_READWRITE));

  if (!pShellcodeRemote) {
    VirtualFreeEx(hProcess, pDataRemote, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    g_lastError = "VirtualAllocEx para shellcode falhou.";
    return false;
  }

  if (!WriteProcessMemory(hProcess, pShellcodeRemote, ShellcodeRun,
                          shellcodeSize, nullptr)) {
    VirtualFreeEx(hProcess, pShellcodeRemote, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, pDataRemote, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    g_lastError = "WriteProcessMemory para shellcode falhou.";
    return false;
  }

  // ── Execute shellcode in target ───────────────────────────────────────────
  HANDLE hThread = CreateRemoteThread(
      hProcess, nullptr, 0,
      reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellcodeRemote), pDataRemote,
      0, nullptr);

  if (!hThread) {
    VirtualFreeEx(hProcess, pShellcodeRemote, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, pDataRemote, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    g_lastError = "CreateRemoteThread falhou.";
    return false;
  }

  DWORD waitResult = WaitForSingleObject(hThread, 15000);
  CloseHandle(hThread);

  // ── Cleanup shellcode + data allocations (image stays mapped) ─────────────
  VirtualFreeEx(hProcess, pShellcodeRemote, 0, MEM_RELEASE);
  VirtualFreeEx(hProcess, pDataRemote, 0, MEM_RELEASE);
  CloseHandle(hProcess);

  if (waitResult == WAIT_TIMEOUT) {
    g_lastError = "Timeout: shellcode nao retornou em 15s.";
    return false;
  }

  return true;
}

// =============================================================================
//  LoadLibraryInject — classic LoadLibraryW injection (writes DLL to temp file)
// =============================================================================

bool LoadLibraryInject(const std::wstring &processName,
                       const std::vector<uint8_t> &dllBytes) {
  g_lastError.clear();
  if (dllBytes.empty())
    return false;

  DWORD pid = GetProcessIdByName(processName);
  if (pid == 0)
    return g_lastError = "Processo alvo nao encontrado.", false;

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
  if (!hProcess)
    return g_lastError = "OpenProcess falhou.", false;

  wchar_t tempPath[MAX_PATH];
  GetTempPathW(MAX_PATH, tempPath);
  std::wstring dllPath =
      std::wstring(tempPath) + L"brs_" + std::to_wstring(pid) + L".tmp";

  HANDLE hFile = CreateFileW(dllPath.c_str(), GENERIC_WRITE, 0, nullptr,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    CloseHandle(hProcess);
    return g_lastError = "Falha ao criar arquivo temporario para DLL.", false;
  }

  DWORD written = 0;
  WriteFile(hFile, dllBytes.data(), (DWORD)dllBytes.size(), &written, nullptr);
  CloseHandle(hFile);

  SIZE_T allocSize = (dllPath.size() + 1) * sizeof(wchar_t);
  LPVOID addr = VirtualAllocEx(hProcess, nullptr, allocSize,
                               MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!addr) {
    DeleteFileW(dllPath.c_str());
    CloseHandle(hProcess);
    g_lastError = "VirtualAllocEx falhou.";
    return false;
  }

  if (!WriteProcessMemory(hProcess, addr, dllPath.c_str(), allocSize,
                          nullptr)) {
    VirtualFreeEx(hProcess, addr, 0, MEM_RELEASE);
    DeleteFileW(dllPath.c_str());
    CloseHandle(hProcess);
    g_lastError = "WriteProcessMemory falhou.";
    return false;
  }

  HANDLE hThread =
      CreateRemoteThread(hProcess, nullptr, 0,
                         (LPTHREAD_START_ROUTINE)GetProcAddress(
                             GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"),
                         addr, 0, nullptr);

  if (!hThread) {
    VirtualFreeEx(hProcess, addr, 0, MEM_RELEASE);
    DeleteFileW(dllPath.c_str());
    CloseHandle(hProcess);
    g_lastError = "CreateRemoteThread falhou.";
    return false;
  }

  DWORD waitResult = WaitForSingleObject(hThread, 15000);
  DWORD exitCode = 0;
  GetExitCodeThread(hThread, &exitCode);

  VirtualFreeEx(hProcess, addr, 0, MEM_RELEASE);
  CloseHandle(hThread);
  CloseHandle(hProcess);
  DeleteFileW(dllPath.c_str());

  if (waitResult == WAIT_TIMEOUT) {
    g_lastError = "Timeout: LoadLibrary nao retornou em 15s.";
    return false;
  }

  if (exitCode == 0) {
    g_lastError = "LoadLibraryW retornou NULL no processo alvo. "
                  "Verifique se a DLL e compativel (x86/x64).";
    return false;
  }

  return true;
}

const char *GetLastError() { return g_lastError.c_str(); }

} // namespace Injector
