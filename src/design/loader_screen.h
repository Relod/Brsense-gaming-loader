#pragma once

#include "imgui.h"
#include <future>

struct AppContext;

class LoaderScreen {
public:
  LoaderScreen();

  void Render(AppContext &ctx);

private:
  // Refactored render methods
  void DrawTopBar(AppContext &ctx, ImVec2 dp, float time);
  void DrawSidebar(AppContext &ctx, ImVec2 dp, float time);
  void DrawGameContent(AppContext &ctx, ImVec2 wPos, float mainW,
                       float contentH, float time);
  void DrawActivityLog(AppContext &ctx, ImDrawList *dl, ImVec2 logMin,
                       ImVec2 logMax);
  void DrawCheatInfoPanel(AppContext &ctx, ImDrawList *dl, ImVec2 panelMin,
                          ImVec2 panelMax);

  int m_selectedCheatIdx = 0;
  std::future<void> m_injectionFuture;
};
