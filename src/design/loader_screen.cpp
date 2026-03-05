
#include "loader_screen.h"
#include "app.h"
#include "design_system.h"
#include "fonts.h"
#include "injector.h"
#include "logo.h"
#include "session.h"
#include "strings.h"
#include "ui_controls.h"

#include "imgui.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>

// ── Helpers ─────────────────────────────────────────────────────────────────

static ImU32 HexToColor(const std::string &hex, int alpha = 255) {
  if (hex.size() < 7)
    return IM_COL32(0, 204, 106, alpha);
  int r = 0, g = 0, b = 0;
  sscanf(hex.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
  return IM_COL32(r, g, b, alpha);
}

static void GetGameInitials(const std::string &game, char out[4]) {
  out[0] = out[1] = out[2] = out[3] = 0;
  if (game.empty()) {
    out[0] = '?';
    return;
  }
  int idx = 0;
  bool newWord = true;
  for (size_t i = 0; i < game.size() && idx < 2; i++) {
    if (game[i] == ' ') {
      newWord = true;
      continue;
    }
    if (newWord) {
      out[idx++] = (char)toupper(game[i]);
      newWord = false;
    }
  }
  if (idx == 0)
    out[0] = '?';
}

// File-scope static for steam kill tracking (shared by Render + DrawTopBar)
static bool s_steamKilled = false;

LoaderScreen::LoaderScreen() {}

// ═════════════════════════════════════════════════════════════════════════════
// MAIN RENDER — Orchestrates the 3 panels
// ═════════════════════════════════════════════════════════════════════════════

void LoaderScreen::Render(AppContext &ctx) {
  const auto &S = GetStrings(ctx.language);
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 dp = io.DisplaySize;
  float time = (float)ImGui::GetTime();

  // Kill Steam on first render
  if (!s_steamKilled) {
    s_steamKilled = true;
    Injector::KillProcessByName(L"steam.exe");
    Injector::KillProcessByName(L"steamwebhelper.exe");
    {
      std::lock_guard<std::mutex> lock(ctx.logMutex);
      ctx.logs.push_back(S.steamClosed);
    }
  }

  // Auto-select first cheat
  if (m_selectedCheatIdx < 0 && !ctx.cheats.empty()) {
    m_selectedCheatIdx = 0;
    for (int i = 0; i < (int)ctx.cheats.size(); i++) {
      if (ctx.cheats[i].game.find("Counter Strike") != std::string::npos ||
          ctx.cheats[i].game.find("CS2") != std::string::npos) {
        m_selectedCheatIdx = i;
        break;
      }
    }
  }

  DrawTopBar(ctx, dp, time);
  DrawSidebar(ctx, dp, time);

  // ── Main content area ─────────────────────────────────────────────────
  float mainX = DS::SIDEBAR_W;
  float mainW = dp.x - DS::SIDEBAR_W;
  float contentY = DS::TOPBAR_H;
  float contentH = dp.y - DS::TOPBAR_H;

  ImGui::SetNextWindowPos(ImVec2(mainX, contentY));
  ImGui::SetNextWindowSize(ImVec2(mainW, contentH));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      ImVec2(DS::PAD_MD, DS::PAD_MD));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, DS::V4_BG_BASE);
  ImGui::Begin("##content", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav);

  ImVec2 wPos = ImGui::GetWindowPos();
  DrawGameContent(ctx, wPos, mainW, contentH, time);

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

// ═════════════════════════════════════════════════════════════════════════════
// TOP BAR
// ═════════════════════════════════════════════════════════════════════════════

