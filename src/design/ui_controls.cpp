// =============================================================================
// ui_controls.cpp — Implementacao dos Controles de UI
// =============================================================================
// Controles customizados para janela WS_POPUP (borderless):
//   - Botoes circulares de fechar/maximizar/minimizar
//   - Bandeiras de idioma desenhadas proceduralmente
//   - Arrastar janela pela area superior
// =============================================================================

#include "ui_controls.h"
#include "imgui.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Handle da janela definido em main.cpp
extern HWND g_hWnd;

// =============================================================================
// CONTROLES DA JANELA — Fechar, Maximizar, Minimizar
// =============================================================================

void DrawWindowControls(ImDrawList *dl, ImVec2 sz, bool lightBg) {
  float bs = 12.0f, sp = 7.0f, tp = 9.0f, rp = 12.0f;
  ImU32 closeCol = IM_COL32(235, 72, 120, 220);
  ImU32 closeHov = IM_COL32(255, 80, 80, 255);
  ImU32 btnCol =
      lightBg ? IM_COL32(160, 160, 170, 140) : IM_COL32(60, 60, 68, 180);
  ImU32 btnHov =
      lightBg ? IM_COL32(130, 130, 140, 200) : IM_COL32(80, 80, 90, 200);
  ImU32 iconCol =
      lightBg ? IM_COL32(80, 80, 90, 220) : IM_COL32(200, 200, 210, 200);

  // ── Fechar ────────────────────────────────────────────────────────────────
  {
    float cx = sz.x - rp - bs * .5f, cy = tp + bs * .5f;
    ImVec2 mn(cx - bs * .5f, cy - bs * .5f), mx(cx + bs * .5f, cy + bs * .5f);
    bool h = ImGui::IsMouseHoveringRect(mn, mx);
    dl->AddCircleFilled(ImVec2(cx, cy), bs * .55f, h ? closeHov : closeCol);
    float m = 3.0f;
    dl->AddLine(ImVec2(cx - m, cy - m), ImVec2(cx + m, cy + m),
                IM_COL32(255, 255, 255, 220), 1.3f);
    dl->AddLine(ImVec2(cx + m, cy - m), ImVec2(cx - m, cy + m),
                IM_COL32(255, 255, 255, 220), 1.3f);
    if (h && ImGui::IsMouseClicked(0))
      PostMessage(g_hWnd, WM_CLOSE, 0, 0);
  }

  // ── Maximizar ─────────────────────────────────────────────────────────────
  {
    float cx = sz.x - rp - bs - sp - bs * .5f, cy = tp + bs * .5f;
    ImVec2 mn(cx - bs * .5f, cy - bs * .5f), mx(cx + bs * .5f, cy + bs * .5f);
    bool h = ImGui::IsMouseHoveringRect(mn, mx);
    dl->AddCircleFilled(ImVec2(cx, cy), bs * .55f, h ? btnHov : btnCol);
    float m = 2.5f;
    dl->AddRect(ImVec2(cx - m, cy - m), ImVec2(cx + m, cy + m), iconCol, 0.5f,
                0, 1.0f);
    if (h && ImGui::IsMouseClicked(0)) {
      WINDOWPLACEMENT wp = {sizeof(wp)};
      GetWindowPlacement(g_hWnd, &wp);
      ShowWindow(g_hWnd, wp.showCmd == SW_MAXIMIZE ? SW_RESTORE : SW_MAXIMIZE);
    }
  }

  // ── Minimizar ─────────────────────────────────────────────────────────────
  {
    float cx = sz.x - rp - (bs + sp) * 2 - bs * .5f, cy = tp + bs * .5f;
    ImVec2 mn(cx - bs * .5f, cy - bs * .5f), mx(cx + bs * .5f, cy + bs * .5f);
    bool h = ImGui::IsMouseHoveringRect(mn, mx);
    dl->AddCircleFilled(ImVec2(cx, cy), bs * .55f, h ? btnHov : btnCol);
    dl->AddLine(ImVec2(cx - 3, cy), ImVec2(cx + 3, cy), iconCol, 1.3f);
    if (h && ImGui::IsMouseClicked(0))
      ShowWindow(g_hWnd, SW_MINIMIZE);
  }
}

// =============================================================================
// BANDEIRAS DE IDIOMA — EUA e Brasil
// =============================================================================

void DrawLangFlags(ImDrawList *dl, Language &lang, float x, float y) {
  float fw = 22, fh = 14, sp = 5;
  float aOn = 255, aOff = 70;

  // ── Bandeira EUA ──────────────────────────────────────────────────────────
  {
    ImVec2 mn(x, y), mx(x + fw, y + fh);
    bool hov = ImGui::IsMouseHoveringRect(mn, mx);
    float a = (lang == Language::EN) ? aOn : (hov ? 170.0f : aOff);
    int ai = (int)a;
    dl->AddRectFilled(mn, mx, IM_COL32(180, 40, 50, ai), 2);
    for (int i = 0; i < 7; i += 2) {
      float sy = y + i * (fh / 7.f);
      dl->AddRectFilled(ImVec2(x, sy), ImVec2(x + fw, sy + fh / 7.f),
                        IM_COL32(255, 255, 255, ai));
    }
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + fw * .4f, y + fh * .55f),
                      IM_COL32(40, 60, 130, ai));
    dl->AddCircleFilled(ImVec2(x + fw * .2f, y + fh * .28f), 1.5f,
                        IM_COL32(255, 255, 255, ai));
    if (hov && ImGui::IsMouseClicked(0))
      lang = Language::EN;
  }

  // ── Bandeira Brasil ───────────────────────────────────────────────────────
  {
    float bx = x + fw + sp;
    ImVec2 mn(bx, y), mx(bx + fw, y + fh);
    bool hov = ImGui::IsMouseHoveringRect(mn, mx);
    float a = (lang == Language::PT) ? aOn : (hov ? 170.0f : aOff);
    int ai = (int)a;
    dl->AddRectFilled(mn, mx, IM_COL32(0, 130, 60, ai), 2);
    float cx = bx + fw * .5f, cy = y + fh * .5f, dw = fw * .38f, dh = fh * .38f;
    ImVec2 d[4] = {{cx, cy - dh}, {cx + dw, cy}, {cx, cy + dh}, {cx - dw, cy}};
    dl->AddConvexPolyFilled(d, 4, IM_COL32(255, 210, 50, ai));
    dl->AddCircleFilled(ImVec2(cx, cy), fh * .17f, IM_COL32(20, 50, 120, ai),
                        12);
    if (hov && ImGui::IsMouseClicked(0))
      lang = Language::PT;
  }
}

// =============================================================================
// ARRASTAR JANELA — pela area superior da janela borderless
// =============================================================================

void HandleDrag(float h) {
  ImGuiIO &io = ImGui::GetIO();
  if (io.MousePos.y < h && !ImGui::IsAnyItemHovered() &&
      !ImGui::IsAnyItemActive()) {
    if (ImGui::IsMouseClicked(0)) {
      ReleaseCapture();
      SendMessage(g_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }
  }
}
