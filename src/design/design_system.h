#pragma once

#include "imgui.h"
#include <algorithm>
#include <cmath>

// =============================================================================
// BR Sense — Design System
// Centralized design tokens, colors, spacing, animation & drawing helpers.
// =============================================================================

namespace DS {

// ─── Color Palette ──────────────────────────────────────────────────────────

// Backgrounds (darkest → lightest)
constexpr ImU32 BG_DARKEST   = IM_COL32(8,   8,  16, 255);   // #080810
constexpr ImU32 BG_DARK      = IM_COL32(10,  10,  20, 255);   // #0a0a14
constexpr ImU32 BG_BASE      = IM_COL32(14,  14,  24, 255);   // #0e0e18
constexpr ImU32 BG_ELEVATED  = IM_COL32(18,  18,  30, 255);   // #12121e
constexpr ImU32 BG_CARD      = IM_COL32(22,  22,  36, 255);   // #161624
constexpr ImU32 BG_INPUT     = IM_COL32(26,  26,  40, 255);   // #1a1a28
constexpr ImU32 BG_HOVER     = IM_COL32(32,  32,  48, 255);   // #202030
constexpr ImU32 BG_TOPBAR    = IM_COL32(12,  12,  22, 255);   // #0c0c16
constexpr ImU32 BG_SIDEBAR   = IM_COL32(10,  10,  18, 255);   // #0a0a12

// Accent — Emerald Green
constexpr ImU32 ACCENT           = IM_COL32(0,   204, 106, 255);  // #00cc6a
constexpr ImU32 ACCENT_HOVER     = IM_COL32(0,   230, 120, 255);  // #00e678
constexpr ImU32 ACCENT_ACTIVE    = IM_COL32(0,   170,  90, 255);  // #00aa5a
constexpr ImU32 ACCENT_GLOW      = IM_COL32(0,   204, 106,  40);  // glow alpha
constexpr ImU32 ACCENT_SUBTLE    = IM_COL32(0,   204, 106,  20);  // very subtle
constexpr ImU32 ACCENT_DIM       = IM_COL32(0,   204, 106, 100);  // dimmed

// Text
constexpr ImU32 TEXT_PRIMARY     = IM_COL32(240, 240, 248, 255);  // #f0f0f8
constexpr ImU32 TEXT_SECONDARY   = IM_COL32(160, 160, 180, 255);  // #a0a0b4
constexpr ImU32 TEXT_MUTED       = IM_COL32(100, 100, 120, 150);  // #64647880
constexpr ImU32 TEXT_DISABLED    = IM_COL32(70,  70,  85,  180);  // #464655

// Semantic colors
constexpr ImU32 SUCCESS          = IM_COL32(80,  200, 120, 255);  // #50c878
constexpr ImU32 SUCCESS_BG       = IM_COL32(10,  50,  25,  200);  // dark green bg
constexpr ImU32 ERROR_COL        = IM_COL32(240, 70,  70,  255);  // #f04646
constexpr ImU32 ERROR_BG         = IM_COL32(60,  15,  15,  200);  // dark red bg
constexpr ImU32 WARNING          = IM_COL32(255, 200, 60,  255);  // #ffc83c
constexpr ImU32 WARNING_BG       = IM_COL32(60,  50,  10,  200);  // dark yellow bg
constexpr ImU32 INFO             = IM_COL32(100, 180, 255, 255);  // #64b4ff
constexpr ImU32 INFO_BG          = IM_COL32(15,  30,  60,  200);  // dark blue bg

// Borders / Dividers
constexpr ImU32 BORDER           = IM_COL32(45,  45,  65,  100);  // subtle border
constexpr ImU32 BORDER_FOCUS     = IM_COL32(0,   204, 106, 180);  // green border
constexpr ImU32 DIVIDER          = IM_COL32(40,  40,  58,  80);   // section divider

// Window control dot colors
constexpr ImU32 DOT_CLOSE        = IM_COL32(255, 95,  87,  255);  // red
constexpr ImU32 DOT_MAXIMIZE     = IM_COL32(255, 189, 46,  255);  // amber
constexpr ImU32 DOT_MINIMIZE     = IM_COL32(40,  200, 65,  255);  // green
constexpr ImU32 DOT_INACTIVE     = IM_COL32(60,  60,  75,  180);  // gray

// ImVec4 versions (for PushStyleColor)
constexpr ImVec4 V4_BG_DARKEST   = ImVec4(0.031f, 0.031f, 0.063f, 1.0f);
constexpr ImVec4 V4_BG_DARK      = ImVec4(0.039f, 0.039f, 0.078f, 1.0f);
constexpr ImVec4 V4_BG_BASE      = ImVec4(0.055f, 0.055f, 0.094f, 1.0f);
constexpr ImVec4 V4_BG_ELEVATED  = ImVec4(0.071f, 0.071f, 0.118f, 1.0f);
constexpr ImVec4 V4_BG_CARD      = ImVec4(0.086f, 0.086f, 0.141f, 1.0f);
constexpr ImVec4 V4_BG_INPUT     = ImVec4(0.102f, 0.102f, 0.157f, 1.0f);
constexpr ImVec4 V4_BG_HOVER     = ImVec4(0.125f, 0.125f, 0.188f, 1.0f);

constexpr ImVec4 V4_ACCENT       = ImVec4(0.0f,  0.80f, 0.42f, 1.0f);
constexpr ImVec4 V4_ACCENT_HOVER = ImVec4(0.0f,  0.90f, 0.47f, 1.0f);
constexpr ImVec4 V4_ACCENT_ACTIVE= ImVec4(0.0f,  0.67f, 0.35f, 1.0f);

constexpr ImVec4 V4_TEXT         = ImVec4(0.94f, 0.94f, 0.97f, 1.0f);
constexpr ImVec4 V4_TEXT_SEC     = ImVec4(0.63f, 0.63f, 0.71f, 1.0f);
constexpr ImVec4 V4_TEXT_MUTED   = ImVec4(0.39f, 0.39f, 0.47f, 0.6f);
constexpr ImVec4 V4_BORDER       = ImVec4(0.18f, 0.18f, 0.25f, 0.4f);
constexpr ImVec4 V4_TRANSPARENT  = ImVec4(0, 0, 0, 0);
constexpr ImVec4 V4_ERROR        = ImVec4(0.94f, 0.27f, 0.27f, 1.0f);

// ─── Spacing Tokens ─────────────────────────────────────────────────────────

constexpr float PAD_XS  = 4.0f;
constexpr float PAD_SM  = 8.0f;
constexpr float PAD_MD  = 16.0f;
constexpr float PAD_LG  = 24.0f;
constexpr float PAD_XL  = 32.0f;
constexpr float PAD_2XL = 48.0f;

// ─── Rounding Tokens ────────────────────────────────────────────────────────

constexpr float ROUND_SM   = 4.0f;
constexpr float ROUND_MD   = 8.0f;
constexpr float ROUND_LG   = 12.0f;
constexpr float ROUND_XL   = 16.0f;
constexpr float ROUND_PILL = 100.0f; // fully rounded

// ─── Layout ─────────────────────────────────────────────────────────────────

constexpr float TOPBAR_H   = 52.0f;
constexpr float SIDEBAR_W  = 68.0f;

// ─── Animation Helpers ──────────────────────────────────────────────────────

inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline ImVec4 LerpColor(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(
        Lerp(a.x, b.x, t), Lerp(a.y, b.y, t),
        Lerp(a.z, b.z, t), Lerp(a.w, b.w, t)
    );
}

inline float EaseOutCubic(float t) {
    t = t - 1.0f;
    return t * t * t + 1.0f;
}

inline float EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f;
}

