// =============================================================================
// database.cpp — Implementacao do API Client (WinHTTP)
// =============================================================================
// Usa WinHTTP (nativo do Windows) para comunicar com o servidor backend.
// JSON parsing manual simples sem dependencias externas.
// =============================================================================

#include "database.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#include <winhttp.h>

#include "../security/xorstr.hpp"
#include <cstdio>
#include <cstring>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

// =============================================================================
// HELPERS — JSON Parsing Manual
// =============================================================================

/// Extrai o valor de uma chave string em JSON (ex: "key":"value")
static std::string JsonGetString(const std::string &json,
                                 const std::string &key) {
  std::string search = "\"" + key + "\"";
  size_t pos = json.find(search);
  if (pos == std::string::npos)
    return "";

  pos = json.find(':', pos + search.size());
  if (pos == std::string::npos)
    return "";

  // Pular whitespace
  pos++;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
    pos++;

  if (pos >= json.size())
    return "";

  if (json[pos] == '"') {
    // String value
    pos++;
    size_t end = pos;
    while (end < json.size() && json[end] != '"') {
      if (json[end] == '\\')
        end++; // skip escaped
      end++;
    }
    return json.substr(pos, end - pos);
  }

  // Non-string (number, bool, null)
  size_t end = pos;
  while (end < json.size() && json[end] != ',' && json[end] != '}' &&
         json[end] != ']')
    end++;
  std::string val = json.substr(pos, end - pos);
  // Trim whitespace
  while (!val.empty() &&
         (val.back() == ' ' || val.back() == '\n' || val.back() == '\r'))
    val.pop_back();
  return val;
}

/// Verifica se uma chave bool JSON e true
static bool JsonGetBool(const std::string &json, const std::string &key) {
  std::string val = JsonGetString(json, key);
  return val == "true";
}

/// Extrai valor int de JSON
static int JsonGetInt(const std::string &json, const std::string &key) {
  std::string val = JsonGetString(json, key);
  if (val.empty())
    return 0;
  return atoi(val.c_str());
}

/// Extrai um sub-objeto JSON { ... } pela chave
static std::string JsonGetObject(const std::string &json,
                                 const std::string &key) {
  std::string search = "\"" + key + "\"";
  size_t pos = json.find(search);
  if (pos == std::string::npos)
    return "";

  pos = json.find('{', pos + search.size());
  if (pos == std::string::npos)
    return "";

  int depth = 1;
  size_t start = pos;
  pos++;
  while (pos < json.size() && depth > 0) {
    if (json[pos] == '{')
      depth++;
    else if (json[pos] == '}')
      depth--;
    pos++;
  }
  return json.substr(start, pos - start);
}

/// Extrai um array JSON [ ... ] pela chave
static std::string JsonGetArray(const std::string &json,
                                const std::string &key) {
  std::string search = "\"" + key + "\"";
  size_t pos = json.find(search);
  if (pos == std::string::npos)
    return "";

  pos = json.find('[', pos + search.size());
  if (pos == std::string::npos)
    return "";

  int depth = 1;
  size_t start = pos;
  pos++;
  while (pos < json.size() && depth > 0) {
    if (json[pos] == '[')
      depth++;
    else if (json[pos] == ']')
      depth--;
    pos++;
  }
  return json.substr(start, pos - start);
}

/// Separa objetos de dentro de um array JSON
static std::vector<std::string> JsonSplitArray(const std::string &arr) {
  std::vector<std::string> items;
  size_t pos = 1; // skip '['
  while (pos < arr.size()) {
    // Find next '{'
    size_t start = arr.find('{', pos);
    if (start == std::string::npos)
      break;

    int depth = 1;
    size_t end = start + 1;
    while (end < arr.size() && depth > 0) {
      if (arr[end] == '{')
        depth++;
      else if (arr[end] == '}')
        depth--;
      end++;
    }
    items.push_back(arr.substr(start, end - start));
    pos = end;
  }
  return items;
}

// =============================================================================
// HELPERS — HASH
// =============================================================================
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

// =============================================================================
// CONSTRUTOR / DESTRUTOR
// =============================================================================

Database::Database() {}
Database::~Database() { Disconnect(); }

// =============================================================================
// HttpRequest — Requisicao HTTP via WinHTTP
// =============================================================================

