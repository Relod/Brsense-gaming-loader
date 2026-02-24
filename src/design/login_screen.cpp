// =============================================================================
// login_screen.cpp — Implementacao da Tela de Login
// =============================================================================
// Design split-screen:
//   ESQUERDA (40%):
//     - Gradiente azul-escuro → roxo
//     - Particulas rosa animadas (40 particulas)
//     - Logo Anubis geometrico (grande)
//     - Nome "BR Sense" (B verde, R amarelo, Sense branco)
//     - Tagline localizada
//     - Versao no canto inferior
//
//   DIREITA (60%):
//     - Fundo cinza medio
//     - Titulo "Bem-vindo" / "Welcome"
//     - Input de username (arredondado, borda rosa quando ativo)
//     - Input de password (arredondado, borda rosa quando ativo)
//     - Checkbox "Manter conectado"
//     - Link "Esqueceu sua senha?"
//     - Botao "ENTRAR" rosa gradiente
//     - Mensagem de erro com fade-out
//     - Indicador de banco (ca-0 / ca-1)
// =============================================================================

#include "login_screen.h"
#include "app.h"
#include "logo.h"
#include "session.h"
#include "strings.h"
#include "ui_controls.h"

#include "imgui.h"
#include <cstdlib>
#include <cstring>

LoginScreen::LoginScreen() {}

