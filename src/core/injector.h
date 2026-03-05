
#pragma once

#include <TlHelp32.h>
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>


namespace Injector {

// Data passed to the shellcode running inside the target process
struct ManualMapData {
  BYTE *pBaseAddress;
  HINSTANCE hLibModule;
  FARPROC pLoadLibraryA;
  FARPROC pGetProcAddress;
  FARPROC pRtlAddFunctionTable;
};

// Process utilities
DWORD GetProcessIdByName(const std::wstring &processName);
void KillProcessByName(const std::wstring &processName);

// Injection methods
bool ManualMap(const std::wstring &processName,
               const std::vector<uint8_t> &dllBytes);

bool LoadLibraryInject(const std::wstring &processName,
                       const std::vector<uint8_t> &dllBytes);

const char *GetLastError();

} // namespace Injector
