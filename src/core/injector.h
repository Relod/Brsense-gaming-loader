
#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>
#include <vector>


namespace Injector {
struct ManualMappingData {
  HINSTANCE hLibModule;
  FARPROC pLoadLibraryA;
  FARPROC pGetProcAddress;
  FARPROC pRtlAddFunctionTable;
  BYTE *pBaseAddress;
};

DWORD GetProcessIdByName(const std::wstring &processName);

void KillProcessByName(const std::wstring &processName);

bool ManualMap(const std::wstring &processName,
               const std::vector<uint8_t> &dllBytes);

const char *GetLastError();
}
