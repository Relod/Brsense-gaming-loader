
#include "logo.h"
#include "imgui.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void DrawAnubisLogo(ImDrawList *dl, float cx, float cy, float scale, float time,
                    int alphaBase) {
  float s = scale;
  float R = s * 0.48f;
  float Ri = s * 0.42f;
  float Rc = s * 0.06f;
  float rot = time * 0.3f;

  ImU32 mainCol = IM_COL32(68, 204, 68, alphaBase);
  ImU32 fillCol = IM_COL32(68, 204, 68, (int)(alphaBase * 0.85f));
  ImU32 circleCol = IM_COL32(68, 204, 68, (int)(alphaBase * 0.6f));
  ImU32 circleFill = IM_COL32(68, 204, 68, (int)(alphaBase * 0.08f));

  dl->AddCircleFilled(ImVec2(cx, cy), R, circleFill, 48);

  for (int blade = 0; blade < 3; blade++) {
    float baseAngle = rot + blade * (2.0f * (float)M_PI / 3.0f);

    const int NPTS = 20;
    ImVec2 outer[NPTS];
    ImVec2 inner[NPTS];

    for (int i = 0; i < NPTS; i++) {
      float t_frac = (float)i / (float)(NPTS - 1);

      float r = Rc + (Ri - Rc) * t_frac;

      float sweep = 0.8f;
      float a = baseAngle + t_frac * sweep;

      float width = s * 0.08f * sinf(t_frac * (float)M_PI);

      float perpA = a + (float)M_PI * 0.5f;
      float px = cosf(perpA) * width;
      float py = sinf(perpA) * width;

      float mx = cx + cosf(a) * r;
      float my = cy + sinf(a) * r;

      outer[i] = ImVec2(mx + px, my + py);
      inner[i] = ImVec2(mx - px, my - py);
    }

    ImVec2 poly[NPTS * 2];
    for (int i = 0; i < NPTS; i++)
      poly[i] = outer[i];
    for (int i = 0; i < NPTS; i++)
      poly[NPTS + i] = inner[NPTS - 1 - i];

    dl->AddConvexPolyFilled(poly, NPTS * 2, fillCol);
    dl->AddPolyline(poly, NPTS * 2, mainCol, ImDrawFlags_Closed, 1.5f);
  }

  dl->AddCircleFilled(ImVec2(cx, cy), Rc * 1.2f,
                      IM_COL32(68, 204, 68, (int)(alphaBase * 0.3f)), 20);
  dl->AddCircle(ImVec2(cx, cy), Rc * 1.2f, mainCol, 20, 1.5f);

  dl->AddCircle(ImVec2(cx, cy), R, circleCol, 48, 2.0f);

  float pulse = 0.5f + 0.5f * sinf(time * 1.5f);
  int pa = (int)(10 + 20 * pulse);
  ImU32 pulseCol = IM_COL32(68, 204, 68, pa);
  dl->AddCircle(ImVec2(cx, cy), R + 3.0f, pulseCol, 48, 1.0f);
}
