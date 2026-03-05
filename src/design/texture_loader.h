#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "imgui.h"
#include <d3d11.h>

ImTextureID LoadTextureFromFile(const char *filename, ID3D11Device *device,
                                int *outWidth = nullptr,
                                int *outHeight = nullptr);

void ReleaseTexture(ImTextureID texture);
