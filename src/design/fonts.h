#pragma once

#include "imgui.h"

// =============================================================================
// BR Sense — Font System
// Loads Inter (Regular/SemiBold/Bold) + JetBrains Mono from TTF files.
// Global font pointers accessible from any screen.
// =============================================================================

namespace Fonts {

// Global font pointers (set after LoadFonts)
extern ImFont *Regular;   // Inter Regular 16px — body text
extern ImFont *Small;     // Inter Regular 13px — captions, labels
extern ImFont *SemiBold;  // Inter SemiBold 18px — subtitles
extern ImFont *Bold;      // Inter Bold 22px — headings
extern ImFont *BoldLarge; // Inter Bold 28px — large titles
extern ImFont *Mono;      // JetBrains Mono 13px — log/terminal
extern ImFont *Title;     // Inter Bold 32px — splash titles

// Call once after ImGui::CreateContext(), before the render loop.
// Returns true on success.
bool LoadFonts(ImGuiIO &io);

// Helper to push/pop font conveniently
struct FontScope {
  FontScope(ImFont *font) {
    if (font)
      ImGui::PushFont(font);
    m_pushed = (font != nullptr);
  }
  ~FontScope() {
    if (m_pushed)
      ImGui::PopFont();
  }
  bool m_pushed;
};

} // namespace Fonts

// Convenience macros
#define FONT_REGULAR Fonts::Regular
#define FONT_SMALL Fonts::Small
#define FONT_SEMIBOLD Fonts::SemiBold
#define FONT_BOLD Fonts::Bold
#define FONT_BOLD_LG Fonts::BoldLarge
#define FONT_MONO Fonts::Mono
#define FONT_TITLE Fonts::Title
