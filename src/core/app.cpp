// =============================================================================
// app.cpp — Implementacao do Orquestrador da Aplicacao
// =============================================================================
// Responsabilidades:
//   1. Coletar informacoes de hardware (HWID, MAC, IP) antes do login
//   2. Conectar ao servidor backend
//   3. Tentar auto-login com sessao salva
//   4. Gerenciar o fade-in entre telas
//   5. Delegar renderizacao para LoginScreen ou LoaderScreen
//   6. Processar transicoes de tela via AppContext::requestScreen
// =============================================================================

#include "app.h"
#include "injector.h"
#include "hwid.h"
#include "session.h"

#include "imgui.h"

// =============================================================================
// CONSTRUTOR / DESTRUTOR
// =============================================================================

App::App()
    : m_ctx{m_database, LoadConfig(),     Language::PT, "", false, false,
            0.0f,       AppScreen::LOGIN, {},           {}, {},    {},
            {}} {}

App::~App() {}

// =============================================================================
// INIT — Coleta hardware, conecta ao servidor, tenta auto-login
// =============================================================================

bool App::Init() {
  // ── Coletar informacoes de hardware ANTES de tudo ─────────────────────
  // Isso agora é feito de forma rapida no inicio, enquando a ConnectingScreen
  // vai rodar.
  m_ctx.hwInfo = CollectHardwareInfo();

  // Atualizar host/porta no Database (override defaults)
  m_database.Connect(m_ctx.config.host, m_ctx.config.port);

  // Limpar processos do Steam e do jogo prevendo uma injeção limpa
  Injector::KillProcessByName(L"cs2.exe");
  Injector::KillProcessByName(L"steam.exe");
  Injector::KillProcessByName(L"steamwebhelper.exe");

  return true;
}

// =============================================================================
// RENDER — Frame principal
// =============================================================================

void App::Render() {
  // ── Fade-in entre telas ───────────────────────────────────────────────
  if (m_ctx.fadeAlpha < 1.0f) {
    m_ctx.fadeAlpha += ImGui::GetIO().DeltaTime * 3.0f;
    if (m_ctx.fadeAlpha > 1.0f)
      m_ctx.fadeAlpha = 1.0f;
  }

  // ── Sincronizar tela solicitada ───────────────────────────────────────
  m_ctx.requestScreen = m_currentScreen;

  // ── Renderizar a tela ativa ───────────────────────────────────────────
  switch (m_currentScreen) {
  case AppScreen::CONNECTING:
    m_connectingScreen.Render(m_ctx);
    break;
  case AppScreen::LOGIN:
    m_loginScreen.Render(m_ctx);
    break;
  case AppScreen::LOADER:
    m_loaderScreen.Render(m_ctx);
    break;
  }

  // ── Processar transicao de tela (se solicitada) ───────────────────────
  if (m_ctx.requestScreen != m_currentScreen) {
    m_currentScreen = m_ctx.requestScreen;
  }
}
