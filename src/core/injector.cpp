// =============================================================================
// injector.cpp — Implementação C++ do Manual Mapper x64
// =============================================================================

#include <Windows.h>
#include <TlHelp32.h>

#include "injector.h"
#include "../security/peb_stealth.h"
#include <iostream>
#include <string>

namespace Injector {

static std::string g_lastError;

// =========================================================================
// O Shellcode: Rodará dentro da memória do jogo (Remoto)
// Resolve Dependências e CHAMA o DllMain remotamente!
// =========================================================================

// Obs: Esta função precisa ser compilada com parâmetros __stdcall e sem
// checagens de buffer/stack (Security Cookies) pois o compilador de forma
// padrão coloca jmps pra calls fora do escopo (GS flag). Usaremos pragmas pra
// desligar a proteção desta func.
#pragma runtime_checks("", off)
#pragma optimize("", off)
DWORD __stdcall ShellcodeRun(ManualMappingData *pData) {
  if (!pData)
    return 0;

  BYTE *pBase = pData->pBaseAddress;
  auto *pOpt =
      &reinterpret_cast<IMAGE_NT_HEADERS *>(
           pBase +
           reinterpret_cast<IMAGE_DOS_HEADER *>((uintptr_t)pBase)->e_lfanew)
           ->OptionalHeader;

  // 1. Relocar (Base Relocations)
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
      for (auto i = 0; i < numEntries; ++i, ++pDest) {
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

  // 2. Import Address Table (IAT) - Carregar Dependencias do Windows pra Dll
  // Alvo sem pisar em ganchos
  auto *pImport = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(
      pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
  if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {

    auto _LoadLibraryA = (HMODULE(__stdcall *)(LPCSTR))pData->pLoadLibraryA;
    auto _GetProcAddress = (FARPROC(__stdcall *)(HMODULE, LPCSTR))pData->pGetProcAddress;

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

  // 3. Exception Handlers (Se for x64 e tiver try..catch na DLL alvo,
  // precisamos setar na TEB)
  if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size) {
    auto *pFuncTable = reinterpret_cast<IMAGE_RUNTIME_FUNCTION_ENTRY *>(
        pBase +
        pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress);
    auto numFuncs = pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size /
                    sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);

    auto _RtlAddFunctionTable = (BOOLEAN(__stdcall *)(PRUNTIME_FUNCTION, DWORD, DWORD64))pData->pRtlAddFunctionTable;

    if (_RtlAddFunctionTable) {
      _RtlAddFunctionTable(pFuncTable, numFuncs,
                           reinterpret_cast<DWORD64>(pBase));
    }
  }
  // 4. Invocar a magia principal: DLLMain() com razão "DLL_PROCESS_ATTACH"
  if (pOpt->AddressOfEntryPoint) {
    auto DllMain = reinterpret_cast<BOOL(__stdcall *)(HMODULE, DWORD, LPVOID)>(
        pBase + pOpt->AddressOfEntryPoint);
    DllMain(reinterpret_cast<HMODULE>(pBase), DLL_PROCESS_ATTACH, nullptr);
  }

  return 0; // Se finalizou, a dll está viva rodando no outro app
}

// Dummy func apenas para medir a quantia de bytes do shellcode de forma rapida
// em x64.
DWORD __stdcall ShellcodeEnd() { return 0; }

#pragma optimize("", on)
#pragma runtime_checks("", restore)

// =========================================================================

DWORD GetProcessIdByName(const std::wstring &processName) {
  PROCESSENTRY32W pe32;
  pe32.dwSize = sizeof(PROCESSENTRY32W);

  // Tirar fotografia atomica de todos os processos atuais da base (Toolhelp)
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
  if (hSnapshot == INVALID_HANDLE_VALUE) return;

  if (Process32FirstW(hSnapshot, &pe32)) {
    do {
      if (processName == pe32.szExeFile) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
        if (hProcess) {
          TerminateProcess(hProcess, 0);
          CloseHandle(hProcess);
        }
      }
    } while (Process32NextW(hSnapshot, &pe32));
  }
  CloseHandle(hSnapshot);
}

bool ManualMap(const std::wstring &processName,
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

  // Usa diretório temporario para despejar a payload e e injetar (imitando o conceito do cs2Injector de nao mapear manualmente)
  wchar_t tempPath[MAX_PATH];
  GetTempPathW(MAX_PATH, tempPath);
  std::wstring dllPath = std::wstring(tempPath) + L"cs2_payload_imgui.dll";

  HANDLE hFile = CreateFileW(dllPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    CloseHandle(hProcess);
    return g_lastError = "Falha ao criar arquivo temporario para DLL.", false;
  }
  
  DWORD written = 0;
  WriteFile(hFile, dllBytes.data(), dllBytes.size(), &written, nullptr);
  CloseHandle(hFile);

  SIZE_T allocSize = (dllPath.size() + 1) * sizeof(wchar_t);
  LPVOID addr = VirtualAllocEx(hProcess, nullptr, allocSize, MEM_COMMIT, PAGE_READWRITE);
  if (!addr) {
    DeleteFileW(dllPath.c_str());
    CloseHandle(hProcess);
    return g_lastError = "VirtualAllocEx falhou.", false;
  }

  WriteProcessMemory(hProcess, addr, dllPath.c_str(), allocSize, nullptr);

  HANDLE hThread = CreateRemoteThread(
      hProcess, nullptr, 0,
      (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"),
      addr, 0, nullptr);

  if (!hThread) {
    VirtualFreeEx(hProcess, addr, 0, MEM_RELEASE);
    DeleteFileW(dllPath.c_str());
    CloseHandle(hProcess);
    return g_lastError = "CreateRemoteThread falhou.", false;
  }

  WaitForSingleObject(hThread, INFINITE);

  VirtualFreeEx(hProcess, addr, 0, MEM_RELEASE);
  CloseHandle(hThread);
  CloseHandle(hProcess);

  // Deletar o rastro do disco limpa a DLL pós-injeçao
  DeleteFileW(dllPath.c_str());

  return true;
}

const char *GetLastError() { return g_lastError.c_str(); }

} // namespace Injector