void LoaderScreen::DrawTopBar(AppContext &ctx, ImVec2 dp, float time) {
  const auto &S = GetStrings(ctx.language);

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(dp.x, DS::TOPBAR_H));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(DS::PAD_MD, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(0.047f, 0.047f, 0.086f, 1.0f));
  ImGui::Begin("##topbar", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImDrawList *dl = ImGui::GetWindowDrawList();

  // Logo + Brand
  if (ctx.logoTexture) {
    ImGui::SetCursorPos(ImVec2(10, 7));
    ImGui::Image(ctx.logoTexture, ImVec2(36, 36));
  }

  ImGui::SetCursorPos(ImVec2(50, 8));
  if (FONT_BOLD)
    ImGui::PushFont(FONT_BOLD);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT);
  ImGui::Text("BR");
  ImGui::PopStyleColor();
  if (FONT_BOLD)
    ImGui::PopFont();

  ImGui::SameLine(0, 0);
  ImGui::SetCursorPosY(8);
  if (FONT_BOLD)
    ImGui::PushFont(FONT_BOLD);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_ACCENT);
  ImGui::Text("Sense");
  ImGui::PopStyleColor();
  if (FONT_BOLD)
    ImGui::PopFont();

  ImGui::SameLine(0, 6);
  ImGui::SetCursorPosY(14);
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT_MUTED);
  ImGui::Text("Alpha");
  ImGui::PopStyleColor();
  if (FONT_SMALL)
    ImGui::PopFont();

  // ── Right side: User + Logout ─────────────────────────────────────────
  {
    float logoutW = 80;
    float rightEdge = dp.x - 100;

    // User avatar dot + name
    ImGui::SetCursorPos(ImVec2(rightEdge - logoutW - 120, 10));
    ImVec2 avatarPos = ImGui::GetCursorScreenPos();
    dl->AddCircleFilled(ImVec2(avatarPos.x, avatarPos.y + 16), 14,
                        DS::WithAlpha(DS::ACCENT, 30), 16);
    dl->AddCircleFilled(ImVec2(avatarPos.x, avatarPos.y + 12), 5,
                        DS::WithAlpha(DS::ACCENT, 200), 12);

    ImGui::SetCursorPos(ImVec2(rightEdge - logoutW - 100, 14));
    if (FONT_REGULAR)
      ImGui::PushFont(FONT_REGULAR);
    ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT_SEC);
    ImGui::Text("%s", ctx.userInfo.nickname.c_str());
    ImGui::PopStyleColor();
    if (FONT_REGULAR)
      ImGui::PopFont();

    // Logout button
    ImGui::SameLine();
    ImGui::SetCursorPosY(10);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, DS::ROUND_MD);
    ImGui::PushStyleColor(ImGuiCol_Button, DS::V4_BG_CARD);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DS::V4_BG_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, DS::V4_BG_INPUT);
    ImGui::PushStyleColor(ImGuiCol_Text, DS::V4_TEXT_SEC);

    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    if (ImGui::Button(S.logout, ImVec2(logoutW, 30))) {
      ctx.requestScreen = AppScreen::LOGIN;
      SessionClear();
      ctx.loggedUser.clear();
      ctx.userInfo = {};
      ctx.cheats.clear();
      ctx.logs.clear();
      ctx.keepLoggedIn = false;
      ctx.fadeAlpha = 0.0f;
      s_steamKilled = false;
    }
    if (FONT_SMALL)
      ImGui::PopFont();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
  }

  DrawLangFlags(dl, ctx.language, 175, 18);
  DrawWindowControls(dl, dp);
  HandleDrag(DS::TOPBAR_H);

  // Bottom border line
  dl->AddLine(ImVec2(0, DS::TOPBAR_H - 1), ImVec2(dp.x, DS::TOPBAR_H - 1),
              DS::WithAlpha(DS::ACCENT, 25));

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

// ═════════════════════════════════════════════════════════════════════════════
// SIDEBAR
// ═════════════════════════════════════════════════════════════════════════════

