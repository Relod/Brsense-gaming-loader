
#include "ui_controls.h"
#include "design_system.h"
#include "fonts.h"
#include "imgui.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

extern HWND g_hWnd;

// ═════════════════════════════════════════════════════════════════════════════
// Window Controls — macOS-style colored dots
// ═════════════════════════════════════════════════════════════════════════════

void DrawWindowControls(ImDrawList *dl, ImVec2 sz, bool lightBg) {
  (void)lightBg; // no longer needed — always dark

  float dotR = 6.5f;      // dot radius
  float gap = 10.0f;      // spacing between dots
  float topPad = 11.0f;   // top padding
  float rightPad = 14.0f; // right padding

  // Positions (right to left: close, maximize, minimize)
  float cx3 = sz.x - rightPad - dotR; // close
  float cx2 = cx3 - (dotR * 2 + gap); // maximize
  float cx1 = cx2 - (dotR * 2 + gap); // minimize
  float cy = topPad + dotR;

  // ── Close (red) ───────────────────────────────────────────────────────
  {
    ImVec2 mn(cx3 - dotR, cy - dotR);
    ImVec2 mx(cx3 + dotR, cy + dotR);
    bool hover = ImGui::IsMouseHoveringRect(mn, mx);
    dl->AddCircleFilled(ImVec2(cx3, cy), dotR,
                        hover ? IM_COL32(255, 60, 50, 255) : DS::DOT_CLOSE, 16);
    if (hover) {
      float m = 3.0f;
      dl->AddLine(ImVec2(cx3 - m, cy - m), ImVec2(cx3 + m, cy + m),
                  IM_COL32(80, 0, 0, 255), 1.5f);
      dl->AddLine(ImVec2(cx3 + m, cy - m), ImVec2(cx3 - m, cy + m),
                  IM_COL32(80, 0, 0, 255), 1.5f);
    }
    if (hover && ImGui::IsMouseClicked(0))
      PostMessage(g_hWnd, WM_CLOSE, 0, 0);
  }

  // ── Maximize (amber) ─────────────────────────────────────────────────
  {
    ImVec2 mn(cx2 - dotR, cy - dotR);
    ImVec2 mx(cx2 + dotR, cy + dotR);
    bool hover = ImGui::IsMouseHoveringRect(mn, mx);
    dl->AddCircleFilled(ImVec2(cx2, cy), dotR,
                        hover ? IM_COL32(255, 200, 20, 255) : DS::DOT_MAXIMIZE,
                        16);
    if (hover) {
      float m = 2.5f;
      dl->AddRect(ImVec2(cx2 - m, cy - m), ImVec2(cx2 + m, cy + m),
                  IM_COL32(100, 60, 0, 255), 0.5f, 0, 1.2f);
    }
    if (hover && ImGui::IsMouseClicked(0)) {
      WINDOWPLACEMENT wp = {sizeof(wp)};
      GetWindowPlacement(g_hWnd, &wp);
      ShowWindow(g_hWnd, wp.showCmd == SW_MAXIMIZE ? SW_RESTORE : SW_MAXIMIZE);
    }
  }

  // ── Minimize (green) ─────────────────────────────────────────────────
  {
    ImVec2 mn(cx1 - dotR, cy - dotR);
    ImVec2 mx(cx1 + dotR, cy + dotR);
    bool hover = ImGui::IsMouseHoveringRect(mn, mx);
    dl->AddCircleFilled(ImVec2(cx1, cy), dotR,
                        hover ? IM_COL32(20, 220, 50, 255) : DS::DOT_MINIMIZE,
                        16);
    if (hover) {
      dl->AddLine(ImVec2(cx1 - 3, cy), ImVec2(cx1 + 3, cy),
                  IM_COL32(0, 60, 10, 255), 1.5f);
    }
    if (hover && ImGui::IsMouseClicked(0))
      ShowWindow(g_hWnd, SW_MINIMIZE);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Language Flags — Improved rendering
// ═════════════════════════════════════════════════════════════════════════════

void DrawLangFlags(ImDrawList *dl, Language &lang, float x, float y) {
  float fw = 22, fh = 14, sp = 5;
  float aOn = 255, aOff = 70;

  // ── English Flag ──────────────────────────────────────────────────────
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

  // ── Brazil Flag ───────────────────────────────────────────────────────
  {
    float bx = x + fw + sp;
    ImVec2 mn(bx, y), mx(bx + fw, y + fh);
    bool hov = ImGui::IsMouseHoveringRect(mn, mx);
    float a = (lang == Language::PT) ? aOn : (hov ? 170.0f : aOff);
    int ai = (int)a;
    dl->AddRectFilled(mn, mx, IM_COL32(0, 130, 60, ai), 2);
    float cx = bx + fw * .5f, cy = y + fh * .5f;
    float dw = fw * .38f, dh = fh * .38f;
    ImVec2 d[4] = {{cx, cy - dh}, {cx + dw, cy}, {cx, cy + dh}, {cx - dw, cy}};
    dl->AddConvexPolyFilled(d, 4, IM_COL32(255, 210, 50, ai));
    dl->AddCircleFilled(ImVec2(cx, cy), fh * .17f, IM_COL32(20, 50, 120, ai),
                        12);
    if (hov && ImGui::IsMouseClicked(0))
      lang = Language::PT;
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Window Drag Handler
// ═════════════════════════════════════════════════════════════════════════════

void HandleDrag(float h) {
  ImGuiIO &io = ImGui::GetIO();
  float dragH = (h > 0.0f) ? h : io.DisplaySize.y;
  if (io.MousePos.y < dragH && !ImGui::IsAnyItemHovered() &&
      !ImGui::IsAnyItemActive()) {
    if (ImGui::IsMouseClicked(0)) {
      ReleaseCapture();
      SendMessage(g_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }
  }
}
