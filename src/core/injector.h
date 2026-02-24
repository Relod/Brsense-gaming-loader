// =============================================================================
// injector.h — Definição do Manual Mapper
// =============================================================================

#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>
#include <vector>


namespace Injector {
/// Estrutura para os dados a serem passados para a Thread Remota (O Stub)
struct ManualMappingData {
  HINSTANCE hLibModule;    // Endereço base da imagem do LoadLibraryA (usado no
                           // fallback se necessário)
  FARPROC pLoadLibraryA;   // Endereço proc kernel32
  FARPROC pGetProcAddress; // Endereço proc kernel32
  FARPROC pRtlAddFunctionTable; // Endereço proc da ntdll
  BYTE *pBaseAddress;           // Endereço alocado da base da DLL local
};

/// Encontra o Process ID (PID) do alvo baseado no nome EXE
DWORD GetProcessIdByName(const std::wstring &processName);

/// Mata o processo especificado pelo nome se estiver rodando
void KillProcessByName(const std::wstring &processName);

/// Realiza o Manual Mapping da DLL (Buffer) para dentro do processo
/// especificado. Retorna TRUE caso a DLL tenha sido ativada perfeitamente na
/// RAM remota.
bool ManualMap(const std::wstring &processName,
               const std::vector<uint8_t> &dllBytes);

/// Ultimo erro textual da injecao (válido após ManualMap retornar false).
const char *GetLastError();
} // namespace Injector
