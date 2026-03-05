#include "fonts.h"

#include <cstdio>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>



// =============================================================================
// BR Sense — Font System Implementation
// =============================================================================

namespace Fonts {

ImFont *Regular = nullptr;
ImFont *Small = nullptr;
ImFont *SemiBold = nullptr;
ImFont *Bold = nullptr;
ImFont *BoldLarge = nullptr;
ImFont *Mono = nullptr;
ImFont *Title = nullptr;

// Validate that file is a real TTF/OTF (not woff2 or HTML error)
static bool IsValidTTF(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  unsigned char magic[4] = {0};
  fread(magic, 1, 4, f);
  fclose(f);
  // TTF: 00 01 00 00 | TrueType: 74 72 75 65 | OTF: 4F 54 54 4F
  unsigned int m =
      (magic[0] << 24) | (magic[1] << 16) | (magic[2] << 8) | magic[3];
  return (m == 0x00010000 || m == 0x74727565 || m == 0x4F54544F);
}

// Try to find font file in multiple locations
static std::string FindFont(const char *filename) {
  char exePath[MAX_PATH] = {0};
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  char *lastSlash = strrchr(exePath, '\\');
  if (lastSlash)
    *(lastSlash + 1) = 0;

  // 1. fonts/ subdirectory next to executable
  std::string path1 = std::string(exePath) + "fonts\\" + filename;
  if (IsValidTTF(path1.c_str()))
    return path1;

  // 2. Directly next to executable
  std::string path1b = std::string(exePath) + filename;
  if (IsValidTTF(path1b.c_str()))
    return path1b;

  // 3. Relative to working directory (dev mode)
  std::string path2 = std::string("src\\design\\fonts\\") + filename;
  if (IsValidTTF(path2.c_str()))
    return path2;

  return "";
}

// Find a Windows system font as fallback
static std::string FindSystemFont(const char *fontName) {
  char winDir[MAX_PATH] = {0};
  GetWindowsDirectoryA(winDir, MAX_PATH);
  std::string path = std::string(winDir) + "\\Fonts\\" + fontName;
  if (IsValidTTF(path.c_str()))
    return path;
  return "";
}

bool LoadFonts(ImGuiIO &io) {
  ImFontConfig cfg;
  cfg.OversampleH = 2;
  cfg.OversampleV = 1;
  cfg.PixelSnapH = true;

  // Find custom font paths
  std::string interRegular = FindFont("Inter-Regular.ttf");
  std::string interSemiBold = FindFont("Inter-SemiBold.ttf");
  std::string interBold = FindFont("Inter-Bold.ttf");
  std::string monoRegular = FindFont("JetBrainsMono-Regular.ttf");

  // Fallback: use Windows system fonts
  // Segoe UI is a clean, modern font available on all Windows 10+
  std::string segoeUI = FindSystemFont("segoeui.ttf");
  std::string segoeSB = FindSystemFont("seguisb.ttf");
  std::string segoeBold = FindSystemFont("segoeuib.ttf");
  std::string consolas = FindSystemFont("consola.ttf");

  // Determine best available fonts
  std::string bodyFont = !interRegular.empty() ? interRegular
                         : !segoeUI.empty()    ? segoeUI
                                               : "";
  std::string semiFont = !interSemiBold.empty() ? interSemiBold
                         : !segoeSB.empty()     ? segoeSB
                                                : bodyFont;
  std::string boldFont = !interBold.empty()   ? interBold
                         : !segoeBold.empty() ? segoeBold
                                              : bodyFont;
  std::string monoFont = !monoRegular.empty() ? monoRegular
                         : !consolas.empty()  ? consolas
                                              : "";

  bool allCustom = true;

  // ── Body font (Regular / Small) ─────────────────────────────────────
  if (!bodyFont.empty()) {
    Regular = io.Fonts->AddFontFromFileTTF(bodyFont.c_str(), 16.0f, &cfg);
    Small = io.Fonts->AddFontFromFileTTF(bodyFont.c_str(), 13.0f, &cfg);
    if (bodyFont != interRegular)
      allCustom = false;
  } else {
    Regular = io.Fonts->AddFontDefault();
    Small = Regular;
    allCustom = false;
  }

  // ── SemiBold ────────────────────────────────────────────────────────
  if (!semiFont.empty()) {
    SemiBold = io.Fonts->AddFontFromFileTTF(semiFont.c_str(), 18.0f, &cfg);
  } else {
    SemiBold = Regular;
    allCustom = false;
  }

  // ── Bold / BoldLarge / Title ─────────────────────────────────────────
  if (!boldFont.empty()) {
    Bold = io.Fonts->AddFontFromFileTTF(boldFont.c_str(), 22.0f, &cfg);
    BoldLarge = io.Fonts->AddFontFromFileTTF(boldFont.c_str(), 28.0f, &cfg);
    Title = io.Fonts->AddFontFromFileTTF(boldFont.c_str(), 32.0f, &cfg);
  } else {
    Bold = BoldLarge = Title = Regular;
    allCustom = false;
  }

  // ── Mono ────────────────────────────────────────────────────────────
  if (!monoFont.empty()) {
    Mono = io.Fonts->AddFontFromFileTTF(monoFont.c_str(), 13.0f, &cfg);
  } else {
    Mono = Regular;
    allCustom = false;
  }

  // Build font atlas
  io.Fonts->Build();

  return allCustom;
}

} // namespace Fonts
