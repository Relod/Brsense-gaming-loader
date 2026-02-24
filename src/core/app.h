// =============================================================================
// app.h - Cabecalho da Classe App (Orquestrador)
// =============================================================================
// A classe App orquestra:
//   - O contexto compartilhado (AppContext)
//   - A transicao entre telas (LOGIN <-> LOADER)
//   - A inicializacao do servidor e auto-login
//
// Toda a logica de renderizacao esta nos modulos em design/:
//   - login_screen.h/cpp  -> Tela de login
//   - loader_screen.h/cpp -> Tela do loader
// =============================================================================

#pragma once

#include "config.h"
#include "connecting_screen.h"
#include "database.h"
#include "hwid.h"
#include "loader_screen.h"
#include "login_screen.h"
#include "strings.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// Telas disponiveis
enum class AppScreen {
  CONNECTING, ///< Tela inicial de loading
  LOGIN,      ///< Tela de login
  LOADER      ///< Tela do loader de cheats
};

// Contexto compartilhado entre telas
struct AppContext {
  Database &database;      ///< Referencia ao API client
  LoaderConfig config;     ///< Host/porta carregados de arquivo
  Language language;       ///< Idioma atual da interface (EN/PT)
  std::string loggedUser;  ///< Nome do usuario logado (username)
  bool dbConnected;        ///< Se o servidor esta conectado
  bool keepLoggedIn;       ///< Se a opcao "manter conectado" esta marcada
  float fadeAlpha;         ///< Alpha do fade-in entre telas (0->1)
  AppScreen requestScreen; ///< Tela solicitada (para transicoes)

  // Hardware info (coletado antes do login)
  HardwareInfo hwInfo; ///< HWID, MAC, IP reais da maquina

  // Dados de usuario e estado da UI
  UserInfo userInfo;
  std::vector<CheatLicense> cheats;
  ServerStatus serverStatus;
  std::vector<std::string> logs;
  std::mutex logMutex;
  std::atomic<bool> isInjecting{false};
  std::atomic<bool> shouldExit{false};
};

// Orquestrador da aplicacao
class App {
public:
  App();
  ~App();

  /// Inicializa o servidor e tenta auto-login.
  bool Init();

  /// Renderiza o frame atual (delega para a tela ativa).
  void Render();

  /// Retorna a tela atual.
  AppScreen GetScreen() const { return m_currentScreen; }

  /// Retorna se a aplicacao deve fechar.
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
