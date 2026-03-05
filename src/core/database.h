
#pragma once

#include "hwid.h"

#include <string>
#include <vector>

struct CheatLicense {
  int id = 0;
  std::string game;
  std::string name;
  std::string iconColor;
  std::string process;
  std::string injectionMethod;
  std::string killProcesses;
  std::string launchParams;
  std::string steamAppId;
  bool requiresAdmin = false;
  bool enabled = true;
  std::string hash;
  std::string notes;
  std::string timeLeft;
  std::string status;
};

struct UserInfo {
  std::string nickname;
  std::string plan;
  std::string hwid;
  std::string mac;
  std::string ip;
};

struct ServerStatus {
  bool online = false;
  std::string version;
  std::string region;
  std::string lastUpdate;
  int ping = 0;
};

struct UpdateInfo {
  bool updateAvailable = false;
  std::string latestVersion;
  std::string changelog;
  bool fileExists = false;
};

class Database {
public:
  Database();
  ~Database();

  bool Connect(const std::string &host = "localhost", unsigned int port = 3000);

  void Disconnect();

  bool IsConnected() const;

  bool Authenticate(const std::string &username, const std::string &password,
                    const HardwareInfo &hwInfo);

  ServerStatus GetServerStatus();

  bool Heartbeat();

  bool RequestHwidReset(const std::string &reason);

  bool RequestHwidResetByCreds(const std::string &username,
                               const std::string &password,
                               const std::string &reason);

  bool DownloadCheat(int cheatId, std::vector<uint8_t> &outBuffer,
                     const std::string &expectedSha256 = "");

  bool RefreshCheats();

  UpdateInfo CheckForUpdate(const std::string &currentVersion);

  const UserInfo &GetUserInfo() const { return m_userInfo; }
  const std::vector<CheatLicense> &GetCheats() const { return m_cheats; }
  const std::string &GetJwtToken() const { return m_jwtToken; }

  const std::string &GetLastError() const { return m_lastError; }

private:
  std::string HttpRequest(const std::string &method, const std::string &path,
                          const std::string &body = "", int authType = 0);

  bool HttpDownloadRequest(const std::string &path,
                           std::vector<uint8_t> &outBuffer);

  std::string m_host = "localhost";
  unsigned int m_port = 3000;
  bool m_useHttps = false;
  bool m_connected = false;
  std::string m_lastError;

  UserInfo m_userInfo;
  std::vector<CheatLicense> m_cheats;
  std::string m_jwtToken;
};
