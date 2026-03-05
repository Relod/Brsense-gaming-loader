
#include "theme.h"
#include "design_system.h"
#include "imgui.h"

void ApplyCustomDarkTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  // ── Geometry ────────────────────────────────────────────────────────────
  style.WindowRounding = DS::ROUND_MD;
  style.FrameRounding = DS::ROUND_MD;
  style.PopupRounding = DS::ROUND_MD;
  style.ScrollbarRounding = DS::ROUND_LG;
  style.GrabRounding = DS::ROUND_SM;
  style.TabRounding = DS::ROUND_SM;
  style.ChildRounding = DS::ROUND_MD;

  style.WindowPadding = ImVec2(DS::PAD_MD, DS::PAD_MD);
  style.FramePadding = ImVec2(12.0f, 8.0f);
  style.ItemSpacing = ImVec2(DS::PAD_SM, DS::PAD_SM);
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
  style.ScrollbarSize = 8.0f;
  style.GrabMinSize = 6.0f;

  style.WindowBorderSize = 0.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;

  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
  style.AntiAliasedLines = true;
  style.AntiAliasedFill = true;

  // ── Colors ──────────────────────────────────────────────────────────────
  colors[ImGuiCol_Text] = DS::V4_TEXT;
  colors[ImGuiCol_TextDisabled] = DS::V4_TEXT_MUTED;

  colors[ImGuiCol_WindowBg] = DS::V4_BG_BASE;
  colors[ImGuiCol_ChildBg] = DS::V4_TRANSPARENT;
  colors[ImGuiCol_PopupBg] = DS::V4_BG_ELEVATED;

  colors[ImGuiCol_Border] = DS::V4_BORDER;
  colors[ImGuiCol_BorderShadow] = DS::V4_TRANSPARENT;

  colors[ImGuiCol_FrameBg] = DS::V4_BG_INPUT;
  colors[ImGuiCol_FrameBgHovered] = DS::V4_BG_HOVER;
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.14f, 0.14f, 0.20f, 1.0f);

  colors[ImGuiCol_TitleBg] = DS::V4_BG_DARK;
  colors[ImGuiCol_TitleBgActive] = DS::V4_BG_DARK;
  colors[ImGuiCol_TitleBgCollapsed] = DS::V4_BG_DARK;

  colors[ImGuiCol_MenuBarBg] = DS::V4_BG_DARK;

  colors[ImGuiCol_ScrollbarBg] = DS::V4_TRANSPARENT;
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.22f, 0.30f, 0.6f);
  colors[ImGuiCol_ScrollbarGrabHovered] = DS::V4_ACCENT_HOVER;
  colors[ImGuiCol_ScrollbarGrabActive] = DS::V4_ACCENT;

  colors[ImGuiCol_CheckMark] = DS::V4_ACCENT;
  colors[ImGuiCol_SliderGrab] = DS::V4_ACCENT;
  colors[ImGuiCol_SliderGrabActive] = DS::V4_ACCENT_HOVER;

  colors[ImGuiCol_Button] = DS::V4_ACCENT;
  colors[ImGuiCol_ButtonHovered] = DS::V4_ACCENT_HOVER;
  colors[ImGuiCol_ButtonActive] = DS::V4_ACCENT_ACTIVE;

  colors[ImGuiCol_Header] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.18f, 0.24f, 1.0f);
  colors[ImGuiCol_HeaderActive] = DS::V4_ACCENT;

  colors[ImGuiCol_Separator] = DS::V4_BORDER;
  colors[ImGuiCol_SeparatorHovered] = DS::V4_ACCENT_HOVER;
  colors[ImGuiCol_SeparatorActive] = DS::V4_ACCENT;

  colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.26f, 0.5f);
  colors[ImGuiCol_ResizeGripHovered] = DS::V4_ACCENT_HOVER;
  colors[ImGuiCol_ResizeGripActive] = DS::V4_ACCENT;

  colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);
  colors[ImGuiCol_TabHovered] = DS::V4_ACCENT_HOVER;
  colors[ImGuiCol_TabSelected] = DS::V4_ACCENT;
  colors[ImGuiCol_TabDimmed] = DS::V4_BG_DARK;
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.16f, 0.16f, 0.20f, 1.0f);

  colors[ImGuiCol_PlotLines] = DS::V4_ACCENT;
  colors[ImGuiCol_PlotLinesHovered] = DS::V4_ACCENT_HOVER;
  colors[ImGuiCol_PlotHistogram] = DS::V4_ACCENT;
  colors[ImGuiCol_PlotHistogramHovered] = DS::V4_ACCENT_HOVER;

  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);
  colors[ImGuiCol_TableBorderStrong] = DS::V4_BORDER;
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.12f, 0.12f, 0.16f, 1.0f);
  colors[ImGuiCol_TableRowBg] = DS::V4_TRANSPARENT;
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.04f, 0.04f, 0.06f, 0.3f);

  colors[ImGuiCol_TextSelectedBg] =
      ImVec4(DS::V4_ACCENT.x, DS::V4_ACCENT.y, DS::V4_ACCENT.z, 0.25f);

  colors[ImGuiCol_DragDropTarget] = DS::V4_ACCENT;
  colors[ImGuiCol_NavHighlight] = DS::V4_ACCENT;
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);
}
