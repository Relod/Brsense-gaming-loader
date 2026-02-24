// =============================================================================
// ui_controls.h — Controles de UI da Janela Borderless
// =============================================================================
// Funcoes para desenhar controles customizados de janela e interacao:
//   - DrawWindowControls: botoes fechar/maximizar/minimizar (circulos)
//   - DrawLangFlags: bandeiras EUA/Brasil para troca de idioma
//   - HandleDrag: arrastar a janela pela area superior
//
// Requer o handle da janela global `g_hWnd` (definido em main.cpp).
// =============================================================================

#pragma once

#include "strings.h" // Language enum

struct ImDrawList;
struct ImVec2;

/// Desenha os botoes de controle da janela (fechar, maximizar, minimizar).
/// @param dl      ImDrawList para desenhar
/// @param sz      Tamanho da janela (ImVec2 com largura e altura)
/// @param lightBg Se true, usa cores mais claras para os botoes (fundo claro)
void DrawWindowControls(ImDrawList *dl, ImVec2 sz, bool lightBg = false);

/// Desenha as bandeiras de idioma (EUA e Brasil) como seletor.
/// @param dl   ImDrawList para desenhar
/// @param lang Referencia ao idioma atual — sera alterado ao clicar
/// @param x    Posicao X do inicio das bandeiras
/// @param y    Posicao Y do inicio das bandeiras
void DrawLangFlags(ImDrawList *dl, Language &lang, float x, float y);

/// Permite arrastar a janela borderless segurando a area superior.
/// @param h Altura da area de arraste em pixels (padrao: 32)
void HandleDrag(float h = 32);
