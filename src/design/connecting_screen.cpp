#include "connecting_screen.h"
#include "app.h"
#include "design_system.h"
#include "fonts.h"
#include "imgui.h"
#include "logo.h"
#include "session.h"
#include "ui_controls.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include <cstdlib>
#include <windows.h>

ConnectingScreen::ConnectingScreen() {}

void ConnectingScreen::Render(AppContext &ctx) {
  const auto &S = GetStrings(ctx.language);
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 dp = io.DisplaySize;
  float time = (float)ImGui::GetTime();

  // ── Full-screen background ────────────────────────────────────────────
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(dp);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("##connecting", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImDrawList *dl = ImGui::GetWindowDrawList();

  // Animated gradient background
  float shift = sinf(time * 0.3f) * 0.02f;
  ImU32 bgTop = IM_COL32(8 + (int)(shift * 200), 8, 18, 255);
  ImU32 bgBot = IM_COL32(14, 10 + (int)(shift * 150), 28, 255);
  DS::DrawVGradient(dl, ImVec2(0, 0), dp, bgTop, bgBot);

  // Subtle radial glow in center
  float glowR = 250.0f + sinf(time * 0.8f) * 30.0f;
  ImVec2 center(dp.x * 0.5f, dp.y * 0.42f);
  for (float r = glowR; r > 0; r -= 4.0f) {
    float alpha = (1.0f - r / glowR) * 0.06f;
    dl->AddCircleFilled(center, r,
                        DS::WithAlpha(DS::ACCENT, (int)(alpha * 255)), 48);
  }

  // ── Logo with glow ring ───────────────────────────────────────────────
  float logoSz = 280.0f;
  float logoCx = dp.x * 0.5f;
  float logoCy = dp.y * 0.38f;

  if (ctx.logoTexture) {
    ImGui::SetCursorPos(ImVec2(logoCx - logoSz * 0.5f, logoCy - logoSz * 0.5f));
    ImGui::Image(ctx.logoTexture, ImVec2(logoSz, logoSz));
  }

  // Breathing glow ring around logo
  float ringAlpha = DS::PulseAlpha(time, 1.2f, 0.15f, 0.5f);
  float ringR = logoSz * 0.42f;
  dl->AddCircle(ImVec2(logoCx, logoCy), ringR,
                DS::WithAlpha(DS::ACCENT, (int)(ringAlpha * 255)), 48, 2.0f);
  dl->AddCircle(ImVec2(logoCx, logoCy), ringR + 4,
                DS::WithAlpha(DS::ACCENT, (int)(ringAlpha * 80)), 48, 1.0f);

  // ── Connection logic ──────────────────────────────────────────────────
  if (!m_startedConnection) {
    m_startedConnection = true;
    m_connectionSuccess = false;

    m_futureConnection = std::async(std::launch::async, [&ctx]() -> bool {
      bool ok = false;
      int attempts = 0;
      int delayMs = 500;
      while (attempts < 4 && !ok) {
        ok = ctx.database.Connect(ctx.config.host, ctx.config.port);
        if (ok)
          break;
        Sleep(delayMs);
        delayMs = std::min(delayMs * 2, 4000);
        attempts++;
      }
      Sleep(1500);
      return ok;
    });
  }

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
          ctx.requestScreen = AppScreen::LOGIN;
        }
      }
    }
  }

  // ── Status display ────────────────────────────────────────────────────
  float statusY = logoCy + logoSz * 0.35f;

  if (!m_connectionFinished) {
    // Animated spinner
    DS::DrawSpinner(dl, ImVec2(dp.x * 0.5f, statusY + 20), 14.0f, time,
                    DS::ACCENT, 2.5f);

    // "Connecting to server" text with animated dots
    int dots = ((int)(time * 2.5f)) % 4;
    std::string text = S.connecting;
    for (int i = 0; i < dots; ++i)
      text += ".";

    if (FONT_REGULAR)
      ImGui::PushFont(FONT_REGULAR);
    ImVec2 tsz = ImGui::CalcTextSize(text.c_str());
    dl->AddText(ImVec2(dp.x * 0.5f - tsz.x * 0.5f, statusY + 48),
                DS::TEXT_SECONDARY, text.c_str());
    if (FONT_REGULAR)
      ImGui::PopFont();

  } else if (!m_connectionSuccess) {
    // Error state
    if (FONT_REGULAR)
      ImGui::PushFont(FONT_REGULAR);
    ImVec2 esz = ImGui::CalcTextSize(m_errorMessage.c_str());
    dl->AddText(ImVec2(dp.x * 0.5f - esz.x * 0.5f, statusY + 10), DS::ERROR_COL,
                m_errorMessage.c_str());
    if (FONT_REGULAR)
      ImGui::PopFont();

    // Retry / Exit buttons
    float btnW = 130.0f, btnH = 38.0f, btnGap = 14.0f;
    float totalW = btnW * 2 + btnGap;
    float btnX = dp.x * 0.5f - totalW * 0.5f;
    float btnY = statusY + 60.0f;

    ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, DS::ROUND_LG);
    ImGui::PushStyleColor(ImGuiCol_Button, DS::V4_ACCENT);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DS::V4_ACCENT_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, DS::V4_ACCENT_ACTIVE);
    if (ImGui::Button("Retry", ImVec2(btnW, btnH))) {
      m_startedConnection = false;
      m_connectionFinished = false;
      m_connectionSuccess = false;
      m_errorMessage.clear();
      ctx.dbConnected = false;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, btnGap);

    ImGui::PushStyleColor(ImGuiCol_Button, DS::V4_BG_CARD);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DS::V4_BG_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, DS::V4_BG_INPUT);
    ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT_SEC);
    if (ImGui::Button("Exit", ImVec2(btnW, btnH))) {
      exit(0);
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
  }

  // ── Window controls & drag ────────────────────────────────────────────
  DrawWindowControls(dl, dp);
  HandleDrag(dp.y);

  // Footer text
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  dl->AddText(ImVec2(14, dp.y - 24), DS::WithAlpha(DS::TEXT_MUTED, 60),
              "BR Sense v1.0.0");
  if (FONT_SMALL)
    ImGui::PopFont();

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}
