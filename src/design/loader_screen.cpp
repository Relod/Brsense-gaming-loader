// =============================================================================
// loader_screen.cpp — Tela do Loader de Cheats (Redesign)
// =============================================================================
// Layout baseado nas referencias visuais:
//   TOP BAR: Logo + titulo + bandeiras + usuario + logout + controles
//   ESQUERDA (55%): Lista de cheats licenciados (cards por jogo)
//   DIREITA  (45%): User Info + Server Status + Log Console
//   BOTTOM: Botoes LOAD + UPDATE
// =============================================================================

#include "loader_screen.h"
#include "app.h"
#include "injector.h"
#include "logo.h"
#include "session.h"
#include "strings.h"
#include "ui_controls.h"

#include "imgui.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>

// ── Helpers ─────────────────────────────────────────────────────────────────

/// Converte cor hex (#RRGGBB) para ImU32
static ImU32 HexToColor(const std::string &hex, int alpha = 255) {
  if (hex.size() < 7)
    return IM_COL32(100, 100, 180, alpha);
  int r = 0, g = 0, b = 0;
  sscanf(hex.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
  return IM_COL32(r, g, b, alpha);
}

LoaderScreen::LoaderScreen() {}

void LoaderScreen::Render(AppContext &ctx) {
  const auto &S = GetStrings(ctx.language);
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 dp = io.DisplaySize;
  float time = (float)ImGui::GetTime();

  float topBarH = 50.0f;
  float bottomBarH = 70.0f;
  float pad = 16.0f;

  // ═════════════════════════════════════════════════════════════════════════
  // TOP BAR — Barra de navegacao superior
  // ═════════════════════════════════════════════════════════════════════════

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(dp.x, topBarH));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.11f, 1.0f));
  ImGui::Begin("##topbar", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImDrawList *tbDl = ImGui::GetWindowDrawList();

  // ── Logo Anubis mini + "BR Sense" ─────────────────────────────────────
  {
    DrawAnubisLogo(tbDl, 28, 25, 22.0f, time, 200);

    ImGui::SetCursorPos(ImVec2(48, 10));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.61f, 0.23f, 1.0f));
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("B");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 0);
    ImGui::SetCursorPosY(10);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.87f, 0.0f, 1.0f));
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("R");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosY(10);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.94f, 1.0f));
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("Sense");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Tag "Alpha Version"
    ImGui::SameLine();
    ImGui::SetCursorPosY(14);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.28f, 0.47f, 0.7f));
    ImGui::Text("Alpha");
    ImGui::PopStyleColor();
  }

  // ── Bandeiras apos o titulo ───────────────────────────────────────────
  DrawLangFlags(tbDl, ctx.language, 200, 18);

  // ── Nome do usuario + Logout ──────────────────────────────────────────
  {
    float logoutW = 70;
    float rightEdge = dp.x - 120;

    ImGui::SetCursorPos(ImVec2(rightEdge - logoutW - 80, 10));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.80f, 1.0f));

    // Icone de usuario (circulo)
    ImVec2 uPos = ImGui::GetCursorScreenPos();
    tbDl->AddCircleFilled(ImVec2(uPos.x - 18, uPos.y + 14), 10,
                          IM_COL32(235, 72, 120, 100), 16);
    tbDl->AddCircleFilled(ImVec2(uPos.x - 18, uPos.y + 10), 4,
                          IM_COL32(235, 72, 120, 180), 12);

    ImGui::Text("%s", ctx.userInfo.nickname.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosY(10);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.19f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.92f, 0.28f, 0.47f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.92f, 0.28f, 0.47f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.80f, 1.0f));

    if (ImGui::Button(S.logout, ImVec2(logoutW, 28))) {
      ctx.requestScreen = AppScreen::LOGIN;
      SessionClear();
      ctx.loggedUser.clear();
      ctx.userInfo = {};
      ctx.cheats.clear();
      ctx.logs.clear();
      ctx.keepLoggedIn = false;
      ctx.fadeAlpha = 0.0f;
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
  }

  // ── Controles de janela ───────────────────────────────────────────────
  DrawWindowControls(tbDl, dp);
  HandleDrag(topBarH);

  // Linha inferior da topbar
  tbDl->AddLine(ImVec2(0, topBarH - 1), ImVec2(dp.x, topBarH - 1),
                IM_COL32(235, 72, 120, 30));

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  // ═════════════════════════════════════════════════════════════════════════
  // CONTEUDO PRINCIPAL — Area do loader
  // ═════════════════════════════════════════════════════════════════════════

  float contentY = topBarH;
  float contentH = dp.y - topBarH;

  ImGui::SetNextWindowPos(ImVec2(0, contentY));
  ImGui::SetNextWindowSize(ImVec2(dp.x, contentH));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, pad));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 1.0f));
  ImGui::Begin("##content", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav);

  ImDrawList *cDl = ImGui::GetWindowDrawList();
  ImVec2 wPos = ImGui::GetWindowPos();

  float totalW = dp.x - pad * 2;
  float leftW = totalW * 0.55f;
  float rightW = totalW - leftW - pad;
  float panelH = contentH - pad * 2 - bottomBarH;

  // ═════════════════════════════════════════════════════════════════════════
  // LADO ESQUERDO — Interface de Seleção Melhorada (Combo Box + Injetar)
  // ═════════════════════════════════════════════════════════════════════════
  {
    ImVec2 lMin(wPos.x + pad, wPos.y + pad);
    ImVec2 lMax(lMin.x + leftW, lMin.y + panelH);

    // Titulo "Your Cheats"
    cDl->AddRectFilled(lMin, ImVec2(lMax.x, lMin.y + 36),
                       IM_COL32(18, 18, 26, 255), 8.0f,
                       ImDrawFlags_RoundCornersTop);
    cDl->AddText(ImVec2(lMin.x + 16, lMin.y + 10), IM_COL32(230, 230, 240, 240),
                 S.yourCheats);

    // Fundo da area de selecao
    cDl->AddRectFilled(ImVec2(lMin.x, lMin.y + 36), lMax,
                       IM_COL32(16, 16, 22, 255), 8.0f,
                       ImDrawFlags_RoundCornersBottom);
    cDl->AddRect(lMin, lMax, IM_COL32(50, 50, 65, 100), 8.0f, 0, 1.0f);

    float formY = lMin.y + 56;
    float formX = lMin.x + 20;

    if (ctx.cheats.empty()) {
      cDl->AddText(ImVec2(formX, formY), IM_COL32(120, 120, 140, 180), S.noAccess);
    } else {
      // 1. Label
      cDl->AddText(ImVec2(formX, formY), IM_COL32(180, 180, 200, 255), "Select Game to Inject:");
      formY += 24;

      // 2. Combo box setup
      std::vector<const char*> items;
      for (const auto& ch : ctx.cheats) {
        items.push_back(ch.name.c_str());
      }

      ImGui::SetCursorScreenPos(ImVec2(formX, formY));
      ImGui::PushItemWidth(leftW - 40);
      ImGui::Combo("##CheatSelect", &m_selectedCheatIdx, items.data(), items.size());
      ImGui::PopItemWidth();

      formY += 40;

      // 3. Exibir informacoes do cheat selecionado curante
      if (m_selectedCheatIdx >= 0 && m_selectedCheatIdx < ctx.cheats.size()) {
          const auto& ch = ctx.cheats[m_selectedCheatIdx];
          bool isExpired = (ch.status == "expired");
          bool isLifetime = (ch.timeLeft == "LIFETIME");

          char infoBuf[256];
          snprintf(infoBuf, sizeof(infoBuf), "Game: %s", ch.game.c_str());
          cDl->AddText(ImVec2(formX, formY), IM_COL32(160, 160, 180, 255), infoBuf);
          formY += 24;

          const char* timeText = ch.timeLeft.c_str();
          ImU32 timeCol = IM_COL32(255, 200, 60, 255);
          if (isLifetime) {
              timeText = S.lifetime;
              timeCol = IM_COL32(80, 200, 120, 255);
          } else if (isExpired) {
              timeText = S.expired;
              timeCol = IM_COL32(220, 60, 60, 255);
          }
          
          snprintf(infoBuf, sizeof(infoBuf), "Status: %s", timeText);
          cDl->AddText(ImVec2(formX, formY), timeCol, infoBuf);
          formY += 40;

          // 4. Botao de Inject & Play
          ImGui::SetCursorScreenPos(ImVec2(formX, formY));
          if (isExpired) {
             ImGui::BeginDisabled();
          }

          if (ImGui::Button("INJECT AND PLAY", ImVec2(leftW - 40, 40))) {
              if (ctx.isInjecting) {
                  std::lock_guard<std::mutex> lock(ctx.logMutex);
                  ctx.logs.push_back("[ERR] An injection is already in progress.");
              } else {
                  ctx.isInjecting = true;
                  {
                      std::lock_guard<std::mutex> lock(ctx.logMutex);
                      ctx.logs.push_back("[LOAD] Starting payload download...");
                  }

                  std::thread([&ctx, ch]() {
                      // Baixar o cheat
                      std::vector<uint8_t> pPayloadBytes;
                      if (ctx.database.DownloadCheat(ch.id, pPayloadBytes, ch.hash)) {
                          {
                              std::lock_guard<std::mutex> lock(ctx.logMutex);
                              char logBuf[256];
                              snprintf(logBuf, sizeof(logBuf),
                                       "[OK] Payload received (%zu bytes).",
                                       pPayloadBytes.size());
                              ctx.logs.push_back(logBuf);
                              ctx.logs.push_back("[INFO] Auto-Starting Steam via Protocol...");
                          }

                          // Launch Steam protocol
                          std::wstring targetProcess = L"cs2.exe";
                          if (!ch.process.empty()) {
                              targetProcess = std::wstring(ch.process.begin(), ch.process.end());
                          } else if (ch.game == "Valorant") {
                              targetProcess = L"VALORANT-Win64-Shipping.exe";
                          }

                          // Matar possiveis processos antigos da Steam para lancar liso
                          Injector::KillProcessByName(targetProcess);
                          Injector::KillProcessByName(L"steam.exe");
                          
                          // Lancar CS2 via URL Protocol
                          ShellExecuteA(NULL, "open", "steam://run/730//-console -novid", NULL, NULL, SW_SHOWNORMAL);

                          bool injected = false;
                          bool found = false;

                          auto HasMainWindow = [](DWORD processId) -> bool {
                              struct EnumData {
                                  DWORD pid;
                                  HWND hwnd;
                              };
                              EnumData enumData = { processId, NULL };
                              EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                                  EnumData* pData = reinterpret_cast<EnumData*>(lParam);
                                  DWORD pid = 0;
                                  GetWindowThreadProcessId(hwnd, &pid);
                                  if (pid == pData->pid && GetWindow(hwnd, GW_OWNER) == (HWND)0 && IsWindowVisible(hwnd)) {
                                      pData->hwnd = hwnd;
                                      return FALSE;
                                  }
                                  return TRUE;
                              }, reinterpret_cast<LPARAM>(&enumData));
                              return enumData.hwnd != NULL;
                          };

                          for (int i = 0; i < 180; ++i) { // Espera ate 3 min pro jogo abrir
                              DWORD pid = Injector::GetProcessIdByName(targetProcess);
                              if (pid != 0) {
                                  if (!found) {
                                      found = true;
                                      std::lock_guard<std::mutex> lock(ctx.logMutex);
                                      ctx.logs.push_back("[INFO] Processo encontrado! Aguardando janela principal...");
                                  }

                                  if (HasMainWindow(pid)) {
                                      {
                                          std::lock_guard<std::mutex> lock(ctx.logMutex);
                                          ctx.logs.push_back("[INFO] Tela do jogo pronta. Injetando agora...");
                                      }
                                      std::this_thread::sleep_for(std::chrono::milliseconds(2500));

                                      if (Injector::ManualMap(targetProcess, pPayloadBytes)) {
                                          injected = true;
                                          std::lock_guard<std::mutex> lock(ctx.logMutex);
                                          ctx.logs.push_back("[OK] Payload Injetado com Sucesso!");
                                      } else {
                                          std::string err = Injector::GetLastError();
                                          std::lock_guard<std::mutex> lock(ctx.logMutex);
                                          ctx.logs.push_back(std::string("[ERR] Falha ao injetar payload: ") + err);
                                      }
                                      break; // Sai do loop
                                  }
                              }
                              std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                          }

                          if (!injected && !found) {
                              std::lock_guard<std::mutex> lock(ctx.logMutex);
                              ctx.logs.push_back("[ERR] Timeout: O jogo nao abriu.");
                          }

                          SecureZeroMemory(pPayloadBytes.data(), pPayloadBytes.size());
                          pPayloadBytes.clear();

                          if (injected) {
                              std::this_thread::sleep_for(std::chrono::seconds(3));
                              ctx.shouldExit = true; // SINALIZAR SAIDA DA APP
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

          if (isExpired) {
             ImGui::EndDisabled();
          }
      }
    }
  }

  // ═════════════════════════════════════════════════════════════════════════
  // LADO DIREITO — User Info + Server Status + Log Console
  // ═════════════════════════════════════════════════════════════════════════
  {
    float rx = wPos.x + pad + leftW + pad;
    float ry = wPos.y + pad;

    // ── Card: User Info ─────────────────────────────────────────────────
    float userCardH = 150;
    {
      ImVec2 uMin(rx, ry);
      ImVec2 uMax(rx + rightW, ry + userCardH);

      cDl->AddRectFilled(uMin, uMax, IM_COL32(18, 18, 26, 255), 8.0f);
      cDl->AddRect(uMin, uMax, IM_COL32(50, 50, 65, 100), 8.0f, 0, 1.0f);

      // Icone de usuario grande
      float iconX = uMin.x + 36, iconY = uMin.y + 40;
      cDl->AddCircleFilled(ImVec2(iconX, iconY), 24, IM_COL32(235, 72, 120, 60),
                           20);
      cDl->AddCircleFilled(ImVec2(iconX, iconY - 4), 8,
                           IM_COL32(235, 72, 120, 150), 16);
      cDl->AddCircle(ImVec2(iconX, iconY), 24, IM_COL32(235, 72, 120, 100), 20,
                     1.5f);

      float lx = uMin.x + 72;
      float ly = uMin.y + 10;
      float vx = lx + 90;

      // Nickname
      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.nickname);
      cDl->AddText(ImVec2(vx, ly), IM_COL32(240, 240, 245, 255),
                   ctx.userInfo.nickname.c_str());
      ly += 22;

      // HWID
      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.hwid);
      std::string hwid = ctx.userInfo.hwid;
      if (hwid.size() > 20)
        hwid = hwid.substr(0, 20) + "...";
      cDl->AddText(ImVec2(vx, ly), IM_COL32(200, 200, 210, 200), hwid.c_str());
      ly += 22;

      // MAC
      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.mac);
      cDl->AddText(ImVec2(vx, ly), IM_COL32(200, 200, 210, 200),
                   ctx.userInfo.mac.c_str());
      ly += 22;

      // IP
      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.ip);
      cDl->AddText(ImVec2(vx, ly), IM_COL32(200, 200, 210, 200),
                   ctx.userInfo.ip.c_str());
      ly += 22;

      // Plan
      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.plan);
      ImU32 planCol = IM_COL32(180, 180, 190, 255);
      if (ctx.userInfo.plan == "Lifetime")
        planCol = IM_COL32(80, 200, 120, 255);
      else if (ctx.userInfo.plan == "VIP")
        planCol = IM_COL32(255, 200, 60, 255);
      else if (ctx.userInfo.plan == "Premium")
        planCol = IM_COL32(100, 180, 255, 255);
      cDl->AddText(ImVec2(vx, ly), planCol, ctx.userInfo.plan.c_str());
    }

    // ── Card: Server Status ─────────────────────────────────────────────
    float serverCardH = 120;
    float sy = ry + userCardH + pad;
    {
      ImVec2 sMin(rx, sy);
      ImVec2 sMax(rx + rightW, sy + serverCardH);

      cDl->AddRectFilled(sMin, sMax, IM_COL32(18, 18, 26, 255), 8.0f);
      cDl->AddRect(sMin, sMax, IM_COL32(50, 50, 65, 100), 8.0f, 0, 1.0f);

      // Icone servidor
      float sIconX = sMin.x + 30, sIconY = sMin.y + 30;
      cDl->AddRectFilled(ImVec2(sIconX - 10, sIconY - 12),
                         ImVec2(sIconX + 10, sIconY + 12),
                         IM_COL32(80, 160, 240, 80), 3.0f);
      cDl->AddRect(ImVec2(sIconX - 10, sIconY - 12),
                   ImVec2(sIconX + 10, sIconY + 12),
                   IM_COL32(80, 160, 240, 150), 3.0f, 0, 1.0f);
      // Linhas do servidor
      cDl->AddLine(ImVec2(sIconX - 6, sIconY - 5),
                   ImVec2(sIconX + 6, sIconY - 5), IM_COL32(80, 160, 240, 200));
      cDl->AddLine(ImVec2(sIconX - 6, sIconY), ImVec2(sIconX + 6, sIconY),
                   IM_COL32(80, 160, 240, 200));
      cDl->AddLine(ImVec2(sIconX - 6, sIconY + 5),
                   ImVec2(sIconX + 6, sIconY + 5), IM_COL32(80, 160, 240, 200));

      float lx = sMin.x + 52;
      float ly = sMin.y + 12;
      float vx = lx + 110;

      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.region);
      cDl->AddText(ImVec2(vx, ly), IM_COL32(220, 220, 230, 255),
                   ctx.serverStatus.region.c_str());
      ly += 20;

      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200),
                   S.serverStatus);
      ImU32 statusCol = ctx.serverStatus.online ? IM_COL32(80, 200, 120, 255)
                                                : IM_COL32(220, 60, 60, 255);
      cDl->AddText(ImVec2(vx, ly), statusCol,
                   ctx.serverStatus.online ? S.online : S.offline);
      ly += 20;

      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.lastUpdate);
      cDl->AddText(ImVec2(vx, ly), IM_COL32(200, 200, 210, 200),
                   ctx.serverStatus.lastUpdate.c_str());
      ly += 20;

      char pingBuf[32];
      snprintf(pingBuf, sizeof(pingBuf), "%d ms", ctx.serverStatus.ping);
      cDl->AddText(ImVec2(lx, ly), IM_COL32(140, 140, 155, 200), S.ping);
      cDl->AddText(ImVec2(vx, ly), IM_COL32(80, 200, 120, 255), pingBuf);
    }

    // ── Card: Log Console ───────────────────────────────────────────────
    float logY = sy + serverCardH + pad;
    float logH = panelH - (logY - ry);
    if (logH < 60)
      logH = 60;
    {
      ImVec2 logMin(rx, logY);
      ImVec2 logMax(rx + rightW, logY + logH);

      cDl->AddRectFilled(logMin, logMax, IM_COL32(14, 14, 20, 255), 8.0f);
      cDl->AddRect(logMin, logMax, IM_COL32(50, 50, 65, 100), 8.0f, 0, 1.0f);

      // Titulo
      cDl->AddText(ImVec2(logMin.x + 12, logMin.y + 8),
                   IM_COL32(230, 230, 240, 240), S.activityLog);

      // Separador
      cDl->AddLine(ImVec2(logMin.x + 8, logMin.y + 28),
                   ImVec2(logMax.x - 8, logMin.y + 28),
                   IM_COL32(50, 50, 65, 120));

      // Linhas do log
      float lineY = logMin.y + 36;
      std::lock_guard<std::mutex> lock(ctx.logMutex);
      for (size_t i = 0; i < ctx.logs.size(); i++) {
        if (lineY + 16 > logMax.y - 4)
          break;

        const auto &logLine = ctx.logs[i];

        // Cor baseada no prefixo
        ImU32 logCol = IM_COL32(160, 160, 175, 200);
        if (logLine.find("[OK]") != std::string::npos)
          logCol = IM_COL32(80, 200, 120, 255);
        else if (logLine.find("[LOAD]") != std::string::npos)
          logCol = IM_COL32(100, 180, 255, 255);
        else if (logLine.find("[ERR]") != std::string::npos)
          logCol = IM_COL32(220, 60, 60, 255);
        else if (logLine.find("[AUTO]") != std::string::npos)
          logCol = IM_COL32(255, 200, 60, 255);

        cDl->AddText(ImVec2(logMin.x + 14, lineY), logCol, logLine.c_str());
        lineY += 18;
      }
    }
  }

  // ═════════════════════════════════════════════════════════════════════════
  // BOTTOM BAR — Botoes LOAD + UPDATE
  // ═════════════════════════════════════════════════════════════════════════
  {
    float btnAreaY = wPos.y + pad + panelH + 8;
    float btnW = (totalW - pad) * 0.5f;
    float btnH = 48;

    // ── Botao LOAD ──────────────────────────────────────────────────────
    ImGui::SetCursorScreenPos(ImVec2(wPos.x + pad, btnAreaY));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.92f, 0.28f, 0.47f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.96f, 0.35f, 0.52f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.82f, 0.22f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

    char loadLabel[64];
    snprintf(loadLabel, sizeof(loadLabel), ">> %s", S.load);
    if (ImGui::Button(loadLabel, ImVec2(btnW, btnH))) {
      std::lock_guard<std::mutex> lock(ctx.logMutex);
      ctx.logs.push_back("[INFO] Please select a specific game to load.");
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();

    // ── Botao UPDATE ────────────────────────────────────────────────────
    ImGui::SetCursorScreenPos(ImVec2(wPos.x + pad + btnW + pad, btnAreaY));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.20f, 0.20f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.12f, 0.12f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.85f, 1));

    char updateLabel[64];
    snprintf(updateLabel, sizeof(updateLabel), "C %s", S.update);
    if (ImGui::Button(updateLabel, ImVec2(btnW, btnH))) {
      ctx.serverStatus = ctx.database.GetServerStatus();
      std::lock_guard<std::mutex> lock(ctx.logMutex);
      ctx.logs.push_back("[OK] Server status updated");
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
  }

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}