void LoaderScreen::DrawSidebar(AppContext &ctx, ImVec2 dp, float time) {
  float contentY = DS::TOPBAR_H;
  float contentH = dp.y - DS::TOPBAR_H;

  ImGui::SetNextWindowPos(ImVec2(0, contentY));
  ImGui::SetNextWindowSize(ImVec2(DS::SIDEBAR_W, contentH));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, DS::PAD_MD));
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(0.039f, 0.039f, 0.071f, 1.0f));
  ImGui::Begin("##sidebar", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav);

  ImDrawList *dl = ImGui::GetWindowDrawList();

  // Right border
  dl->AddLine(ImVec2(DS::SIDEBAR_W - 1, contentY),
              ImVec2(DS::SIDEBAR_W - 1, contentY + contentH), DS::BORDER);

  float iconSize = 38.0f;
  float iconPad = 10.0f;
  float iconStartY = contentY + 16;

  for (int i = 0; i < (int)ctx.cheats.size(); i++) {
    const auto &ch = ctx.cheats[i];
    bool selected = (i == m_selectedCheatIdx);
    ImU32 iconCol = HexToColor(ch.iconColor, selected ? 255 : 100);

    float cx = DS::SIDEBAR_W * 0.5f;
    float cy = iconStartY + i * (iconSize + iconPad) + iconSize * 0.5f;

    // Selected indicator
    if (selected) {
      dl->AddRectFilled(ImVec2(0, cy - 2), ImVec2(3, cy + 20), DS::ACCENT,
                        2.0f);
      dl->AddRectFilled(ImVec2(6, cy - iconSize * 0.5f - 2),
                        ImVec2(DS::SIDEBAR_W - 6, cy + iconSize * 0.5f + 2),
                        DS::WithAlpha(DS::ACCENT, 18), DS::ROUND_MD);
    }

    // Icon circle
    dl->AddCircleFilled(ImVec2(cx, cy), iconSize * 0.42f, iconCol, 20);
    dl->AddCircle(ImVec2(cx, cy), iconSize * 0.42f,
                  DS::WithAlpha(0xFFFFFFFF, selected ? 50 : 15), 20, 1.5f);

    // Game initials
    char initials[4];
    GetGameInitials(ch.game, initials);
    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    ImVec2 textSz = ImGui::CalcTextSize(initials);
    dl->AddText(ImVec2(cx - textSz.x * 0.5f, cy - textSz.y * 0.5f),
                IM_COL32(255, 255, 255, selected ? 240 : 140), initials);
    if (FONT_SMALL)
      ImGui::PopFont();

    // Invisible button for click
    ImGui::SetCursorScreenPos(ImVec2(2, cy - iconSize * 0.5f - 2));
    char btnId[32];
    snprintf(btnId, sizeof(btnId), "##game_%d", i);
    if (ImGui::InvisibleButton(btnId, ImVec2(DS::SIDEBAR_W - 4, iconSize + 4)))
      m_selectedCheatIdx = i;

    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", ch.game.c_str());
  }

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

// ═════════════════════════════════════════════════════════════════════════════
// GAME CONTENT
// ═════════════════════════════════════════════════════════════════════════════

