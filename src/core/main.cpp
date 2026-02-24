// =============================================================================
// main.cpp — Ponto de Entrada da Aplicacao
// =============================================================================
// Este arquivo inicializa:
//   1. A janela Win32 (borderless, sem barra de titulo)
//   2. O dispositivo DirectX 11 (renderizacao)
//   3. O Dear ImGui (interface grafica imediata)
//   4. O loop principal (processar mensagens, renderizar, apresentar)
//
// A janela usa WS_POPUP para remover a barra de titulo do Windows.
// Os botoes de fechar/minimizar/maximizar sao desenhados pelo ImGui (app.cpp).
// =============================================================================

#include "../security/anti_debug.h"
#include "app.h"
#include "theme.h"


#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <d3d11.h>
#include <tchar.h>
#include <dbghelp.h>
#include <cstdarg>
#include <ctime>

#pragma comment(lib, "dbghelp.lib")

// Simple startup/crash logging helper
static void AppendLog(const char *fmt, ...) {
  char tmpPath[MAX_PATH] = {0};
  if (!GetTempPathA(MAX_PATH, tmpPath))
    strcpy_s(tmpPath, sizeof(tmpPath), "C:\\temp\\");
  char logPath[MAX_PATH];
  snprintf(logPath, sizeof(logPath), "%sIMGUI_CS_startup.log", tmpPath);
  FILE *f = fopen(logPath, "a");
  if (!f) return;
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

// Crash handler that writes a minidump
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
                      MiniDumpWithPrivateReadWriteMemory, &mei, nullptr, nullptr);
    CloseHandle(hFile);
    AppendLog("Minidump written to %s", path);
  } else {
    AppendLog("Failed to create dump file %s", path);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

static void SetupCrashHandler() {
  SetUnhandledExceptionFilter(CrashHandler);
}

// ── Variaveis Globais ────────────────────────────────────────────────────────
HWND g_hWnd = nullptr; // Handle da janela — usado pelo app.cpp para controles

// ── Variaveis do DirectX 11 ──────────────────────────────────────────────────
static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;
static bool g_resizeRequested = false; // Flag de redimensionamento pendente
static UINT g_resizeW = 0;             // Nova largura solicitada
static UINT g_resizeH = 0;             // Nova altura solicitada

// ── Declaracoes de Funcoes ───────────────────────────────────────────────────
bool CreateDeviceD3D(HWND hWnd); // Cria o dispositivo e swap chain
void CleanupDeviceD3D();         // Libera recursos do DirectX
void CreateRenderTarget();       // Cria o render target view
void CleanupRenderTarget();      // Libera o render target view
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// =============================================================================
// WINMAIN — Ponto de entrada do Windows
// =============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

  // Setup crash handler and startup log
  SetupCrashHandler();
  AppendLog("Application start (executable: %s)", "IMGUI_CS.exe");

  // Ativar as rotinas de Anti-Debug, Anti-Dump e Obfuscação na RAM.
  // Em Release, ignore se um debugger estiver anexado (permite F5 no VS).
#ifndef _DEBUG
  if (IsDebuggerPresent()) {
    AppendLog("Debugger detectado — pulando StartupSecurity (dev session)");
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

  // ── Registrar a classe da janela ─────────────────────────────────────────
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_CLASSDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"ImGuiCSClass";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassExW(&wc);
  AppendLog("RegisterClassExW() done");

  // ── Criar janela centralizada na tela ────────────────────────────────────
  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);
  int winW = 1100; // largura inicial
  int winH = 700;  // altura inicial
  int posX = (screenW - winW) / 2;
  int posY = (screenH - winH) / 2;

  // WS_POPUP = janela sem barra de titulo (borderless)
  HWND hwnd = CreateWindowExW(
      0, wc.lpszClassName, L"CS",
      WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
      posX, posY, winW, winH, nullptr, nullptr, wc.hInstance, nullptr);
  g_hWnd = hwnd;
  AppendLog("CreateWindowExW returned hwnd=%p", (void *)hwnd);

  // ── Inicializar DirectX 11 ───────────────────────────────────────────────
  if (!CreateDeviceD3D(hwnd)) {
    AppendLog("CreateDeviceD3D() failed");
    CleanupDeviceD3D();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 1;
  }
  AppendLog("CreateDeviceD3D() succeeded");

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  // ── Configurar ImGui ─────────────────────────────────────────────────────
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // navegacao por teclado
  io.IniFilename = nullptr; // desabilitar imgui.ini (nao salvar layout)

  // Aplicar o tema escuro customizado (definido em theme.cpp)
  ApplyCustomDarkTheme();

  // Inicializar backends (Win32 + DirectX 11)
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

  // ── Criar instancia da aplicacao ─────────────────────────────────────────
  App app;
  app.Init();

  // Cor de limpeza do fundo (tema escuro)
  float clearColor[4] = {0.055f, 0.055f, 0.12f, 1.0f};

  // ── Loop Principal ───────────────────────────────────────────────────────
  bool running = true;
  while (running) {
    // Processar mensagens do Windows
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
        running = false;
    }
    if (!running)
      break;

    // Tratar redimensionamento da janela
    if (g_resizeRequested) {
      CleanupRenderTarget();
      g_pSwapChain->ResizeBuffers(0, g_resizeW, g_resizeH, DXGI_FORMAT_UNKNOWN,
                                  0);
      CreateRenderTarget();
      g_resizeRequested = false;
    }

    // Iniciar novo frame ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Renderizar a aplicacao (tela de login ou produtos)
    app.Render();

    if (app.ShouldExit()) {
      running = false;
      continue;
    }

    // Renderizar e apresentar
    ImGui::Render();
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView,
                                            nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                               clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Apresentar (VSync ligado = 1 frame de espera)
    g_pSwapChain->Present(1, 0);
  }

  // ── Limpeza de Recursos ──────────────────────────────────────────────────
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  DestroyWindow(hwnd);
  UnregisterClassW(wc.lpszClassName, wc.hInstance);

  return 0;
}

