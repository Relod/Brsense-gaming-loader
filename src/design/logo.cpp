// =============================================================================
// logo.cpp — Implementacao do Logo Anubis Geometrico
// =============================================================================
// Renderizacao procedural de um Anubis em estilo low-poly/dimensional.
// Todas as formas sao desenhadas com ImDrawList (linhas, poligonos, circulos).
//
// PARTES DO LOGO:
//   1. Orelhas — altas e pontiagudas como chacal, com listras internas
//   2. Coroa/Headpiece — chevrons empilhados (3 camadas)
//   3. Contorno do rosto — hexagono alongado verticalmente
//   4. Diamante na testa
//   5. Olhos — angulares, alongados horizontalmente
//   6. Nariz — linha vertical + narinas em V
//   7. Boca/Mandibula
//   8. Nemes/Headdress egipcio — drapes laterais com hachura diagonal
//   9. Linhas pulsantes decorativas — efeito dimensional animado
// =============================================================================

#include "logo.h"
#include "imgui.h"
#include <cmath>

void DrawAnubisLogo(ImDrawList *dl, float cx, float cy, float scale, float time,
                    int alphaBase) {
  float s = scale;
  ImU32 mainCol = IM_COL32(235, 72, 120, alphaBase);
  ImU32 detCol = IM_COL32(235, 72, 120, (int)(alphaBase * 0.45f));
  float t = 2.0f;  // espessura principal
  float td = 1.2f; // espessura detalhe

  // ══════════════════════════════════════════════════════════════════════════
  // ORELHAS — altas e pontiagudas como chacal, com listras internas
  // ══════════════════════════════════════════════════════════════════════════

  // Orelha esquerda (contorno externo)
  dl->AddLine(ImVec2(cx - 0.18f * s, cy - 0.32f * s),
              ImVec2(cx - 0.32f * s, cy - 1.15f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.32f * s, cy - 1.15f * s),
              ImVec2(cx - 0.42f * s, cy - 0.32f * s), mainCol, t);
  // Orelha esquerda (contorno interno)
  dl->AddLine(ImVec2(cx - 0.10f * s, cy - 0.32f * s),
              ImVec2(cx - 0.25f * s, cy - 0.92f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.25f * s, cy - 0.92f * s),
              ImVec2(cx - 0.32f * s, cy - 1.15f * s), mainCol, t * 0.8f);
  // Listras internas esquerda
  for (int i = 1; i <= 4; i++) {
    float f = i * 0.18f;
    float lx1 = cx - (0.12f + f * 0.05f) * s;
    float ly1 = cy - (0.32f + f * 0.40f) * s;
    float lx2 = cx - (0.38f + f * 0.01f) * s;
    float ly2 = cy - (0.32f + f * 0.28f) * s;
    dl->AddLine(ImVec2(lx1, ly1), ImVec2(lx2, ly2), detCol, td);
  }

  // Orelha direita (espelho)
  dl->AddLine(ImVec2(cx + 0.18f * s, cy - 0.32f * s),
              ImVec2(cx + 0.32f * s, cy - 1.15f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.32f * s, cy - 1.15f * s),
              ImVec2(cx + 0.42f * s, cy - 0.32f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.10f * s, cy - 0.32f * s),
              ImVec2(cx + 0.25f * s, cy - 0.92f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.25f * s, cy - 0.92f * s),
              ImVec2(cx + 0.32f * s, cy - 1.15f * s), mainCol, t * 0.8f);
  for (int i = 1; i <= 4; i++) {
    float f = i * 0.18f;
    float lx1 = cx + (0.12f + f * 0.05f) * s;
    float ly1 = cy - (0.32f + f * 0.40f) * s;
    float lx2 = cx + (0.38f + f * 0.01f) * s;
    float ly2 = cy - (0.32f + f * 0.28f) * s;
    dl->AddLine(ImVec2(lx1, ly1), ImVec2(lx2, ly2), detCol, td);
  }

  // ══════════════════════════════════════════════════════════════════════════
  // COROA / HEADPIECE — chevrons empilhados (3 camadas)
  // ══════════════════════════════════════════════════════════════════════════

  dl->AddLine(ImVec2(cx - 0.38f * s, cy - 0.30f * s),
              ImVec2(cx, cy - 0.52f * s), mainCol, t);
  dl->AddLine(ImVec2(cx, cy - 0.52f * s),
              ImVec2(cx + 0.38f * s, cy - 0.30f * s), mainCol, t);

  dl->AddLine(ImVec2(cx - 0.32f * s, cy - 0.24f * s),
              ImVec2(cx, cy - 0.42f * s), detCol, td);
  dl->AddLine(ImVec2(cx, cy - 0.42f * s),
              ImVec2(cx + 0.32f * s, cy - 0.24f * s), detCol, td);

  dl->AddLine(ImVec2(cx - 0.26f * s, cy - 0.18f * s),
              ImVec2(cx, cy - 0.33f * s), detCol, td);
  dl->AddLine(ImVec2(cx, cy - 0.33f * s),
              ImVec2(cx + 0.26f * s, cy - 0.18f * s), detCol, td);

  // ══════════════════════════════════════════════════════════════════════════
  // CONTORNO DO ROSTO — hexagono alongado verticalmente
  // ══════════════════════════════════════════════════════════════════════════

  // Lado esquerdo
  dl->AddLine(ImVec2(cx - 0.38f * s, cy - 0.30f * s),
              ImVec2(cx - 0.34f * s, cy - 0.05f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.34f * s, cy - 0.05f * s),
              ImVec2(cx - 0.26f * s, cy + 0.15f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.26f * s, cy + 0.15f * s),
              ImVec2(cx - 0.14f * s, cy + 0.32f * s), mainCol, t);
  // Lado direito
  dl->AddLine(ImVec2(cx + 0.38f * s, cy - 0.30f * s),
              ImVec2(cx + 0.34f * s, cy - 0.05f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.34f * s, cy - 0.05f * s),
              ImVec2(cx + 0.26f * s, cy + 0.15f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.26f * s, cy + 0.15f * s),
              ImVec2(cx + 0.14f * s, cy + 0.32f * s), mainCol, t);
  // Queixo (V invertido)
  dl->AddLine(ImVec2(cx - 0.14f * s, cy + 0.32f * s),
              ImVec2(cx, cy + 0.44f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.14f * s, cy + 0.32f * s),
              ImVec2(cx, cy + 0.44f * s), mainCol, t);

  // ══════════════════════════════════════════════════════════════════════════
  // DIAMANTE NA TESTA
  // ══════════════════════════════════════════════════════════════════════════

  float dY = cy - 0.16f * s;
  float dS = 0.055f * s;
  ImVec2 diamond[4] = {
      ImVec2(cx, dY - dS * 1.3f),
      ImVec2(cx + dS, dY),
      ImVec2(cx, dY + dS * 1.3f),
      ImVec2(cx - dS, dY),
  };
  dl->AddPolyline(diamond, 4, mainCol, ImDrawFlags_Closed, t);

  // ══════════════════════════════════════════════════════════════════════════
  // OLHOS — angulares, alongados horizontalmente
  // ══════════════════════════════════════════════════════════════════════════

  float eyeY = cy - 0.03f * s;
  float ew = 0.12f * s, eh = 0.04f * s;

  // Olho esquerdo
  {
    float ex = cx - 0.17f * s;
    ImVec2 eL[4] = {
        ImVec2(ex - ew, eyeY),
        ImVec2(ex, eyeY - eh),
        ImVec2(ex + ew * 0.6f, eyeY),
        ImVec2(ex, eyeY + eh),
    };
    dl->AddPolyline(eL, 4, mainCol, ImDrawFlags_Closed, t);
    dl->AddCircleFilled(ImVec2(ex - 0.01f * s, eyeY), 0.02f * s, mainCol, 8);
  }

  // Olho direito
  {
    float ex = cx + 0.17f * s;
    ImVec2 eR[4] = {
        ImVec2(ex + ew, eyeY),
        ImVec2(ex, eyeY - eh),
        ImVec2(ex - ew * 0.6f, eyeY),
        ImVec2(ex, eyeY + eh),
    };
    dl->AddPolyline(eR, 4, mainCol, ImDrawFlags_Closed, t);
    dl->AddCircleFilled(ImVec2(ex + 0.01f * s, eyeY), 0.02f * s, mainCol, 8);
  }

  // ══════════════════════════════════════════════════════════════════════════
  // NARIZ — linha vertical + pequeno triangulo
  // ══════════════════════════════════════════════════════════════════════════

  dl->AddLine(ImVec2(cx, cy - 0.10f * s), ImVec2(cx, cy + 0.18f * s), mainCol,
              td);
  // Narinas (V invertido pequeno)
  dl->AddLine(ImVec2(cx - 0.035f * s, cy + 0.14f * s),
              ImVec2(cx, cy + 0.18f * s), mainCol, td);
  dl->AddLine(ImVec2(cx + 0.035f * s, cy + 0.14f * s),
              ImVec2(cx, cy + 0.18f * s), mainCol, td);

  // Linha horizontal do focinho
  dl->AddLine(ImVec2(cx - 0.10f * s, cy + 0.08f * s),
              ImVec2(cx + 0.10f * s, cy + 0.08f * s), detCol, td);

  // ══════════════════════════════════════════════════════════════════════════
  // BOCA / MANDIBULA
  // ══════════════════════════════════════════════════════════════════════════

  dl->AddLine(ImVec2(cx - 0.08f * s, cy + 0.24f * s),
              ImVec2(cx + 0.08f * s, cy + 0.24f * s), detCol, td);

  // ══════════════════════════════════════════════════════════════════════════
  // SIDE DRAPES (Nemes/headdress egipcio) com hachura diagonal
  // ══════════════════════════════════════════════════════════════════════════

  // Drape esquerdo — contorno
  dl->AddLine(ImVec2(cx - 0.34f * s, cy - 0.05f * s),
              ImVec2(cx - 0.44f * s, cy + 0.42f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.34f * s, cy - 0.05f * s),
              ImVec2(cx - 0.30f * s, cy + 0.42f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.44f * s, cy + 0.42f * s),
              ImVec2(cx - 0.40f * s, cy + 0.56f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.30f * s, cy + 0.42f * s),
              ImVec2(cx - 0.33f * s, cy + 0.56f * s), mainCol, t);
  dl->AddLine(ImVec2(cx - 0.40f * s, cy + 0.56f * s),
              ImVec2(cx - 0.33f * s, cy + 0.56f * s), mainCol, t);
  // Hachura diagonal esquerda
  for (int i = 0; i < 7; i++) {
    float f = i * 0.065f;
    float y1 = cy + (0.02f + f) * s;
    float y2 = y1 + 0.06f * s;
    float xOffset = -0.34f + f * 0.12f;
    dl->AddLine(ImVec2(cx + (xOffset - 0.08f) * s, y1),
                ImVec2(cx + (xOffset - 0.02f) * s, y2), detCol, 0.9f);
  }

  // Drape direito — contorno (espelho)
  dl->AddLine(ImVec2(cx + 0.34f * s, cy - 0.05f * s),
              ImVec2(cx + 0.44f * s, cy + 0.42f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.34f * s, cy - 0.05f * s),
              ImVec2(cx + 0.30f * s, cy + 0.42f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.44f * s, cy + 0.42f * s),
              ImVec2(cx + 0.40f * s, cy + 0.56f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.30f * s, cy + 0.42f * s),
              ImVec2(cx + 0.33f * s, cy + 0.56f * s), mainCol, t);
  dl->AddLine(ImVec2(cx + 0.40f * s, cy + 0.56f * s),
              ImVec2(cx + 0.33f * s, cy + 0.56f * s), mainCol, t);
  // Hachura diagonal direita
  for (int i = 0; i < 7; i++) {
    float f = i * 0.065f;
    float y1 = cy + (0.02f + f) * s;
    float y2 = y1 + 0.06f * s;
    float xOffset = 0.34f - f * 0.12f;
    dl->AddLine(ImVec2(cx + (xOffset + 0.08f) * s, y1),
                ImVec2(cx + (xOffset + 0.02f) * s, y2), detCol, 0.9f);
  }

  // ══════════════════════════════════════════════════════════════════════════
  // LINHAS DECORATIVAS PULSANTES (efeito dimensional)
  // ══════════════════════════════════════════════════════════════════════════

  float pulse = 0.5f + 0.5f * sinf(time * 2.0f);
  int pa = (int)(20 + 30 * pulse);
  ImU32 pCol = IM_COL32(235, 72, 120, pa);
  dl->AddLine(ImVec2(cx - 0.52f * s, cy - 0.55f * s),
              ImVec2(cx - 0.42f * s, cy - 0.38f * s), pCol, 1.0f);
  dl->AddLine(ImVec2(cx + 0.52f * s, cy - 0.55f * s),
              ImVec2(cx + 0.42f * s, cy - 0.38f * s), pCol, 1.0f);
  dl->AddLine(ImVec2(cx - 0.50f * s, cy + 0.30f * s),
              ImVec2(cx - 0.44f * s, cy + 0.20f * s), pCol, 1.0f);
  dl->AddLine(ImVec2(cx + 0.50f * s, cy + 0.30f * s),
              ImVec2(cx + 0.44f * s, cy + 0.20f * s), pCol, 1.0f);
}