void LoaderScreen::DrawGameContent(AppContext &ctx, ImVec2 wPos, float mainW,
                                   float contentH, float time) {
  const auto &S = GetStrings(ctx.language);
  ImDrawList *dl = ImGui::GetWindowDrawList();
  float pad = DS::PAD_MD;

  if (ctx.cheats.empty()) {
    if (FONT_REGULAR)
      ImGui::PushFont(FONT_REGULAR);
    dl->AddText(ImVec2(wPos.x + pad + 20, wPos.y + 50), DS::TEXT_MUTED,
                S.noAccess);
    if (FONT_REGULAR)
      ImGui::PopFont();
    return;
  }

  if (m_selectedCheatIdx < 0 || m_selectedCheatIdx >= (int)ctx.cheats.size())
    return;

  const auto &ch = ctx.cheats[m_selectedCheatIdx];
  bool isExpired = (ch.status == "expired");
  bool isLifetime = (ch.timeLeft == "LIFETIME");

  float notesW = 230.0f;
  float centerW = mainW - notesW - pad * 3;
  float centerX = wPos.x + pad;
  float notesX = wPos.x + mainW - notesW - pad;
  float curY = wPos.y + pad + 4;

  // ── Game Banner Card ──────────────────────────────────────────────────
  ImVec2 bannerMin(centerX, curY);
  ImVec2 bannerMax(centerX + centerW, curY + 100);
  ImU32 bannerCol = HexToColor(ch.iconColor, 15);
  DS::DrawVGradient(dl, bannerMin, bannerMax, bannerCol, DS::BG_BASE);
  dl->AddRect(bannerMin, bannerMax, HexToColor(ch.iconColor, 30), DS::ROUND_LG,
              0, 1.0f);

  // Game icon
  float bigIconR = 22.0f;
  float bigCx = centerX + 36;
  float bigCy = curY + 50;
  dl->AddCircleFilled(ImVec2(bigCx, bigCy), bigIconR,
                      HexToColor(ch.iconColor, 160), 24);
  dl->AddCircle(ImVec2(bigCx, bigCy), bigIconR, HexToColor(ch.iconColor, 50),
                24, 2.0f);

  char initials[4];
  GetGameInitials(ch.game, initials);
  if (FONT_SEMIBOLD)
    ImGui::PushFont(FONT_SEMIBOLD);
  ImVec2 iniSz = ImGui::CalcTextSize(initials);
  dl->AddText(ImVec2(bigCx - iniSz.x * 0.5f, bigCy - iniSz.y * 0.5f),
              IM_COL32(255, 255, 255, 230), initials);
  if (FONT_SEMIBOLD)
    ImGui::PopFont();

  // Game name
  if (FONT_BOLD)
    ImGui::PushFont(FONT_BOLD);
  dl->AddText(ImVec2(centerX + 72, curY + 18), DS::TEXT_PRIMARY,
              ch.game.c_str());
  if (FONT_BOLD)
    ImGui::PopFont();

  // Cheat name
  if (FONT_REGULAR)
    ImGui::PushFont(FONT_REGULAR);
  dl->AddText(ImVec2(centerX + 72, curY + 50), DS::TEXT_SECONDARY,
              ch.name.c_str());
  if (FONT_REGULAR)
    ImGui::PopFont();

  // Status badge
  {
    const char *statusText = ch.timeLeft.c_str();
    ImU32 badgeText = DS::WARNING;
    ImU32 badgeBg = DS::WARNING_BG;
    if (isLifetime) {
      statusText = S.lifetime;
      badgeText = DS::SUCCESS;
      badgeBg = DS::SUCCESS_BG;
    } else if (isExpired) {
      statusText = S.expired;
      badgeText = DS::ERROR_COL;
      badgeBg = DS::ERROR_BG;
    }
    DS::DrawBadge(dl, ImVec2(centerX + 72, curY + 74), statusText, badgeText,
                  badgeBg);
  }

  curY += 118;

  // Status dot
  {
    ImU32 dotCol = isExpired ? DS::ERROR_COL : DS::SUCCESS;
    const char *readyText =
        isExpired ? "Cheat license expired"
                  : (isLifetime ? "Ready to play" : "License active");
    dl->AddCircleFilled(ImVec2(centerX + 10, curY + 8), 4.0f, dotCol, 12);
    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    dl->AddText(ImVec2(centerX + 22, curY), DS::TEXT_SECONDARY, readyText);
    if (FONT_SMALL)
      ImGui::PopFont();
  }

  curY += 32;

  // ── Inject Button ─────────────────────────────────────────────────────
  float btnW = centerW - 8;
  ImGui::SetCursorScreenPos(ImVec2(centerX + 4, curY));
  if (isExpired)
    ImGui::BeginDisabled();

  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, DS::ROUND_LG);
  ImGui::PushStyleColor(ImGuiCol_Button, DS::V4_ACCENT);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, DS::V4_ACCENT_HOVER);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, DS::V4_ACCENT_ACTIVE);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

  if (FONT_SEMIBOLD)
    ImGui::PushFont(FONT_SEMIBOLD);
  bool clicked = ImGui::Button(ctx.isInjecting ? S.injectingText : S.injectPlay,
                               ImVec2(btnW, 48));
  if (FONT_SEMIBOLD)
    ImGui::PopFont();

  // Hover glow
  if (ImGui::IsItemHovered() && !isExpired) {
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    DS::DrawGlowRect(dl, mn, mx, DS::ACCENT, 10.0f, DS::ROUND_LG);
  }

  // Spinner overlay when injecting
  if (ctx.isInjecting) {
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    float spinCx = mn.x + 30;
    float spinCy = (mn.y + mx.y) * 0.5f;
    DS::DrawSpinner(dl, ImVec2(spinCx, spinCy), 10.0f, time,
                    IM_COL32(255, 255, 255, 200), 2.0f);
  }

  ImGui::PopStyleColor(4);
  ImGui::PopStyleVar();

  if (isExpired)
    ImGui::EndDisabled();

  // ── Injection logic ───────────────────────────────────────────────────
  if (clicked) {
    if (ctx.isInjecting) {
      std::lock_guard<std::mutex> lock(ctx.logMutex);
      ctx.logs.push_back("[INFO] Injection already in progress...");
    } else {
      ctx.isInjecting = true;
      auto &db = ctx.database;
      int selIdx = m_selectedCheatIdx;

      std::thread([&ctx, &db, selIdx]() {
        const auto &ch = ctx.cheats[selIdx];
        {
          std::lock_guard<std::mutex> lock(ctx.logMutex);
          char buf[256];
          snprintf(buf, sizeof(buf), "[LOAD] Downloading payload for '%s'...",
                   ch.name.c_str());
          ctx.logs.push_back(buf);
        }

        std::vector<uint8_t> payload;
        bool dlOk = db.DownloadCheat(ch.id, payload);

        if (dlOk && !payload.empty()) {
          {
            std::lock_guard<std::mutex> lock(ctx.logMutex);
            char buf[256];
            snprintf(buf, sizeof(buf), "[OK] Payload received (%zu bytes)",
                     payload.size());
            ctx.logs.push_back(buf);
          }

          std::wstring target;
          if (!ch.process.empty()) {
            target = std::wstring(ch.process.begin(), ch.process.end());
          } else {
            std::lock_guard<std::mutex> lock(ctx.logMutex);
            ctx.logs.push_back("[ERR] No target process configured.");
            ctx.isInjecting = false;
            return;
          }

          // Kill configured processes
          if (!ch.killProcesses.empty()) {
            std::string procs = ch.killProcesses;
            size_t pos = 0;
            while ((pos = procs.find(',')) != std::string::npos) {
              std::string p = procs.substr(0, pos);
              while (!p.empty() && p.front() == ' ')
                p.erase(p.begin());
              while (!p.empty() && p.back() == ' ')
                p.pop_back();
              if (!p.empty())
                Injector::KillProcessByName(std::wstring(p.begin(), p.end()));
              procs.erase(0, pos + 1);
            }
            while (!procs.empty() && procs.front() == ' ')
              procs.erase(procs.begin());
            while (!procs.empty() && procs.back() == ' ')
              procs.pop_back();
            if (!procs.empty())
              Injector::KillProcessByName(
                  std::wstring(procs.begin(), procs.end()));
            {
              std::lock_guard<std::mutex> lock(ctx.logMutex);
              ctx.logs.push_back("[INFO] Cleaned processes per config.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          }

          {
            std::lock_guard<std::mutex> lock(ctx.logMutex);
            char buf[256];
            snprintf(buf, sizeof(buf), "[INFO] Waiting for process: %s",
                     ch.process.c_str());
            ctx.logs.push_back(buf);
          }

          bool injected = false, found = false;
          auto HasMainWindow = [](DWORD pid) -> bool {
            struct D {
              DWORD pid;
              HWND hwnd;
            };
            D d{pid, NULL};
            EnumWindows(
                [](HWND hwnd, LPARAM lp) -> BOOL {
                  D *p = (D *)lp;
                  DWORD pid = 0;
                  GetWindowThreadProcessId(hwnd, &pid);
                  if (pid == p->pid && !GetWindow(hwnd, GW_OWNER) &&
                      IsWindowVisible(hwnd)) {
                    p->hwnd = hwnd;
                    return FALSE;
                  }
                  return TRUE;
                },
                (LPARAM)&d);
            return d.hwnd != NULL;
          };

          for (int i = 0; i < 180; ++i) {
            DWORD pid = Injector::GetProcessIdByName(target);
            if (pid != 0) {
              if (!found) {
                found = true;
                std::lock_guard<std::mutex> lock(ctx.logMutex);
                ctx.logs.push_back(
                    "[INFO] Process found! Waiting for main window...");
              }
              if (HasMainWindow(pid)) {
                {
                  std::lock_guard<std::mutex> lock(ctx.logMutex);
                  ctx.logs.push_back("[INFO] Game window ready. Injecting...");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                if (Injector::ManualMap(target, payload)) {
                  injected = true;
                  std::lock_guard<std::mutex> lock(ctx.logMutex);
                  ctx.logs.push_back("[OK] Payload injected successfully!");
                } else {
                  std::lock_guard<std::mutex> lock(ctx.logMutex);
                  ctx.logs.push_back(std::string("[ERR] Injection failed: ") +
                                     Injector::GetLastError());
                }
                break;
              }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          }
          if (!injected && !found) {
            std::lock_guard<std::mutex> lock(ctx.logMutex);
            ctx.logs.push_back(
                "[ERR] Timeout: game did not start within 3 minutes.");
          }
          SecureZeroMemory(payload.data(), payload.size());
          payload.clear();
          if (injected) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ctx.shouldExit = true;
          }
        } else {
          std::lock_guard<std::mutex> lock(ctx.logMutex);
          ctx.logs.push_back(std::string("[ERR] Download failed: ") +
                             ctx.database.GetLastError());
        }
        ctx.isInjecting = false;
      }).detach();
    }
  }

  curY += 58;

  // ── Activity Log ──────────────────────────────────────────────────────
  float logTop = curY;
  float logBottom = wPos.y + contentH - pad;
  if (logBottom - logTop > 30) {
    DrawActivityLog(ctx, dl, ImVec2(centerX, logTop),
                    ImVec2(centerX + centerW, logBottom));
  }

  // ── Cheat Info Panel ──────────────────────────────────────────────────
  float rightTop = wPos.y + pad + 4;
  float rightBottom = wPos.y + contentH - pad;
  DrawCheatInfoPanel(ctx, dl, ImVec2(notesX, rightTop),
                     ImVec2(notesX + notesW, rightBottom));
}

// ═════════════════════════════════════════════════════════════════════════════
// ACTIVITY LOG
// ═════════════════════════════════════════════════════════════════════════════

void LoaderScreen::DrawActivityLog(AppContext &ctx, ImDrawList *dl,
                                   ImVec2 logMin, ImVec2 logMax) {
  const auto &S = GetStrings(ctx.language);
  float logH = logMax.y - logMin.y;

  DS::DrawCard(dl, logMin, logMax, DS::BG_DARKEST, DS::ROUND_LG);

  // Header
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  dl->AddText(ImVec2(logMin.x + 14, logMin.y + 10), DS::TEXT_PRIMARY,
              S.activityLog);
  if (FONT_SMALL)
    ImGui::PopFont();

  dl->AddLine(ImVec2(logMin.x + 10, logMin.y + 28),
              ImVec2(logMax.x - 10, logMin.y + 28), DS::DIVIDER);

  // Log entries
  float lineY = logMin.y + 36;
  std::lock_guard<std::mutex> lock(ctx.logMutex);

  int maxLines = (int)((logH - 44) / 17);
  int startIdx = (int)ctx.logs.size() - maxLines;
  if (startIdx < 0)
    startIdx = 0;

  if (FONT_MONO)
    ImGui::PushFont(FONT_MONO);
  for (size_t i = startIdx; i < ctx.logs.size(); i++) {
    if (lineY + 14 > logMax.y - 6)
      break;
    const auto &line = ctx.logs[i];
    ImU32 col = DS::TEXT_MUTED;
    if (line.find("[OK]") != std::string::npos)
      col = DS::SUCCESS;
    else if (line.find("[LOAD]") != std::string::npos)
      col = DS::INFO;
    else if (line.find("[ERR]") != std::string::npos)
      col = DS::ERROR_COL;
    else if (line.find("[INFO]") != std::string::npos)
      col = DS::WARNING;
    else if (line.find("[AUTO]") != std::string::npos)
      col = DS::ACCENT;
    dl->AddText(ImVec2(logMin.x + 14, lineY), col, line.c_str());
    lineY += 17;
  }
  if (FONT_MONO)
    ImGui::PopFont();
}

// ═════════════════════════════════════════════════════════════════════════════
// CHEAT INFO PANEL
// ═════════════════════════════════════════════════════════════════════════════

void LoaderScreen::DrawCheatInfoPanel(AppContext &ctx, ImDrawList *dl,
                                      ImVec2 pMin, ImVec2 pMax) {
  const auto &S = GetStrings(ctx.language);

  if (m_selectedCheatIdx < 0 || m_selectedCheatIdx >= (int)ctx.cheats.size())
    return;
  const auto &ch = ctx.cheats[m_selectedCheatIdx];

  DS::DrawCard(dl, pMin, pMax, DS::BG_DARKEST, DS::ROUND_LG);

  // Header
  if (FONT_SMALL)
    ImGui::PushFont(FONT_SMALL);
  dl->AddText(ImVec2(pMin.x + 14, pMin.y + 12), DS::WARNING, S.cheatInfo);
  if (FONT_SMALL)
    ImGui::PopFont();

  dl->AddLine(ImVec2(pMin.x + 10, pMin.y + 32),
              ImVec2(pMax.x - 10, pMin.y + 32), DS::DIVIDER);

  float noteY = pMin.y + 42;
  float noteMaxW = (pMax.x - pMin.x) - 28;

  if (!ch.notes.empty()) {
    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    std::string notesStr = ch.notes;
    size_t lineStart = 0;
    while (lineStart < notesStr.size() && noteY < pMax.y - 30) {
      size_t lineEnd = notesStr.find('\n', lineStart);
      if (lineEnd == std::string::npos)
        lineEnd = notesStr.size();
      std::string line = notesStr.substr(lineStart, lineEnd - lineStart);
      if (!line.empty()) {
        ImVec2 lineSz = ImGui::CalcTextSize(line.c_str());
        if (lineSz.x <= noteMaxW) {
          dl->AddText(ImVec2(pMin.x + 14, noteY), DS::TEXT_SECONDARY,
                      line.c_str());
          noteY += 18;
        } else {
          dl->AddText(nullptr, 0.0f, ImVec2(pMin.x + 14, noteY),
                      DS::TEXT_SECONDARY, line.c_str(), nullptr, noteMaxW);
          float wrappedH =
              ImGui::CalcTextSize(line.c_str(), nullptr, false, noteMaxW).y;
          noteY += wrappedH + 4;
        }
      } else {
        noteY += 10;
      }
      lineStart = lineEnd + 1;
    }
    if (FONT_SMALL)
      ImGui::PopFont();
  } else {
    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    dl->AddText(ImVec2(pMin.x + 14, noteY), DS::TEXT_MUTED, S.noNotes);
    if (FONT_SMALL)
      ImGui::PopFont();
  }

  // Requires admin footer
  if (ch.requiresAdmin) {
    float footY = pMax.y - 24;
    dl->AddLine(ImVec2(pMin.x + 10, footY - 8), ImVec2(pMax.x - 10, footY - 8),
                DS::DIVIDER);
    if (FONT_SMALL)
      ImGui::PushFont(FONT_SMALL);
    dl->AddText(ImVec2(pMin.x + 14, footY), DS::WARNING, S.requiresAdmin);
    if (FONT_SMALL)
      ImGui::PopFont();
  }
}