// =============================================================================
// FUNCOES AUXILIARES DO DIRECTX 11
// =============================================================================

/// Cria o dispositivo D3D11, o contexto e a swap chain.
/// Tenta primeiro com driver de hardware; se nao suportado, usa WARP
/// (software).
bool CreateDeviceD3D(HWND hWnd) {
  AppendLog("CreateDeviceD3D() start");
  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0; // usar tamanho da janela
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
  const D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_0, // DirectX 11.0
      D3D_FEATURE_LEVEL_10_0  // fallback: DirectX 10.0
  };

  UINT createDeviceFlags = 0;

  // Tentar criar com driver de hardware
  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
      &featureLevel, &g_pd3dDeviceContext);
  AppendLog("D3D11CreateDeviceAndSwapChain (HARDWARE) hr=0x%08X", (unsigned)hr);

  // Se hardware nao suportado, tentar driver WARP (renderizacao por software)
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

/// Libera todos os recursos do DirectX.
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

/// Cria o render target view a partir do back buffer da swap chain.
void CreateRenderTarget() {
  ID3D11Texture2D *pBackBuffer = nullptr;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  if (pBackBuffer) {
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                         &g_mainRenderTargetView);
    pBackBuffer->Release();
  }
}

/// Libera o render target view.
void CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

// =============================================================================
// PROCEDIMENTO DA JANELA (WndProc)
// =============================================================================
// Processa mensagens do Windows. As mensagens do ImGui sao processadas
// primeiro pelo ImGui_ImplWin32_WndProcHandler.
// =============================================================================

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // Deixar o ImGui processar a mensagem primeiro
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg) {
  case WM_SIZE:
    // Redimensionamento da janela — agendar recriacao do render target
    if (wParam == SIZE_MINIMIZED)
      return 0;
    g_resizeW = (UINT)LOWORD(lParam);
    g_resizeH = (UINT)HIWORD(lParam);
    g_resizeRequested = true;
    return 0;

  case WM_SYSCOMMAND:
    // Desabilitar menu do ALT (impedir que a tecla ALT abra o menu do sistema)
    if ((wParam & 0xfff0) == SC_KEYMENU)
      return 0;
    // Bloquear maximize por double-click no caption
    if ((wParam & 0xfff0) == SC_MAXIMIZE)
      return 0;
    break;

  case WM_NCLBUTTONDBLCLK:
    // Bloquear double-click no fake HTCAPTION (impede maximize do Windows)
    return 0;

  case WM_DESTROY:
    // Janela sendo destruida — encerrar o loop principal
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProcW(hWnd, msg, wParam, lParam);
}