inline float SmoothStep(float edge0, float edge1, float x) {
    float t = (std::max)(0.0f, (std::min)(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

inline float PulseAlpha(float time, float speed = 1.5f, float min_a = 0.4f, float max_a = 1.0f) {
    float t = (sinf(time * speed) + 1.0f) * 0.5f;
    return Lerp(min_a, max_a, t);
}

// ─── Drawing Helpers ────────────────────────────────────────────────────────

inline ImU32 WithAlpha(ImU32 col, int alpha) {
    return (col & 0x00FFFFFF) | ((ImU32)alpha << 24);
}

inline ImU32 HexToImU32(const char* hex, int alpha = 255) {
    int r = 0, g = 0, b = 0;
    if (hex && hex[0] == '#' && strlen(hex) >= 7) {
        sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
    }
    return IM_COL32(r, g, b, alpha);
}

// Glow rectangle (rect with soft outer glow)
inline void DrawGlowRect(ImDrawList* dl, ImVec2 min, ImVec2 max,
                          ImU32 glowColor, float glowSize = 8.0f, float rounding = 0.0f) {
    for (float i = glowSize; i > 0; i -= 2.0f) {
        float alpha = (1.0f - i / glowSize) * 0.15f;
        ImU32 c = WithAlpha(glowColor, (int)(alpha * 255));
        dl->AddRect(
            ImVec2(min.x - i, min.y - i),
            ImVec2(max.x + i, max.y + i),
            c, rounding + i * 0.5f, 0, 1.5f
        );
    }
}

// Card with subtle border and optional glow
inline void DrawCard(ImDrawList* dl, ImVec2 min, ImVec2 max,
                     ImU32 bgColor = BG_CARD, float rounding = ROUND_LG,
                     bool glow = false, ImU32 glowColor = ACCENT_GLOW) {
    dl->AddRectFilled(min, max, bgColor, rounding);
    dl->AddRect(min, max, BORDER, rounding, 0, 1.0f);
    if (glow) {
        DrawGlowRect(dl, min, max, glowColor, 6.0f, rounding);
    }
}

// Badge / Pill shape
inline void DrawBadge(ImDrawList* dl, ImVec2 pos, const char* text,
                      ImU32 textCol, ImU32 bgCol, float fontSize = 0.0f) {
    ImVec2 textSz = ImGui::CalcTextSize(text);
    float padX = 10.0f, padY = 4.0f;
    ImVec2 min(pos.x, pos.y);
    ImVec2 max(pos.x + textSz.x + padX * 2, pos.y + textSz.y + padY * 2);
    dl->AddRectFilled(min, max, bgCol, ROUND_PILL);
    dl->AddText(ImVec2(pos.x + padX, pos.y + padY), textCol, text);
}

// Animated spinner (arc)
inline void DrawSpinner(ImDrawList* dl, ImVec2 center, float radius,
                        float time, ImU32 color = ACCENT, float thickness = 2.5f) {
    const float startAngle = time * 4.0f;
    const float arcLen = 3.14159f * 1.2f;
    const int segments = 24;
    for (int i = 0; i < segments; ++i) {
        float a1 = startAngle + ((float)i / segments) * arcLen;
        float a2 = startAngle + ((float)(i + 1) / segments) * arcLen;
        float alpha = (float)i / segments;
        ImU32 c = WithAlpha(color, (int)(alpha * ((color >> 24) & 0xFF)));
        dl->AddLine(
            ImVec2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius),
            ImVec2(center.x + cosf(a2) * radius, center.y + sinf(a2) * radius),
            c, thickness
        );
    }
}

// Gradient filled rect (horizontal)
inline void DrawHGradient(ImDrawList* dl, ImVec2 min, ImVec2 max,
                          ImU32 left, ImU32 right, float rounding = 0.0f) {
    dl->AddRectFilledMultiColor(min, max, left, right, right, left);
}

// Gradient filled rect (vertical)
inline void DrawVGradient(ImDrawList* dl, ImVec2 min, ImVec2 max,
                          ImU32 top, ImU32 bottom, float rounding = 0.0f) {
    dl->AddRectFilledMultiColor(min, max, top, top, bottom, bottom);
}

} // namespace DS