void LoginScreen::Render(AppContext &ctx) {
  const auto &S = GetStrings(ctx.language);
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 dp = io.DisplaySize;
  float time = (float)ImGui::GetTime();
  float dt = io.DeltaTime;

  // ── Atualizar timer de erro ───────────────────────────────────────────────
  if (m_errorTimer > 0.0f)
    m_errorTimer -= dt;

  // ── Inicializar particulas (uma unica vez) ────────────────────────────────
  if (!m_particlesInit) {
    m_particlesInit = true;
    srand(42);
    for (auto &p : m_particles) {
      p.x = (float)(rand() % (int)dp.x);
      p.y = (float)(rand() % (int)dp.y);
      p.vx = ((rand() % 100) - 50) * 0.15f;
      p.vy = ((rand() % 100) - 50) * 0.10f;
      p.r = 1.0f + (rand() % 30) * 0.1f;
      p.alpha = 30 + rand() % 50;
    }
  }

  // ═════════════════════════════════════════════════════════════════════════
  // FUNDO — Cobre toda a tela
  // ═════════════════════════════════════════════════════════════════════════

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(dp);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.12f, 1.0f));
  ImGui::Begin("##bg", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                   ImGuiWindowFlags_NoNav);

  ImDrawList *bgDl = ImGui::GetWindowDrawList();

  // ── Painel esquerdo: gradiente + branding ─────────────────────────────────
  float splitX = dp.x * 0.40f;

  // Gradiente do painel esquerdo (azul-escuro → roxo)
  bgDl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(splitX, dp.y),
                                IM_COL32(12, 12, 30, 255), // topo-esq
                                IM_COL32(20, 15, 40, 255), // topo-dir
                                IM_COL32(35, 18, 55, 255), // baixo-dir
                                IM_COL32(15, 10, 35, 255)  // baixo-esq
  );

  // Linha vertical sutil separando os paineis
  bgDl->AddLine(ImVec2(splitX, 0), ImVec2(splitX, dp.y),
                IM_COL32(235, 72, 120, 40), 1.0f);

  // ── Particulas animadas ───────────────────────────────────────────────────
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

    // Particulas no painel esquerdo: mais brilhantes
    int a = (p.x < splitX) ? p.alpha : p.alpha / 3;
    bgDl->AddCircleFilled(ImVec2(p.x, p.y), p.r, IM_COL32(235, 72, 120, a), 8);
  }

  // ── Logo Anubis + "BR Sense" ──────────────────────────────────────────────
  {
    float cx = splitX * 0.5f;
    float cy = dp.y * 0.35f;

    // Desenhar o Anubis geometrico (grande)
    DrawAnubisLogo(bgDl, cx, cy, 100.0f, time);

    // Nome "B" (verde) + "R" (amarelo) + " Sense" (branco) abaixo do logo
    float textY = cy + 70.0f;
    ImGui::SetWindowFontScale(2.5f);
    ImVec2 bSz = ImGui::CalcTextSize("B");
    ImVec2 rSz = ImGui::CalcTextSize("R");
    ImVec2 senseSz = ImGui::CalcTextSize(" Sense");
    float totalW = bSz.x + rSz.x + senseSz.x;
    float textX = cx - totalW * 0.5f;

    // "B" em verde bandeira
    bgDl->AddText(ImVec2(textX, textY), IM_COL32(0, 155, 58, 255), "B");
    // "R" em amarelo bandeira
    bgDl->AddText(ImVec2(textX + bSz.x, textY), IM_COL32(254, 223, 0, 255),
                  "R");
    // " Sense" em branco
    bgDl->AddText(ImVec2(textX + bSz.x + rSz.x, textY),
                  IM_COL32(230, 230, 240, 240), " Sense");
    ImGui::SetWindowFontScale(1.0f);

    // Tagline abaixo
    float tagY = textY + bSz.y + 8;
    ImVec2 tagSz = ImGui::CalcTextSize(S.tagline);
    float maxTagW = splitX - 40;
    if (tagSz.x > maxTagW) {
      bgDl->AddText(nullptr, 0.0f, ImVec2(cx - maxTagW * 0.5f, tagY),
                    IM_COL32(180, 180, 195, 100), S.tagline, nullptr, maxTagW);
    } else {
      bgDl->AddText(ImVec2(cx - tagSz.x * 0.5f, tagY),
                    IM_COL32(180, 180, 195, 100), S.tagline);
    }
  }

  // ── Versao no canto inferior esquerdo ─────────────────────────────────────
  bgDl->AddText(ImVec2(14, dp.y - 24), IM_COL32(120, 120, 140, 80), "v1.0.0");

  // ── Bandeiras de idioma ──────────────────────────────────────────────────
  DrawLangFlags(bgDl, ctx.language, 12, 9);
  HandleDrag();

  ImGui::End();
  ImGui::PopStyleColor();

  // ═════════════════════════════════════════════════════════════════════════
  // FORMULARIO DE LOGIN — Painel direito
  // ═════════════════════════════════════════════════════════════════════════

  float formW = dp.x * 0.60f;
  float formH = dp.y;
  float formContentW = 320.0f;
  float formStartX = splitX + (formW - formContentW) * 0.5f;

  ImGui::SetNextWindowPos(ImVec2(splitX, 0));
  ImGui::SetNextWindowSize(ImVec2(formW, formH));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.15f, 1.0f));
  ImGui::Begin("##form", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav);

  ImDrawList *fDl = ImGui::GetWindowDrawList();

  // ── Titulo "Welcome" + subtitulo ──────────────────────────────────────────
  float startY = dp.y * 0.22f;
  float relX = formStartX - splitX;

  ImGui::SetCursorPos(ImVec2(relX, startY));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
  ImGui::SetWindowFontScale(1.6f);
  ImGui::Text("%s", S.welcome);
  ImGui::SetWindowFontScale(1.0f);
  ImGui::PopStyleColor();

  ImGui::SetCursorPosX(relX);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.55f, 1.0f));
  ImGui::Text("%s", S.signIn);
  ImGui::PopStyleColor();

  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 30));

  // ── Input: Username ───────────────────────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.60f, 1.0f));
  ImGui::Text("%s", S.username);
  ImGui::PopStyleColor();
  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 4));

  // Input estilizado: fundo escuro arredondado
  ImGui::SetCursorPosX(relX);
  ImGui::PushItemWidth(formContentW);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 10));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.19f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                        ImVec4(0.17f, 0.17f, 0.21f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
                        ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.92f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.30f, 0.5f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

  ImGui::InputText("##user", m_username, sizeof(m_username));
  bool userActive = ImGui::IsItemActive();

  ImGui::PopStyleVar(1); // border size
  ImGui::PopStyleColor(5);
  ImGui::PopStyleVar(2); // padding, rounding
  ImGui::PopItemWidth();

  // Borda rosa brilhante quando ativo
  if (userActive) {
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    fDl->AddRect(mn, mx, IM_COL32(235, 72, 120, 180), 8.0f, 0, 2.0f);
  }

  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 12));

  // ── Input: Password ───────────────────────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.60f, 1.0f));
  ImGui::Text("%s", S.password);
  ImGui::PopStyleColor();
  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 4));

  ImGui::SetCursorPosX(relX);
  ImGui::PushItemWidth(formContentW);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 10));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.19f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                        ImVec4(0.17f, 0.17f, 0.21f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
                        ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.92f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.30f, 0.5f));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

  bool enterPressed = ImGui::InputText(
      "##pass", m_password, sizeof(m_password),
      ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
  bool passActive = ImGui::IsItemActive();

  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(5);
  ImGui::PopStyleVar(2);
  ImGui::PopItemWidth();

  if (passActive) {
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    fDl->AddRect(mn, mx, IM_COL32(235, 72, 120, 180), 8.0f, 0, 2.0f);
  }

  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 10));

  // ── Checkbox + Forgot ─────────────────────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                        ImVec4(0.22f, 0.22f, 0.26f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.92f, 0.28f, 0.47f, 1.0f));
  ImGui::Checkbox(S.keepLogged, &ctx.keepLoggedIn);
  ImGui::PopStyleColor(3);

  // Forgot password na mesma linha
  {
    ImGui::SameLine(relX + formContentW - ImGui::CalcTextSize(S.forgotPass).x);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.28f, 0.47f, 0.8f));
    ImGui::Text("%s", S.forgotPass);
    ImGui::PopStyleColor();
  }

  ImGui::SetCursorPosX(relX);
  ImGui::Dummy(ImVec2(0, 22));

  // ── Botao SIGN IN ─────────────────────────────────────────────────────────
  ImGui::SetCursorPosX(relX);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.92f, 0.28f, 0.47f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.96f, 0.35f, 0.52f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImVec4(0.82f, 0.22f, 0.40f, 1.0f));

  bool doLogin =
      ImGui::Button(S.loginBtn, ImVec2(formContentW, 44)) || enterPressed;

  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar();

  // ── Processar Login ───────────────────────────────────────────────────────
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

  // ── Mensagem de erro ──────────────────────────────────────────────────────
  if (m_errorTimer > 0.0f && !m_errorMsg.empty()) {
    ImGui::SetCursorPosX(relX);
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::SetCursorPosX(relX);
    float alpha = (m_errorTimer < 1.0f) ? m_errorTimer : 1.0f;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.30f, 0.35f, alpha));
    ImGui::TextWrapped("%s", m_errorMsg.c_str());
    ImGui::PopStyleColor();
  }

  // ── Indicador de banco ────────────────────────────────────────────────────
  {
    char buf[16];
    snprintf(buf, sizeof(buf), "ca-%s", ctx.dbConnected ? "1" : "0");
    ImVec2 tsz = ImGui::CalcTextSize(buf);
    ImGui::SetCursorPos(ImVec2(formW - tsz.x - 12, formH - 28));
    ImVec4 col = ctx.dbConnected ? ImVec4(0.30f, 0.80f, 0.50f, 0.4f)
                                 : ImVec4(0.80f, 0.60f, 0.20f, 0.4f);
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::Text("%s", buf);
    ImGui::PopStyleColor();
  }

  // ── Controles da janela (close/max/min) ─────────────────────────────────
  // Desenhados dentro do ##form pois os botoes ficam no canto superior
  // direito, que esta dentro da area do formulario.
  DrawWindowControls(fDl, dp);

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}
