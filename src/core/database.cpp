
#include "database.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <wincrypt.h>
#include <windows.h>
#include <winhttp.h>


#include "../security/xorstr.hpp"
#include <cstdio>
#include <cstring>
#include <nlohmann/json.hpp>


#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

using json = nlohmann::json;

// Safe JSON accessors — never throw
static std::string jstr(const json &j, const char *key) {
  if (j.contains(key) && j[key].is_string())
    return j[key].get<std::string>();
  if (j.contains(key) && j[key].is_number())
    return std::to_string(j[key].get<int>());
  return "";
}

static bool jbool(const json &j, const char *key) {
  if (j.contains(key) && j[key].is_boolean())
    return j[key].get<bool>();
  return false;
}

static int jint(const json &j, const char *key) {
  if (j.contains(key) && j[key].is_number())
    return j[key].get<int>();
  return 0;
}

static json safe_parse(const std::string &s) {
  try {
    return json::parse(s);
  } catch (...) {
    return json();
  }
}

static std::string Sha256(const std::vector<uint8_t> &data) {
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  BYTE rgbHash[32];
  DWORD cbHash = 32;
  if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES,
                           CRYPT_VERIFYCONTEXT))
    return "";
  if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
    CryptReleaseContext(hProv, 0);
    return "";
  }
  CryptHashData(hHash, data.data(), (DWORD)data.size(), 0);
  CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);
  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);

  static const char hex[] = "0123456789abcdef";
  std::string out;
  out.reserve(64);
  for (DWORD i = 0; i < cbHash; ++i) {
    out.push_back(hex[(rgbHash[i] >> 4) & 0xF]);
    out.push_back(hex[rgbHash[i] & 0xF]);
  }
  return out;
}

// Helper: parse CheatLicense array from json
static void ParseCheats(const json &j, std::vector<CheatLicense> &out) {
  out.clear();
  if (!j.contains("cheats") || !j["cheats"].is_array())
    return;

  for (const auto &item : j["cheats"]) {
    CheatLicense cl;
    cl.id = jint(item, "id");
    cl.game = jstr(item, "game");
    cl.name = jstr(item, "name");
    cl.iconColor = jstr(item, "icon_color");
    cl.process = jstr(item, "process");
    cl.injectionMethod = jstr(item, "injection_method");
    cl.killProcesses = jstr(item, "kill_processes");
    cl.launchParams = jstr(item, "launch_params");
    cl.steamAppId = jstr(item, "steam_app_id");
    cl.requiresAdmin = jbool(item, "requires_admin");
    cl.enabled = true;
    cl.hash = jstr(item, "hash");
    cl.notes = jstr(item, "notes");
    cl.timeLeft = jstr(item, "time_left");
    cl.status = jstr(item, "status");
    out.push_back(cl);
  }
}

Database::Database() {}
Database::~Database() { Disconnect(); }

std::string Database::HttpRequest(const std::string &method,
                                  const std::string &path,
                                  const std::string &body, int authType) {
  std::string result;

  int wLen = MultiByteToWideChar(CP_UTF8, 0, m_host.c_str(), -1, nullptr, 0);
  std::wstring wHost(wLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, m_host.c_str(), -1, &wHost[0], wLen);

  int wpLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
  std::wstring wPath(wpLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wPath[0], wpLen);

  int wmLen = MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, nullptr, 0);
  std::wstring wMethod(wmLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, &wMethod[0], wmLen);

  std::string ua = XOR("BRSense/1.0");
  int wUaLen = MultiByteToWideChar(CP_UTF8, 0, ua.c_str(), -1, nullptr, 0);
  std::wstring wUa(wUaLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, ua.c_str(), -1, &wUa[0], wUaLen);

  HINTERNET hSession =
      WinHttpOpen(wUa.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession)
    return "";

  HINTERNET hConnect =
      WinHttpConnect(hSession, wHost.c_str(), (INTERNET_PORT)m_port, 0);
  if (!hConnect) {
    WinHttpCloseHandle(hSession);
    return "";
  }

  DWORD httpFlags = m_useHttps ? WINHTTP_FLAG_SECURE : 0;

  HINTERNET hRequest = WinHttpOpenRequest(
      hConnect, wMethod.c_str(), wPath.c_str(), nullptr, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, httpFlags);
  if (!hRequest) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return "";
  }

  if (method == "POST" && !body.empty()) {
    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json\r\n",
                             (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
  }

  if (authType == 1 && !m_jwtToken.empty()) {
    std::string authHeaderStr = "Authorization: Bearer " + m_jwtToken + "\r\n";
    int wahLen =
        MultiByteToWideChar(CP_UTF8, 0, authHeaderStr.c_str(), -1, nullptr, 0);
    std::wstring wAuthHeader(wahLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, authHeaderStr.c_str(), -1, &wAuthHeader[0],
                        wahLen);
    WinHttpAddRequestHeaders(hRequest, wAuthHeader.c_str(), (DWORD)-1,
                             WINHTTP_ADDREQ_FLAG_ADD);
  }

  BOOL bResult =
      WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                         (LPVOID)(body.empty() ? nullptr : body.c_str()),
                         (DWORD)body.size(), (DWORD)body.size(), 0);

  if (bResult) {
    bResult = WinHttpReceiveResponse(hRequest, nullptr);
  }

  if (bResult) {
    DWORD bytesAvailable = 0;
    do {
      bytesAvailable = 0;
      if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
        break;
      if (bytesAvailable == 0)
        break;

      char *buf = new char[bytesAvailable + 1];
      DWORD bytesRead = 0;
      if (WinHttpReadData(hRequest, buf, bytesAvailable, &bytesRead)) {
        buf[bytesRead] = '\0';
        result.append(buf, bytesRead);
      }
      delete[] buf;
    } while (bytesAvailable > 0);
  }

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  return result;
}

