
#include "login_screen.h"
#include "app.h"
#include "design_system.h"
#include "fonts.h"
#include "logo.h"
#include "session.h"
#include "strings.h"
#include "ui_controls.h"

#include "imgui.h"
#include <cmath>
#include <cstdlib>
#include <cstring>


LoginScreen::LoginScreen() {}

void LoginScreen::Render(AppContext &ctx) {
  const auto &S = GetStrings(ctx.language);
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 dp = io.DisplaySize;
  float time = (float)ImGui::GetTime();
  float dt = io.DeltaTime;

  if (m_errorTimer > 0.0f)
    m_errorTimer -= dt;

  // Initialize particles
  if (!m_particlesInit) {
    m_particlesInit = true;
    srand(42);
    for (auto &p : m_particles) {
      p.x = (float)(rand() % (int)dp.x);
      p.y = (float)(rand() % (int)dp.y);
      p.vx = ((rand() % 100) - 50) * 0.12f;
      p.vy = ((rand() % 100) - 50) * 0.08f;
      p.r = 1.2f + (rand() % 20) * 0.1f;
      p.alpha = 20 + rand() % 40;
    }
  }

  float splitX = dp.x * 0.40f;

  // ═══════════════════════════════════════════════════════════════════════
  // LEFT PANEL — Logo + Particle Mesh
  // ═══════════════════════════════════════════════════════════════════════
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(dp);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("##bg", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                   ImGuiWindowFlags_NoNav);

  ImDrawList *bgDl = ImGui::GetWindowDrawList();

  // Left panel gradient
  bgDl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(splitX, dp.y),
                                IM_COL32(8, 8, 22, 255),   // top-left
                                IM_COL32(16, 12, 32, 255), // top-right
                                IM_COL32(24, 14, 44, 255), // bottom-right
                                IM_COL32(10, 8, 28, 255)   // bottom-left
  );

  // Right panel background
  bgDl->AddRectFilled(ImVec2(splitX, 0), ImVec2(dp.x, dp.y),
                      IM_COL32(16, 16, 24, 255));

  // Divider line with glow
  bgDl->AddLine(ImVec2(splitX, 0), ImVec2(splitX, dp.y),
                DS::WithAlpha(DS::ACCENT, 50), 1.0f);
  bgDl->AddLine(ImVec2(splitX - 1, 0), ImVec2(splitX - 1, dp.y),
                DS::WithAlpha(DS::ACCENT, 15), 1.5f);

  // ── Particle mesh ─────────────────────────────────────────────────────
  for (auto &p : m_particles) {
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    if (p.x < -10)
      p.x = dp.x + 10;
    if (p.x > dp.x + 10)
      p.x = -10;
    if (p.y < -10)
      p.y = dp.y + 10;
    if (p.y > dp.y + 10)
      p.y = -10;
  }

  // Draw connections between nearby particles (mesh effect)
  float connectDist = 120.0f;
  for (int i = 0; i < 40; ++i) {
    for (int j = i + 1; j < 40; ++j) {
      float dx = m_particles[i].x - m_particles[j].x;
      float dy = m_particles[i].y - m_particles[j].y;
      float dist = sqrtf(dx * dx + dy * dy);
      if (dist < connectDist) {
        float alpha = (1.0f - dist / connectDist) * 0.15f;
        bool leftSide =
            (m_particles[i].x < splitX && m_particles[j].x < splitX);
        int a = leftSide ? (int)(alpha * 255) : (int)(alpha * 60);
        bgDl->AddLine(ImVec2(m_particles[i].x, m_particles[i].y),
                      ImVec2(m_particles[j].x, m_particles[j].y),
                      DS::WithAlpha(DS::ACCENT, a), 0.8f);
      }
    }
  }

  // Draw particle dots
  for (const auto &p : m_particles) {
    bool leftSide = (p.x < splitX);
    int a = leftSide ? p.alpha : p.alpha / 4;
    bgDl->AddCircleFilled(ImVec2(p.x, p.y), p.r, DS::WithAlpha(DS::ACCENT, a),
                          8);
  }

  // ── Logo ──────────────────────────────────────────────────────────────
  {
    float cx = splitX * 0.5f;
    float cy = dp.y * 0.42f;

    if (ctx.logoTexture) {
      float logoSize = 260.0f;
      ImGui::SetCursorPos(ImVec2(cx - logoSize * 0.5f, cy - logoSize * 0.5f));
      ImGui::Image(ctx.logoTexture, ImVec2(logoSize, logoSize));
    }

    // Subtle glow ring around logo area
    float ringAlpha = DS::PulseAlpha(time, 0.8f, 0.1f, 0.3f);
    bgDl->AddCircle(ImVec2(cx, cy), 100.0f,
                    DS::WithAlpha(DS::ACCENT, (int)(ringAlpha * 255)), 48,
                    1.5f);
  }

  // Version footer
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  bgDl->AddText(ImVec2(14, dp.y - 24), DS::WithAlpha(DS::TEXT_MUTED, 60),
                "v1.0.0");
  if (FONT_SMALL)
    ImGui::PopFont();

  DrawLangFlags(bgDl, ctx.language, 12, 9);
  HandleDrag();

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  // ═══════════════════════════════════════════════════════════════════════
  // RIGHT PANEL — Login Form
  // ═══════════════════════════════════════════════════════════════════════
  float formW = dp.x - splitX;
  float formContentW = 320.0f;
  float relX = (formW - formContentW) * 0.5f;

  ImGui::SetNextWindowPos(ImVec2(splitX, 0));
  ImGui::SetNextWindowSize(ImVec2(formW, dp.y));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(0.063f, 0.063f, 0.094f, 1.0f));
  ImGui::Begin("##form", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize);

  ImDrawList *fDl = ImGui::GetWindowDrawList();

  float startY = dp.y * 0.20f;

  // ── Welcome Title ─────────────────────────────────────────────────────
  ImGui::SetCursorPos(ImVec2(relX, startY));
  if (FONT_BOLD)
    ImGui::PushFont(FONT_BOLD);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT);
  ImGui::Text("%s", S.welcome);
  ImGui::PopStyleColor();
  if (FONT_BOLD)
    ImGui::PopFont();

  ImGui::SetCursorPosX(relX);
  if (FONT_REGULAR)
    ImGui::PushFont(FONT_REGULAR);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT_SEC);
  ImGui::Text("%s", S.signIn);
  ImGui::PopStyleColor();
  if (FONT_REGULAR)
    ImGui::PopFont();

  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 32));

  // ── Username Field ────────────────────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT_SEC);
  ImGui::Text("%s", S.username);
  ImGui::PopStyleColor();
  if (FONT_SMALL)
    ImGui::PopFont();

  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 4));

  ImGui::SetCursorPosX(relX);
  ImGui::PushItemWidth(formContentW);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 12));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, DS::ROUND_MD);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, DS::V4_BG_INPUT);
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, DS::V4_BG_HOVER);
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, DS::V4_BG_HOVER);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT);
  ImGui::PushStyleColor(ImGuiCol_Border, DS::V4_BORDER);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

  if (FONT_REGULAR)
    ImGui::PushFont(FONT_REGULAR);
  ImGui::InputText("##user", m_username, sizeof(m_username));
  if (FONT_REGULAR)
    ImGui::PopFont();
  bool userActive = ImGui::IsItemActive();

  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(5);
  ImGui::PopStyleVar(2);
  ImGui::PopItemWidth();

  // Focus glow border
  if (userActive) {
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    fDl->AddRect(mn, mx, DS::BORDER_FOCUS, DS::ROUND_MD, 0, 2.0f);
    DS::DrawGlowRect(fDl, mn, mx, DS::ACCENT, 4.0f, DS::ROUND_MD);
  }

  // ── Password Field ────────────────────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 14));

  ImGui::SetCursorPosX(relX);
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT_SEC);
  ImGui::Text("%s", S.password);
  ImGui::PopStyleColor();
  if (FONT_SMALL)
    ImGui::PopFont();

  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 4));

  ImGui::SetCursorPosX(relX);
  ImGui::PushItemWidth(formContentW);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 12));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, DS::ROUND_MD);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, DS::V4_BG_INPUT);
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, DS::V4_BG_HOVER);
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, DS::V4_BG_HOVER);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT);
  ImGui::PushStyleColor(ImGuiCol_Border, DS::V4_BORDER);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

  if (FONT_REGULAR)
    ImGui::PushFont(FONT_REGULAR);
  bool enterPressed = ImGui::InputText(
      "##pass", m_password, sizeof(m_password),
      ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
  if (FONT_REGULAR)
    ImGui::PopFont();
  bool passActive = ImGui::IsItemActive();

  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(5);
  ImGui::PopStyleVar(2);
  ImGui::PopItemWidth();

  if (passActive) {
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    fDl->AddRect(mn, mx, DS::BORDER_FOCUS, DS::ROUND_MD, 0, 2.0f);
    DS::DrawGlowRect(fDl, mn, mx, DS::ACCENT, 4.0f, DS::ROUND_MD);
  }

  // ── Keep Logged + Forgot Password ─────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 12));

  ImGui::SetCursorPosX(relX);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, DS::V4_BG_INPUT);
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, DS::V4_BG_HOVER);
  ImGui::PushStyleColor(ImGuiCol_CheckMark, DS::V4_ACCENT);
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  ImGui::Checkbox(S.keepLogged, &ctx.keepLoggedIn);
  if (FONT_SMALL)
    ImGui::PopFont();
  ImGui::PopStyleColor(3);

  {
    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    ImGui::SameLine(relX + formContentW - ImGui::CalcTextSize(S.forgotPass).x);
    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(DS::V4_ACCENT.x, DS::V4_ACCENT.y, DS::V4_ACCENT.z, 0.7f));
    ImGui::Text("%s", S.forgotPass);
    ImGui::PopStyleColor();
    if (FONT_SMALL)
      ImGui::PopFont();
  }

  // ── Sign In Button ────────────────────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 24));

  ImGui::SetCursorPosX(relX);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, DS::ROUND_LG);
  ImGui::PushStyleColor(ImGuiCol_Button, DS::V4_ACCENT);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DS::V4_ACCENT_HOVER);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, DS::V4_ACCENT_ACTIVE);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

  if (FONT_SEMIBOLD)
    ImGui::PushFont(FONT_SEMIBOLD);
  bool doLogin =
      ImGui::Button(S.loginBtn, ImVec2(formContentW, 48)) || enterPressed;
  if (FONT_SEMIBOLD)
    ImGui::PopFont();

  // Glow effect on hover
  if (ImGui::IsItemHovered()) {
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    DS::DrawGlowRect(fDl, mn, mx, DS::ACCENT, 8.0f, DS::ROUND_LG);
  }

  ImGui::PopStyleColor(4);
  ImGui::PopStyleVar();

  // ── Login Logic ───────────────────────────────────────────────────────
  if (doLogin) {
    std::string u(m_username), p(m_password);
    if (u.empty() || p.empty()) {
      m_errorMsg = S.fillFields;
      m_errorTimer = 4.0f;
    } else if (ctx.database.Authenticate(u, p, ctx.hwInfo)) {
      ctx.loggedUser = u;
      ctx.userInfo = ctx.database.GetUserInfo();
      ctx.cheats = ctx.database.GetCheats();
      ctx.serverStatus = ctx.database.GetServerStatus();
      ctx.logs.clear();
      ctx.logs.push_back("[OK] Login successful");
      ctx.logs.push_back("[OK] Cheats loaded");
      ctx.requestScreen = AppScreen::LOADER;
      ctx.fadeAlpha = 0.0f;
      if (ctx.keepLoggedIn)
        SessionSave(m_username, m_password);
      memset(m_password, 0, sizeof(m_password));
    } else {
      m_errorMsg = ctx.database.GetLastError().c_str();
      m_errorTimer = 4.0f;
      memset(m_password, 0, sizeof(m_password));
    }
  }

  // ── Error Message ─────────────────────────────────────────────────────
  if (m_errorTimer > 0.0f && !m_errorMsg.empty()) {
    ImGui::SetCursorPosX(relX);
    ImGui::Dummy(ImVec2(0, 12));
    ImGui::SetCursorPosX(relX);
    float alpha = (m_errorTimer < 1.0f) ? m_errorTimer : 1.0f;

    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(DS::V4_ERROR.x, DS::V4_ERROR.y,
                                                DS::V4_ERROR.z, alpha));
    ImGui::TextWrapped("%s", m_errorMsg.c_str());
    ImGui::PopStyleColor();
    if (FONT_SMALL)
      ImGui::PopFont();

    // HWID reset button
    if (m_errorMsg.find("HWID") != std::string::npos) {
      ImGui::SetCursorPosX(relX);
      ImGui::Dummy(ImVec2(0, 6));
      ImGui::SetCursorPosX(relX);

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, DS::ROUND_MD);
      ImGui::PushStyleColor(ImGuiCol_Button, DS::V4_BG_CARD);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DS::V4_BG_HOVER);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, DS::V4_BG_INPUT);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.65f, 1.0f, alpha));

      if (FONT_SMALL)
        ImGui::PushFont(FONT_SMALL);
      if (ImGui::Button("Request HWID Reset", ImVec2(formContentW, 34))) {
        std::string u(m_username), p(m_password);
        if (!u.empty()) {
          if (ctx.database.RequestHwidResetByCreds(
                  u, p, "HWID mismatch from login")) {
            m_errorMsg = "Reset requested! Wait for admin approval.";
            m_errorTimer = 6.0f;
          } else {
            m_errorMsg = ctx.database.GetLastError();
            m_errorTimer = 5.0f;
          }
        }
      }
      if (FONT_SMALL)
        ImGui::PopFont();

      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar();
    }
  }

  // ── Connection status indicator ───────────────────────────────────────
  {
    float dotR = 4.0f;
    float footY = dp.y - 20.0f;
    float footX = formW - 16.0f;
    ImU32 dotCol = ctx.dbConnected ? DS::SUCCESS : DS::WARNING;
    ImVec2 dotScreen(splitX + footX, footY);
    fDl->AddCircleFilled(dotScreen, dotR, dotCol, 12);
    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    const char *statusTxt = ctx.dbConnected ? "online" : "offline";
    ImVec2 tsz = ImGui::CalcTextSize(statusTxt);
    fDl->AddText(ImVec2(dotScreen.x - tsz.x - 8, footY - tsz.y * 0.5f),
                 DS::WithAlpha(DS::TEXT_MUTED, 120), statusTxt);
    if (FONT_SMALL)
      ImGui::PopFont();
  }

  DrawWindowControls(fDl, dp);

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}
