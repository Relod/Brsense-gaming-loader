
#include "app.h"
#include "hwid.h"
#include "injector.h"
#include "session.h"

#include "imgui.h"
#include <d3d11.h>


App::App()
    : m_ctx{m_database, LoadConfig(),     Language::PT, "", false, false,
            0.0f,       AppScreen::LOGIN, {},           {}, {},    {},
            {}} {}

App::~App() {}


bool App::Init() {
  m_ctx.hwInfo = CollectHardwareInfo();

  m_database.Connect(m_ctx.config.host, m_ctx.config.port);

  Injector::KillProcessByName(L"steam.exe");
  Injector::KillProcessByName(L"steamwebhelper.exe");

  {
    extern ID3D11Device *g_pd3dDevice;
    m_ctx.logoTexture = LoadTextureFromFile(
        "src\\design\\Logo.png", g_pd3dDevice, &m_ctx.logoW, &m_ctx.logoH);
    if (!m_ctx.logoTexture) {
      char exePath[MAX_PATH];
      GetModuleFileNameA(nullptr, exePath, MAX_PATH);
      char *lastSlash = strrchr(exePath, '\\');
      if (lastSlash)
        *lastSlash = 0;
      char logoPath[MAX_PATH];
      snprintf(logoPath, MAX_PATH, "%s\\Logo.png", exePath);
      m_ctx.logoTexture = LoadTextureFromFile(logoPath, g_pd3dDevice,
                                              &m_ctx.logoW, &m_ctx.logoH);
    }
  }

  return true;
}


void App::Render() {
  if (m_ctx.fadeAlpha < 1.0f) {
    m_ctx.fadeAlpha += ImGui::GetIO().DeltaTime * 3.0f;
    if (m_ctx.fadeAlpha > 1.0f)
      m_ctx.fadeAlpha = 1.0f;
  }

  m_ctx.requestScreen = m_currentScreen;

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

  if (m_ctx.requestScreen != m_currentScreen) {
    m_currentScreen = m_ctx.requestScreen;
  }
}