bool Database::Connect(const std::string &host, unsigned int port) {
  m_host = host;
  m_port = port;
  m_useHttps = (port == 443);

  std::string getStr = XOR("GET");
  std::string statusStr = XOR("/api/status");

  std::string resp = HttpRequest(getStr, statusStr);
  if (resp.empty()) {
    std::string errStr = XOR("Servidor nao acessivel.");
    m_lastError = errStr;
    m_connected = false;
    return false;
  }

  auto j = safe_parse(resp);
  m_connected = jbool(j, "online");
  if (!m_connected) {
    m_lastError = "Servidor offline.";
  } else {
    m_lastError.clear();
  }
  return m_connected;
}

void Database::Disconnect() {
  m_connected = false;
  m_userInfo = {};
  m_cheats.clear();
}

bool Database::IsConnected() const { return m_connected; }

bool Database::Authenticate(const std::string &username,
                            const std::string &password,
                            const HardwareInfo &hwInfo) {
  json bodyJ = {{"username", username},
                {"password", password},
                {"hwid", hwInfo.hwid},
                {"mac", hwInfo.mac},
                {"ip", hwInfo.ip}};

  std::string postStr = XOR("POST");
  std::string loginStr = XOR("/api/login");

  std::string resp = HttpRequest(postStr, loginStr, bodyJ.dump());
  if (resp.empty()) {
    std::string errStr = XOR("Falha na conexao com o servidor.");
    m_lastError = errStr;
    return false;
  }

  auto j = safe_parse(resp);
  if (!jbool(j, "success")) {
    m_lastError = jstr(j, "error");
    if (m_lastError.empty()) {
      std::string unkStr = XOR("Erro desconhecido.");
      m_lastError = unkStr;
    }
    return false;
  }

  m_jwtToken = jstr(j, "token");

  if (j.contains("user") && j["user"].is_object()) {
    const auto &u = j["user"];
    m_userInfo.nickname = jstr(u, "nickname");
    m_userInfo.plan = jstr(u, "plan");
    m_userInfo.hwid = jstr(u, "hwid");
    m_userInfo.mac = jstr(u, "mac");
    m_userInfo.ip = jstr(u, "ip");
  }

  ParseCheats(j, m_cheats);

  m_lastError.clear();
  return true;
}

ServerStatus Database::GetServerStatus() {
  ServerStatus ss;
  std::string resp = HttpRequest("GET", "/api/status");
  if (resp.empty())
    return ss;

  auto j = safe_parse(resp);
  ss.online = jbool(j, "online");
  ss.version = jstr(j, "version");
  ss.region = jstr(j, "region");
  ss.lastUpdate = jstr(j, "last_update");
  ss.ping = jint(j, "ping");

  return ss;
}

bool Database::RefreshCheats() {
  if (m_jwtToken.empty()) {
    m_lastError = "Nao autenticado.";
    return false;
  }

  std::string resp = HttpRequest("GET", "/api/cheats", "", 1);
  if (resp.empty()) {
    m_lastError = "Falha na conexao com o servidor.";
    return false;
  }

  auto j = safe_parse(resp);
  if (!jbool(j, "success")) {
    m_lastError = jstr(j, "error");
    return false;
  }

  ParseCheats(j, m_cheats);

  m_lastError.clear();
  return true;
}

bool Database::Heartbeat() {
  if (m_jwtToken.empty())
    return false;
  std::string resp = HttpRequest("POST", "/api/heartbeat", "{}", 1);
  if (resp.empty())
    return false;

  auto j = safe_parse(resp);
  if (!jbool(j, "success")) {
    m_lastError = jstr(j, "error");
    return false;
  }
  return true;
}