std::string Database::HttpRequest(const std::string &method,
                                  const std::string &path,
                                  const std::string &body, int authType) {
  std::string result;

  // Converter host para wide string
  int wLen = MultiByteToWideChar(CP_UTF8, 0, m_host.c_str(), -1, nullptr, 0);
  std::wstring wHost(wLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, m_host.c_str(), -1, &wHost[0], wLen);

  int wpLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
  std::wstring wPath(wpLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wPath[0], wpLen);

  int wmLen = MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, nullptr, 0);
  std::wstring wMethod(wmLen, 0);
  MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, &wMethod[0], wmLen);

  // BRSense User-Agent ofuscado
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

  HINTERNET hRequest =
      WinHttpOpenRequest(hConnect, wMethod.c_str(), wPath.c_str(), nullptr,
                         WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
  if (!hRequest) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return "";
  }

  // Adicionar Content-Type para POST
  if (method == "POST" && !body.empty()) {
    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json\r\n",
                             (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
  }

  // Adicionar Authorization Bearer se authType == 1
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

  // Enviar request
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

// =============================================================================
// Connect — Verifica se o servidor esta acessivel
// =============================================================================

bool Database::Connect(const std::string &host, unsigned int port) {
  m_host = host;
  m_port = port;

  std::string getStr = XOR("GET");
  std::string statusStr = XOR("/api/status");

  std::string resp = HttpRequest(getStr, statusStr);
  if (resp.empty()) {
    std::string errStr = XOR("Servidor nao acessivel.");
    m_lastError = errStr;
    m_connected = false;
    return false;
  }

  m_connected = JsonGetBool(resp, "online");
  if (!m_connected) {
    m_lastError = "Servidor offline.";
  } else {
    m_lastError.clear();
  }
  return m_connected;
}

// =============================================================================
// Disconnect
// =============================================================================

void Database::Disconnect() {
  m_connected = false;
  m_userInfo = {};
  m_cheats.clear();
}

// =============================================================================
// IsConnected
// =============================================================================

bool Database::IsConnected() const { return m_connected; }

// =============================================================================
// Authenticate — POST /api/login
// =============================================================================

bool Database::Authenticate(const std::string &username,
                            const std::string &password,
                            const HardwareInfo &hwInfo) {
  // Construir JSON body com credenciais + hardware info (Obfuscado parcialmente
  // p/ seguranca)
  std::string uKey = XOR("\"username\":\"");
  std::string pKey = XOR("\",\"password\":\"");
  std::string hKey = XOR("\",\"hwid\":\"");
  std::string mKey = XOR("\",\"mac\":\"");
  std::string iKey = XOR("\",\"ip\":\"");
  std::string endObj = XOR("\"}");

  std::string body = "{" + uKey + username + pKey + password + hKey +
                     hwInfo.hwid + mKey + hwInfo.mac + iKey + hwInfo.ip +
                     endObj;

  std::string postStr = XOR("POST");
  std::string loginStr = XOR("/api/login");

  std::string resp = HttpRequest(postStr, loginStr, body);
  if (resp.empty()) {
    std::string errStr = XOR("Falha na conexao com o servidor.");
    m_lastError = errStr;
    return false;
  }

  std::string successStr = XOR("success");
  bool success = JsonGetBool(resp, successStr);
  if (!success) {
    std::string errorStr = XOR("error");
    m_lastError = JsonGetString(resp, errorStr);
    if (m_lastError.empty()) {
      std::string unkStr = XOR("Erro desconhecido.");
      m_lastError = unkStr;
    }
    return false;
  }

  // Salvar o JWT token
  std::string tokenStr = XOR("token");
  m_jwtToken = JsonGetString(resp, tokenStr);

  // Extrair dados do usuario
  std::string userObj = JsonGetObject(resp, "user");
  m_userInfo.nickname = JsonGetString(userObj, "nickname");
  m_userInfo.plan = JsonGetString(userObj, "plan");
  m_userInfo.hwid = JsonGetString(userObj, "hwid");
  m_userInfo.mac = JsonGetString(userObj, "mac");
  m_userInfo.ip = JsonGetString(userObj, "ip");

  // Extrair cheats
  m_cheats.clear();
  std::string cheatsArr = JsonGetArray(resp, "cheats");
  auto cheatItems = JsonSplitArray(cheatsArr);

  for (const auto &item : cheatItems) {
    CheatLicense cl;
    cl.id = JsonGetInt(item, "id");
    cl.game = JsonGetString(item, "game");
    cl.name = JsonGetString(item, "name");
    cl.iconColor = JsonGetString(item, "icon_color");
    cl.process = JsonGetString(item, "process");
    cl.hash = JsonGetString(item, "hash");
    cl.notes = JsonGetString(item, "notes");
    cl.timeLeft = JsonGetString(item, "time_left");
    cl.status = JsonGetString(item, "status");
    m_cheats.push_back(cl);
  }

  m_lastError.clear();
  return true;
}

// =============================================================================
// GetServerStatus — GET /api/status
// =============================================================================

ServerStatus Database::GetServerStatus() {
  ServerStatus ss;
  std::string resp = HttpRequest("GET", "/api/status");
  if (resp.empty())
    return ss;

  ss.online = JsonGetBool(resp, "online");
  ss.version = JsonGetString(resp, "version");
  ss.region = JsonGetString(resp, "region");
  ss.lastUpdate = JsonGetString(resp, "last_update");
  ss.ping = JsonGetInt(resp, "ping");

  return ss;
}

// =============================================================================
// RefreshCheats — GET /api/cheats (atualiza lista de cheats)
// =============================================================================

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

  bool success = JsonGetBool(resp, "success");
  if (!success) {
    m_lastError = JsonGetString(resp, "error");
    return false;
  }

  m_cheats.clear();
  std::string cheatsArr = JsonGetArray(resp, "cheats");
  auto cheatItems = JsonSplitArray(cheatsArr);

  for (const auto &item : cheatItems) {
    CheatLicense cl;
    cl.id = JsonGetInt(item, "id");
    cl.game = JsonGetString(item, "game");
    cl.name = JsonGetString(item, "name");
    cl.iconColor = JsonGetString(item, "icon_color");
    cl.process = JsonGetString(item, "process");
    cl.hash = JsonGetString(item, "hash");
    cl.notes = JsonGetString(item, "notes");
    cl.timeLeft = JsonGetString(item, "time_left");
    cl.status = JsonGetString(item, "status");
    m_cheats.push_back(cl);
  }

  m_lastError.clear();
  return true;
}

