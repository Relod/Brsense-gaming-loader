
#include "anti_debug.h"
#include <chrono>
#include <intrin.h>
#include <thread>
#include <windows.h>
#include <winternl.h>

namespace Security {

void *GetModuleBaseAddress() {
#ifdef _WIN64
  PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
  PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif
  return pPeb->Reserved3[1];
}

void ErasePEHeader() {
  DWORD oldProtect;
  char *baseAddress = (char *)GetModuleBaseAddress();

  VirtualProtect(baseAddress, 4096, PAGE_EXECUTE_READWRITE, &oldProtect);

  ZeroMemory(baseAddress, 4096);

  VirtualProtect(baseAddress, 4096, oldProtect, &oldProtect);
}

bool CheckForDebugger() {
  bool isDebugged = false;

  if (IsDebuggerPresent())
    return true;

  CheckRemoteDebuggerPresent(GetCurrentProcess(), (PBOOL)&isDebugged);
  if (isDebugged)
    return true;

#ifdef _WIN64
  PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
  PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif

  if (pPeb->BeingDebugged == 1)
    return true;

  return false;
}

void SecurityLoop() {
  int consecutiveDetections = 0;
  while (true) {
    if (CheckForDebugger()) {
      consecutiveDetections++;
      // Require 3 consecutive detections to avoid false positives
      if (consecutiveDetections >= 3) {
        exit(1);
      }
    } else {
      consecutiveDetections = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }
}

void StartupSecurity() {
  // NOTE: ErasePEHeader() is called separately AFTER full
  // initialization to avoid breaking DLL loading or exception handling.
  if (CheckForDebugger()) {
    exit(1);
  }

  std::thread(SecurityLoop).detach();
}
} // namespace Security
