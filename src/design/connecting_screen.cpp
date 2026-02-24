// =============================================================================
// connecting_screen.cpp
// =============================================================================
#include "connecting_screen.h"
#include "app.h"
#include "imgui.h"
#include "logo.h"
#include "session.h"
#include "ui_controls.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cstdlib>

ConnectingScreen::ConnectingScreen() {}

void ConnectingScreen::Render(AppContext &ctx) {
  const auto &S = GetStrings(ctx.language);
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 dp = io.DisplaySize;
  float time = (float)ImGui::GetTime();

  // Fundo principal escuro
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(dp);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.09f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("##connecting", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImDrawList *dl = ImGui::GetWindowDrawList();

  // Desenhar Logo principal animado centralizado (como no LoginScreen)
  float logoSz = 120.0f;
  DrawAnubisLogo(dl, dp.x / 2.0f, dp.y / 2.0f - 40.0f, logoSz, time, 255);

  // Iniciar conexao em background (Apenas uma vez)
  if (!m_startedConnection) {
    m_startedConnection = true;
    m_connectionSuccess = false;

    // std::async inicia a thread background que nao trava o ImGui
    m_futureConnection = std::async(std::launch::async, [&ctx]() -> bool {
      // Coleta HWID (ja esta no App::Init, mas garantimos usar)
      // Conectar
      bool ok = false;
      int attempts = 0;
      int delayMs = 500;
      while (attempts < 4 && !ok) {
        ok = ctx.database.Connect(ctx.config.host, ctx.config.port);
        if (ok)
          break;
        Sleep(delayMs);
        delayMs = std::min(delayMs * 2, 4000); // backoff ate 4s
        attempts++;
      }

      // Simular um tempo de conexao agradavel para a interface nao piscar mt
      // rapido
      Sleep(1500);

      return ok;
    });
  }

  // Checar se a thread terminou
  if (!m_connectionFinished) {
    if (m_futureConnection.valid() &&
        m_futureConnection.wait_for(std::chrono::seconds(0)) ==
            std::future_status::ready) {
      m_connectionFinished = true;
      m_connectionSuccess = m_futureConnection.get();
      ctx.dbConnected = m_connectionSuccess;

      if (!m_connectionSuccess) {
        m_errorMessage = ctx.database.GetLastError();
        if (m_errorMessage.empty())
          m_errorMessage = S.serverOffline;
      } else {
        // Sucesso — Auto Login?
        SessionData data;
        bool autoLogged = false;
        if (SessionLoad(data)) {
          if (ctx.database.Authenticate(data.username, data.password,
                                        ctx.hwInfo)) {
            ctx.loggedUser = data.username;
            ctx.userInfo = ctx.database.GetUserInfo();
            ctx.cheats = ctx.database.GetCheats();
            ctx.serverStatus = ctx.database.GetServerStatus();
            ctx.logs.push_back("[AUTO] Session restored");
            ctx.requestScreen = AppScreen::LOADER;
            autoLogged = true;
          } else {
            SessionClear();
          }
        }

        if (!autoLogged) {
          ctx.requestScreen = AppScreen::LOGIN; // Login normal
        }
      }
    }
  }

  // Desenhar os Requisitos Visuais: Textos
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);

  if (!m_connectionFinished) {
    // Animacao de pontos (...)
    int dots = ((int)(time * 2.5f)) % 4;
    std::string text = S.connecting;
    for (int i = 0; i < dots; ++i)
      text += ".";

    ImVec2 tsz = ImGui::CalcTextSize(text.c_str());
    ImGui::SetCursorPos(ImVec2((dp.x - tsz.x) / 2.0f, dp.y / 2.0f + 60.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.85f, 1.0f));
    ImGui::Text("%s", text.c_str());
    ImGui::PopStyleColor();
  } else if (!m_connectionSuccess) {
    // Falha exibida
    ImVec2 esz = ImGui::CalcTextSize(m_errorMessage.c_str());
    ImGui::SetCursorPos(ImVec2((dp.x - esz.x) / 2.0f, dp.y / 2.0f + 50.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.3f, 1.0f));
    ImGui::Text("%s", m_errorMessage.c_str());
    ImGui::PopStyleColor();

    // Ações: tentar de novo ou sair
    ImVec2 btnSize = ImVec2(110.0f, 32.0f);
    float totalW = btnSize.x * 2.0f + 12.0f;
    ImGui::SetCursorPos(ImVec2((dp.x - totalW) / 2.0f, dp.y / 2.0f + 110.0f));
    if (ImGui::Button("Retry", btnSize)) {
      // Resetar estado para uma nova tentativa
      m_startedConnection = false;
      m_connectionFinished = false;
      m_connectionSuccess = false;
      m_errorMessage.clear();
      ctx.dbConnected = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit", btnSize)) {
      exit(0);
    }
  }

  ImGui::PopStyleVar();

  // Controles da janela (pode fechar durante a conexao)
  DrawWindowControls(dl, dp);
  HandleDrag(dp.y); // Toda a tela e arrastavel no loading

  // Tag alpha no canto inferior
  dl->AddText(ImVec2(10, dp.y - 20), IM_COL32(255, 255, 255, 30),
              "BR Sense - Checking Server");

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}
