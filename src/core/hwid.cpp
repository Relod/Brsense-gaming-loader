
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>


#include <cstdio>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#include "hwid.h"


static std::string GenerateHWID() {
  std::string hwid = "UNKNOWN";

  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0,
                    KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
    char guid[256] = {0};
    DWORD size = sizeof(guid);
    if (RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr, (LPBYTE)guid,
                         &size) == ERROR_SUCCESS) {
      hwid = guid;
    }
    RegCloseKey(hKey);
  }

  if (hwid == "UNKNOWN" || hwid.empty()) {
    DWORD volumeSerial = 0;
    GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr,
                          nullptr, 0);

    char computerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD nameLen = sizeof(computerName);
    GetComputerNameA(computerName, &nameLen);

    char buf[128];
    snprintf(buf, sizeof(buf), "%08lX-%s", volumeSerial, computerName);
    hwid = buf;
  }

  return hwid;
}

static std::string GetMacAddress() {
  std::string mac = "00:00:00:00:00:00";

  ULONG outBufLen = 15000;
  std::vector<BYTE> buffer(outBufLen);
  PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES *)buffer.data();

  DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX,
                                        NULL, pAddresses, &outBufLen);

  if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
    buffer.resize(outBufLen);
    pAddresses = (IP_ADAPTER_ADDRESSES *)buffer.data();
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL,
                                    pAddresses, &outBufLen);
  }

  if (dwRetVal == NO_ERROR) {
    PIP_ADAPTER_ADDRESSES pCurr = pAddresses;
    while (pCurr) {
      if (pCurr->OperStatus == IfOperStatusUp &&
          pCurr->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
          pCurr->IfType != IF_TYPE_TUNNEL) {

        if (pCurr->PhysicalAddressLength == 6) {
          char buf[24];
          snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                   pCurr->PhysicalAddress[0], pCurr->PhysicalAddress[1],
                   pCurr->PhysicalAddress[2], pCurr->PhysicalAddress[3],
                   pCurr->PhysicalAddress[4], pCurr->PhysicalAddress[5]);
          mac = buf;
          break;
        }
      }
      pCurr = pCurr->Next;
    }
  }

  return mac;
}

#include <wininet.h>
#pragma comment(lib, "wininet.lib")

static std::string GetPublicIP() {
  std::string ip = "0.0.0.0";

  HINTERNET hInternet =
      InternetOpenA("BRSense/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  if (hInternet) {
    HINTERNET hFile = InternetOpenUrlA(hInternet, "http://api.ipify.org", NULL,
                                       0, INTERNET_FLAG_RELOAD, 0);
    if (hFile) {
      char buffer[128];
      DWORD bytesRead = 0;
      if (InternetReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead) &&
          bytesRead > 0) {
        buffer[bytesRead] = '\0';
        ip = buffer;

        while (!ip.empty() && (ip.back() == '\n' || ip.back() == '\r')) {
          ip.pop_back();
        }
      }
      InternetCloseHandle(hFile);
    }
    InternetCloseHandle(hInternet);
  }

  return ip;
}


HardwareInfo CollectHardwareInfo() {
  HardwareInfo info;

  WSADATA wsaData;
  (void)WSAStartup(MAKEWORD(2, 2), &wsaData);

  info.hwid = GenerateHWID();
  info.mac = GetMacAddress();
  info.ip = GetPublicIP();

  WSACleanup();

  return info;
}
