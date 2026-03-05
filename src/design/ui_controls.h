
#pragma once

#include "strings.h"

struct ImDrawList;
struct ImVec2;

void DrawWindowControls(ImDrawList *dl, ImVec2 sz, bool lightBg = false);

void DrawLangFlags(ImDrawList *dl, Language &lang, float x, float y);

void HandleDrag(float h = 32);
