// =============================================================================
// logo.h — Logo Anubis Geometrico
// =============================================================================
// Desenha o logo de Anubis (caveira geometrica de cachorro egipcio) usando
// primitivas do ImDrawList. Estilo low-poly/dimensional com linhas pulsantes.
//
// USO:
//   DrawAnubisLogo(drawList, centerX, centerY, scale, time);
//   DrawAnubisLogo(drawList, centerX, centerY, scale, time, 200); // alpha
// =============================================================================

#pragma once

struct ImDrawList;

/// Desenha o logo geometrico de Anubis.
/// @param dl        ImDrawList para desenhar
/// @param cx        Coordenada X do centro do logo
/// @param cy        Coordenada Y do centro do logo
/// @param scale     Escala do logo (100.0f = tamanho padrao)
/// @param time      Tempo atual para animacao pulsante
/// @param alphaBase Opacidade base das linhas (0-255, padrao: 230)
void DrawAnubisLogo(ImDrawList *dl, float cx, float cy, float scale, float time,
                    int alphaBase = 230);
