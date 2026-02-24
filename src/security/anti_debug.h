// =============================================================================
// anti_debug.h — Segurança e Profiling do Processo
// =============================================================================

#pragma once

namespace Security {
/// Inicia as checagens anti-debug (Threads para checagem em background)
void StartupSecurity();

/// Apaga os headers do PE/NT (DOS Header MZ00) do processo na RAM
/// Impede/dificulta bastante Scylla, ExtremeInjector, e Process Hacker dumping.
void ErasePEHeader();

/// Realiza uma varredura sincrona manual para saber se esta sendo depurado.
/// Retorna true se um debugger/VM suspeita for encontrado.
bool CheckForDebugger();
} // namespace Security
