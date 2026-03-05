
#include "../security/anti_debug.h"
#include "app.h"
#include "fonts.h"
#include "theme.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <cstdarg>
#include <ctime>
#include <d3d11.h>
#include <dbghelp.h>
#include <tchar.h>

#pragma comment(lib, "dbghelp.lib")

static void AppendLog(const char *fmt, ...) {
  char tmpPath[MAX_PATH] = {0};
  if (!GetTempPathA(MAX_PATH, tmpPath))
    strcpy_s(tmpPath, sizeof(tmpPath), "C:\\temp\\");
  char logPath[MAX_PATH];
  snprintf(logPath, sizeof(logPath), "%sIMGUI_CS_startup.log", tmpPath);
  FILE *f = fopen(logPath, "a");
  if (!f)
    return;
  va_list ap;
  va_start(ap, fmt);
  char buf[1024];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_s(&tmv, &t);
  char timestr[64];
  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tmv);
  fprintf(f, "%s - %s\n", timestr, buf);
  fclose(f);
}

static LONG WINAPI CrashHandler(struct _EXCEPTION_POINTERS *pExceptionInfo) {
  AppendLog("Unhandled exception, writing minidump...");
  char tmpPath[MAX_PATH] = {0};
  if (!GetTempPathA(MAX_PATH, tmpPath))
    strcpy_s(tmpPath, sizeof(tmpPath), "C:\\temp\\");
  char path[MAX_PATH];
  time_t t = time(nullptr);
  snprintf(path, sizeof(path), "%scrash_%lld.dmp", tmpPath, (long long)t);
  HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = pExceptionInfo;
    mei.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                      MiniDumpWithPrivateReadWriteMemory, &mei, nullptr,
                      nullptr);
    CloseHandle(hFile);
    AppendLog("Minidump written to %s", path);
  } else {
    AppendLog("Failed to create dump file %s", path);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

static void SetupCrashHandler() { SetUnhandledExceptionFilter(CrashHandler); }

HWND g_hWnd = nullptr;

ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;
static bool g_resizeRequested = false;
static UINT g_resizeW = 0;
static UINT g_resizeH = 0;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

  SetupCrashHandler();
  AppendLog("Application start (executable: %s)", "IMGUI_CS.exe");

#ifndef _DEBUG
  if (IsDebuggerPresent()) {
    AppendLog("Debugger detectado â€” pulando StartupSecurity (dev session)");
  } else {
    Security::StartupSecurity();
  }
#endif
  AppendLog("Security startup completed (debug=%d)",
#ifdef _DEBUG
            1
#else
            0
#endif
  );

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_CLASSDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"ImGuiCSClass";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassExW(&wc);
  AppendLog("RegisterClassExW() done");

  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);
  int winW = 1100;
  int winH = 700;
  int posX = (screenW - winW) / 2;
  int posY = (screenH - winH) / 2;

  HWND hwnd = CreateWindowExW(
      0, wc.lpszClassName, L"CS",
      WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
      posX, posY, winW, winH, nullptr, nullptr, wc.hInstance, nullptr);
  g_hWnd = hwnd;
  AppendLog("CreateWindowExW returned hwnd=%p", (void *)hwnd);

  if (!CreateDeviceD3D(hwnd)) {
    AppendLog("CreateDeviceD3D() failed");
    CleanupDeviceD3D();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 1;
  }
  AppendLog("CreateDeviceD3D() succeeded");

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = nullptr;

  ApplyCustomDarkTheme();

  Fonts::LoadFonts(io);

  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

  App app;
  app.Init();

  float clearColor[4] = {0.055f, 0.055f, 0.12f, 1.0f};

  bool running = true;
  while (running) {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
        running = false;
    }
    if (!running)
      break;

    if (g_resizeRequested) {
      CleanupRenderTarget();
      g_pSwapChain->ResizeBuffers(0, g_resizeW, g_resizeH, DXGI_FORMAT_UNKNOWN,
                                  0);
      CreateRenderTarget();
      g_resizeRequested = false;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    app.Render();

    if (app.ShouldExit()) {
      running = false;
      continue;
    }

    ImGui::Render();
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView,
                                            nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                               clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0);
  }

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  DestroyWindow(hwnd);
  UnregisterClassW(wc.lpszClassName, wc.hInstance);

  return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
  AppendLog("CreateDeviceD3D() start");
  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
  const D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0,
                                             D3D_FEATURE_LEVEL_10_0};

  UINT createDeviceFlags = 0;

  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
      &featureLevel, &g_pd3dDeviceContext);
  AppendLog("D3D11CreateDeviceAndSwapChain (HARDWARE) hr=0x%08X", (unsigned)hr);

  if (hr == DXGI_ERROR_UNSUPPORTED) {
    hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
        featureLevels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
        &featureLevel, &g_pd3dDeviceContext);
    AppendLog("D3D11CreateDeviceAndSwapChain (WARP) hr=0x%08X", (unsigned)hr);
  }

  if (FAILED(hr)) {
    AppendLog("CreateDeviceD3D failed hr=0x%08X", (unsigned)hr);
    return false;
  }

  CreateRenderTarget();
  return true;
}

void CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (g_pSwapChain) {
    g_pSwapChain->Release();
    g_pSwapChain = nullptr;
  }
  if (g_pd3dDeviceContext) {
    g_pd3dDeviceContext->Release();
    g_pd3dDeviceContext = nullptr;
  }
  if (g_pd3dDevice) {
    g_pd3dDevice->Release();
    g_pd3dDevice = nullptr;
  }
}

void CreateRenderTarget() {
  ID3D11Texture2D *pBackBuffer = nullptr;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  if (pBackBuffer) {
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                         &g_mainRenderTargetView);
    pBackBuffer->Release();
  }
}

void CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg) {
  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED)
      return 0;
    g_resizeW = (UINT)LOWORD(lParam);
    g_resizeH = (UINT)HIWORD(lParam);
    g_resizeRequested = true;
    return 0;

  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU)
      return 0;
    if ((wParam & 0xfff0) == SC_MAXIMIZE)
      return 0;
    break;

  case WM_NCLBUTTONDBLCLK:
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProcW(hWnd, msg, wParam, lParam);
}
