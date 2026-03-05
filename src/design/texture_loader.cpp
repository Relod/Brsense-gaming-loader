
#include "texture_loader.h"
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")

ImTextureID LoadTextureFromFile(const char *filename, ID3D11Device *device,
                                int *outWidth, int *outHeight) {
  if (!filename || !device)
    return 0;

  wchar_t wPath[MAX_PATH];
  MultiByteToWideChar(CP_UTF8, 0, filename, -1, wPath, MAX_PATH);

  CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  IWICImagingFactory *pFactory = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
  if (FAILED(hr))
    return 0;

  IWICBitmapDecoder *pDecoder = nullptr;
  hr = pFactory->CreateDecoderFromFilename(
      wPath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
  if (FAILED(hr)) {
    pFactory->Release();
    return 0;
  }

  IWICBitmapFrameDecode *pFrame = nullptr;
  hr = pDecoder->GetFrame(0, &pFrame);
  if (FAILED(hr)) {
    pDecoder->Release();
    pFactory->Release();
    return 0;
  }

  IWICFormatConverter *pConverter = nullptr;
  hr = pFactory->CreateFormatConverter(&pConverter);
  if (FAILED(hr)) {
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();
    return 0;
  }

  hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA,
                              WICBitmapDitherTypeNone, nullptr, 0.0f,
                              WICBitmapPaletteTypeCustom);
  if (FAILED(hr)) {
    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();
    return 0;
  }

  UINT width = 0, height = 0;
  pConverter->GetSize(&width, &height);

  UINT stride = width * 4;
  UINT bufferSize = stride * height;
  unsigned char *pixels = new unsigned char[bufferSize];
  hr = pConverter->CopyPixels(nullptr, stride, bufferSize, pixels);

  pConverter->Release();
  pFrame->Release();
  pDecoder->Release();
  pFactory->Release();

  if (FAILED(hr)) {
    delete[] pixels;
    return 0;
  }

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA subResource = {};
  subResource.pSysMem = pixels;
  subResource.SysMemPitch = stride;

  ID3D11Texture2D *pTexture = nullptr;
  hr = device->CreateTexture2D(&desc, &subResource, &pTexture);
  delete[] pixels;

  if (FAILED(hr))
    return 0;

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;

  ID3D11ShaderResourceView *pSRV = nullptr;
  hr = device->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);
  pTexture->Release();

  if (FAILED(hr))
    return 0;

  if (outWidth)
    *outWidth = (int)width;
  if (outHeight)
    *outHeight = (int)height;

  return (ImTextureID)(uintptr_t)pSRV;
}

void ReleaseTexture(ImTextureID texture) {
  if (texture) {
    auto *srv = (ID3D11ShaderResourceView *)(uintptr_t)texture;
    srv->Release();
  }
}