bool Database::RequestHwidReset(const std::string &reason) {
  if (m_jwtToken.empty()) {
    m_lastError = "Nao autenticado.";
    return false;
  }

  json bodyJ = {{"reason", reason}};
  std::string resp = HttpRequest("POST", "/api/hwid_reset", bodyJ.dump(), 1);

  if (resp.empty()) {
    m_lastError = "Falha na conexao com o servidor.";
    return false;
  }

  auto j = safe_parse(resp);
  if (!jbool(j, "success")) {
    m_lastError = jstr(j, "error");
    return false;
  }
  return true;
}

bool Database::RequestHwidResetByCreds(const std::string &username,
                                       const std::string &password,
                                       const std::string &reason) {
  json bodyJ = {
      {"username", username}, {"password", password}, {"reason", reason}};
  std::string resp =
      HttpRequest("POST", "/api/hwid_reset_by_creds", bodyJ.dump(), 0);

  if (resp.empty()) {
    m_lastError = "Falha na conexao com o servidor.";
    return false;
  }

  auto j = safe_parse(resp);
  if (!jbool(j, "success")) {
    m_lastError = jstr(j, "error");
    return false;
  }
  m_lastError = jstr(j, "message");
  return true;
}

bool Database::HttpDownloadRequest(const std::string &path,
                                   std::vector<uint8_t> &outBuffer) {
  outBuffer.clear();

  int wLen = MultiByteToWideChar(CP_UTF8, 0, m_host.c_str(), -1, nullptr, 0);
  std::wstring wHost(wLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, m_host.c_str(), -1, &wHost[0], wLen);

  int wpLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
  std::wstring wPath(wpLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wPath[0], wpLen);

  HINTERNET hSession =
      WinHttpOpen(L"BRSense/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession)
    return false;

  HINTERNET hConnect =
      WinHttpConnect(hSession, wHost.c_str(), (INTERNET_PORT)m_port, 0);
  if (!hConnect) {
    WinHttpCloseHandle(hSession);
    return false;
  }

  DWORD httpFlags = m_useHttps ? WINHTTP_FLAG_SECURE : 0;

  HINTERNET hRequest = WinHttpOpenRequest(
      hConnect, L"GET", wPath.c_str(), nullptr, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, httpFlags);
  if (!hRequest) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  if (!m_jwtToken.empty()) {
    std::string authHeaderStr = "Authorization: Bearer " + m_jwtToken + "\r\n";
    int wahLen =
        MultiByteToWideChar(CP_UTF8, 0, authHeaderStr.c_str(), -1, nullptr, 0);
    std::wstring wAuthHeader(wahLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, authHeaderStr.c_str(), -1, &wAuthHeader[0],
                        wahLen);
    WinHttpAddRequestHeaders(hRequest, wAuthHeader.c_str(), (DWORD)-1,
                             WINHTTP_ADDREQ_FLAG_ADD);
  }

  BOOL bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

  if (bResult)
    bResult = WinHttpReceiveResponse(hRequest, nullptr);

  if (bResult) {
    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize,
                        WINHTTP_NO_HEADER_INDEX);

    if (dwStatusCode != 200) {
      std::string errorResp;
      DWORD bytesAvailable = 0;
      do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
          break;
        if (bytesAvailable == 0)
          break;
        char *buf = new char[bytesAvailable + 1];
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buf, bytesAvailable, &bytesRead)) {
          buf[bytesRead] = '\0';
          errorResp.append(buf, bytesRead);
        }
        delete[] buf;
      } while (bytesAvailable > 0);

      if (!errorResp.empty()) {
        auto ej = safe_parse(errorResp);
        m_lastError = jstr(ej, "error");
      }
      if (m_lastError.empty())
        m_lastError =
            "Download refused. HTTP Code: " + std::to_string(dwStatusCode);

      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      return false;
    }

    DWORD bytesAvailable = 0;
    do {
      bytesAvailable = 0;
      if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
        break;
      if (bytesAvailable == 0)
        break;

      std::vector<uint8_t> buf(bytesAvailable);
      DWORD bytesRead = 0;
      if (WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead)) {
        outBuffer.insert(outBuffer.end(), buf.begin(), buf.begin() + bytesRead);
      }
    } while (bytesAvailable > 0);
  } else {
    m_lastError = "Falha durante requisicao HTTP de download.";
  }

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  return bResult && !outBuffer.empty();
}

bool Database::DownloadCheat(int cheatId, std::vector<uint8_t> &outBuffer,
                             const std::string &expectedSha256) {
  if (m_jwtToken.empty()) {
    m_lastError = "Nao autenticado.";
    return false;
  }

  std::string path = "/api/download/" + std::to_string(cheatId);
  if (!HttpDownloadRequest(path, outBuffer))
    return false;

  if (!expectedSha256.empty()) {
    std::string got = Sha256(outBuffer);
    if (_stricmp(got.c_str(), expectedSha256.c_str()) != 0) {
      m_lastError = "Hash mismatch on downloaded payload.";
      return false;
    }
  }
  return true;
}