// =============================================================================
// Heartbeat — POST /api/heartbeat
// =============================================================================

bool Database::Heartbeat() {
  if (m_jwtToken.empty())
    return false;
  std::string resp =
      HttpRequest("POST", "/api/heartbeat", "{}", 1); // 1 = Bearer Token
  if (resp.empty())
    return false;

  bool success = JsonGetBool(resp, "success");
  if (!success) {
    m_lastError = JsonGetString(resp, "error");
    return false;
  }
  return true;
}

// =============================================================================
// RequestHwidReset — POST /api/hwid_reset
// =============================================================================

bool Database::RequestHwidReset(const std::string &reason) {
  if (m_jwtToken.empty()) {
    m_lastError = "Nao autenticado.";
    return false;
  }

  std::string body = "{\"reason\":\"" + reason + "\"}";
  std::string resp = HttpRequest("POST", "/api/hwid_reset", body, 1);

  if (resp.empty()) {
    m_lastError = "Falha na conexao com o servidor.";
    return false;
  }

  bool success = JsonGetBool(resp, "success");
  if (!success) {
    m_lastError = JsonGetString(resp, "error");
    return false;
  }
  return true;
}

// =============================================================================
// HttpDownloadRequest — Retorna bytes ignorando parsing de string
// =============================================================================

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

  HINTERNET hRequest =
      WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(), nullptr,
                         WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
  if (!hRequest) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  // Adicionar JWT Token
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

    // Se nao for 200 OK, a requisicao falhou ou foi bloqueada (ex: sem licenca)
    if (dwStatusCode != 200) {

      // Tentar ler a mensagem de erro JSON retornada
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
        m_lastError = JsonGetString(errorResp, "error");
      }
      if (m_lastError.empty())
        m_lastError =
            "Download refused. HTTP Code: " + std::to_string(dwStatusCode);

      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      return false;
    }

    // Baixar dados
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

// =============================================================================
// DownloadCheat — GET /api/download/:cheat_id
// =============================================================================

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
