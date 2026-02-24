// =============================================================================
// anti_debug.cpp — Implementacao da Seguranca
// =============================================================================

#include "anti_debug.h"
#include <chrono>
#include <thread>
#include <windows.h>
#include <winternl.h>
#include <intrin.h>


namespace Security {

// Helper: Encontra a base do processo atual (como HMODULE do main) sem depender
// de GetModuleHandleA() com strings
void *GetModuleBaseAddress() {
#ifdef _WIN64
  // No x64, a PEB fica em GS:[0x60]
  PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
  // No x86, a PEB fica em FS:[0x30]
  PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif
  return pPeb->Reserved3[1];
}

void ErasePEHeader() {
  DWORD oldProtect;
  char *baseAddress = (char *)GetModuleBaseAddress();

  // Alterar permissão de memória pra poder sobrescrever o header da nossa
  // propria imagem 4096 é o tamanho padrão de uma página do Windows System,
  // geralmente abriga o header
  VirtualProtect(baseAddress, 4096, PAGE_EXECUTE_READWRITE, &oldProtect);

  // Zerar as assinaturas clássicas "MZ" e o header DOS
  ZeroMemory(baseAddress, 4096);

  // Restaurar permissão
  VirtualProtect(baseAddress, 4096, oldProtect, &oldProtect);
}

bool CheckForDebugger() {
  bool isDebugged = false;

  // 1. O básico da API WIN32
  if (IsDebuggerPresent())
    return true;

  // 2. A API estendida
  CheckRemoteDebuggerPresent(GetCurrentProcess(), (PBOOL)&isDebugged);
  if (isDebugged)
    return true;

// 3. O avançado: Checar a PEB diretamente
#ifdef _WIN64
  PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
  PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif

  if (pPeb->BeingDebugged == 1)
    return true;

  return false;
}

// A thread em loop constante
void SecurityLoop() {
  while (true) {
    if (CheckForDebugger()) {
      // Se detectar que um debugger tentou se atachar enquanto já carregou,
      // crasha a aplicação violentamente gerando uma falha grave (proteção)
      exit(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  }
}

void StartupSecurity() {
  // Apagar as referências a ser um Executável na memória
  ErasePEHeader();

  // Se abrir com um debugger nativo, sai já na startup
  if (CheckForDebugger()) {
    exit(1);
  }

  // Criar thread desacoplada que checará continuamente pro resto de sua vida
  std::thread(SecurityLoop).detach();
}
} // namespace Security
