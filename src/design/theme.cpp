// =============================================================================
// theme.cpp — Tema Visual Dark Mode Customizado
// =============================================================================
// Aplica uma paleta de cores moderna ao ImGui:
// - Fundo: cinza escuro (#383839)
// - Acento: rosa/magenta (#eb4878) nos botoes e destaques
// - Texto: branco suave nos escuros, cinza escuro nos claros
// - Bordas arredondadas suaves, espacamento confortavel
// =============================================================================

#include "theme.h"
#include "imgui.h"

void ApplyCustomDarkTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  // ── Paleta de Cores ──────────────────────────────────────────────────────

  // Fundos — cinzas neutros escuros
  const ImVec4 bg_main =
      ImVec4(0.22f, 0.22f, 0.24f, 1.00f); // #383839 fundo principal
  const ImVec4 bg_dark =
      ImVec4(0.18f, 0.18f, 0.20f, 1.00f); // #2e2e33 fundo escuro
  const ImVec4 bg_input =
      ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // transparente (inputs)

  // Acento — rosa/magenta (efeito gradiente)
  const ImVec4 accent =
      ImVec4(0.92f, 0.28f, 0.47f, 1.00f); // #eb4878 cor principal
  const ImVec4 accent_hover =
      ImVec4(0.96f, 0.35f, 0.52f, 1.00f); // #f55985 hover
  const ImVec4 accent_active =
      ImVec4(0.82f, 0.22f, 0.40f, 1.00f); // #d13866 ao clicar

  // Texto
  const ImVec4 text_light =
      ImVec4(0.90f, 0.90f, 0.92f, 1.00f); // texto claro (fundo escuro)
  const ImVec4 text_disabled =
      ImVec4(0.38f, 0.38f, 0.40f, 1.00f); // texto desabilitado

  // Bordas
  const ImVec4 border_subtle =
      ImVec4(0.30f, 0.30f, 0.32f, 0.5f); // bordas sutis

  // ── Estilo (arredondamentos, espacamentos) ───────────────────────────────
  style.WindowRounding = 4.0f; // arredondamento das janelas
  style.FrameRounding = 3.0f;  // arredondamento dos inputs/botoes
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 3.0f;
  style.ChildRounding = 4.0f;

  style.WindowPadding = ImVec2(16.0f, 16.0f); // espaco interno das janelas
  style.FramePadding = ImVec2(10.0f, 7.0f);   // espaco interno dos frames
  style.ItemSpacing = ImVec2(8.0f, 8.0f);     // espaco entre itens
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
  style.ScrollbarSize = 10.0f;
  style.GrabMinSize = 8.0f;

  style.WindowBorderSize = 0.0f; // sem borda nas janelas
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 0.0f;

  style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // titulo centralizado

  // ── Atribuicao de Cores ──────────────────────────────────────────────────

  // Texto
  colors[ImGuiCol_Text] = text_light;
  colors[ImGuiCol_TextDisabled] = text_disabled;

  // Janelas
  colors[ImGuiCol_WindowBg] = bg_main;
  colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0); // transparente
  colors[ImGuiCol_PopupBg] = bg_dark;

  // Bordas
  colors[ImGuiCol_Border] = border_subtle;
  colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

  // Frames (inputs, etc) — cinza escuro sutil
  colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);

  // Titulo das janelas
  colors[ImGuiCol_TitleBg] = bg_dark;
  colors[ImGuiCol_TitleBgActive] = bg_dark;
  colors[ImGuiCol_TitleBgCollapsed] = bg_dark;

  // Barra de menu
  colors[ImGuiCol_MenuBarBg] = bg_dark;

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg] = bg_dark;
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.38f, 1.0f);
  colors[ImGuiCol_ScrollbarGrabHovered] = accent_hover;
  colors[ImGuiCol_ScrollbarGrabActive] = accent;

  // Checkmark e sliders — cor acento
  colors[ImGuiCol_CheckMark] = accent;
  colors[ImGuiCol_SliderGrab] = accent;
  colors[ImGuiCol_SliderGrabActive] = accent_hover;

  // Botoes — rosa/magenta
  colors[ImGuiCol_Button] = accent;
  colors[ImGuiCol_ButtonHovered] = accent_hover;
  colors[ImGuiCol_ButtonActive] = accent_active;

  // Headers (collapsable, selectable)
  colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.33f, 1.0f);
  colors[ImGuiCol_HeaderActive] = accent;

  // Separadores
  colors[ImGuiCol_Separator] = border_subtle;
  colors[ImGuiCol_SeparatorHovered] = accent_hover;
  colors[ImGuiCol_SeparatorActive] = accent;

  // Alca de redimensionamento
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.30f, 0.33f, 1.0f);
  colors[ImGuiCol_ResizeGripHovered] = accent_hover;
  colors[ImGuiCol_ResizeGripActive] = accent;

  // Abas
  colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
  colors[ImGuiCol_TabHovered] = accent_hover;
  colors[ImGuiCol_TabSelected] = accent;
  colors[ImGuiCol_TabDimmed] = bg_dark;
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);

  // Graficos
  colors[ImGuiCol_PlotLines] = accent;
  colors[ImGuiCol_PlotLinesHovered] = accent_hover;
  colors[ImGuiCol_PlotHistogram] = accent;
  colors[ImGuiCol_PlotHistogramHovered] = accent_hover;

  // Tabelas
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
  colors[ImGuiCol_TableBorderStrong] = border_subtle;
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.05f, 0.05f, 0.07f, 0.3f);

  // Selecao de texto
  colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.30f);

  // Arrastar e soltar
  colors[ImGuiCol_DragDropTarget] = accent;

  // Navegacao
  colors[ImGuiCol_NavHighlight] = accent;
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.60f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.55f);
}
