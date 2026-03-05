
#include "config.h"

#include <fstream>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>


static bool ParseLine(const std::string &line, std::string &key,
                      std::string &value) {
  auto pos = line.find('=');
  if (pos == std::string::npos)
    return false;
  key = line.substr(0, pos);
  value = line.substr(pos + 1);
  auto l = key.find_first_not_of(" \t\r\n");
  auto r = key.find_last_not_of(" \t\r\n");
  if (l == std::string::npos)
    return false;
  key = key.substr(l, r - l + 1);
  l = value.find_first_not_of(" \t\r\n");
  r = value.find_last_not_of(" \t\r\n");
  if (l == std::string::npos)
    value.clear();
  else
    value = value.substr(l, r - l + 1);
  return true;
}

static bool LoadFromPath(const std::string &path, LoaderConfig &cfg) {
  std::ifstream f(path);
  if (!f.is_open())
    return false;
  std::string line;
  while (std::getline(f, line)) {
    std::string k, v;
    if (!ParseLine(line, k, v))
      continue;
    if (k == "host")
      cfg.host = v;
    else if (k == "port") {
      try {
        cfg.port = static_cast<unsigned int>(std::stoul(v));
      } catch (...) {
      }
    }
  }
  return true;
}

LoaderConfig LoadConfig() {
  LoaderConfig cfg;

  // 1. Try %APPDATA%\BRSense\config.txt
  char appData[MAX_PATH] = {0};
  if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
    std::string appPath = std::string(appData) + "\\BRSense\\config.txt";
    LoadFromPath(appPath, cfg);
  }

  // 2. Try next to executable (overrides APPDATA values)
  char exePath[MAX_PATH] = {0};
  if (GetModuleFileNameA(nullptr, exePath, MAX_PATH)) {
    std::string path(exePath);
    auto pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
      path = path.substr(0, pos + 1) + "brsense_config.txt";
      LoadFromPath(path, cfg);
    }
  }

  return cfg;
}
