
#pragma once

#include "config.h"
#include "connecting_screen.h"
#include "database.h"
#include "hwid.h"
#include "loader_screen.h"
#include "login_screen.h"
#include "strings.h"
#include "texture_loader.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

enum class AppScreen {
  CONNECTING,
  LOGIN,
  LOADER
};

struct AppContext {
  Database &database;
  LoaderConfig config;
  Language language;
  std::string loggedUser;
  bool dbConnected;
  bool keepLoggedIn;
  float fadeAlpha;
  AppScreen requestScreen;

  HardwareInfo hwInfo;

  UserInfo userInfo;
  std::vector<CheatLicense> cheats;
  ServerStatus serverStatus;
  std::vector<std::string> logs;
  std::mutex logMutex;
  std::atomic<bool> isInjecting{false};
  std::atomic<bool> shouldExit{false};

  ImTextureID logoTexture = 0;
  int logoW = 0, logoH = 0;
};

class App {
public:
  App();
  ~App();

  bool Init();

  void Render();

  AppScreen GetScreen() const { return m_currentScreen; }

  bool ShouldExit() const { return m_ctx.shouldExit; }

private:
  AppScreen m_currentScreen = AppScreen::CONNECTING;
  Database m_database;
  AppContext m_ctx;

  ConnectingScreen m_connectingScreen;
  LoginScreen m_loginScreen;
  LoaderScreen m_loaderScreen;

  bool m_sessionLoaded = false;
};
