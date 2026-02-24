// =============================================================================
// config.h — Carrega configuracoes do loader (host/porta do backend)
// =============================================================================
// Formato simples (INI-like):
//   host=localhost
//   port=3000
//
// Arquivo esperado: C:\temp\brsense_config.txt (ou mesmo diretorio do exe).
// Se nao existir ou falhar, usa defaults.
// =============================================================================
#pragma once

#include <string>

struct LoaderConfig {
  std::string host = "localhost";
  unsigned int port = 3000;
};

/// Tenta carregar configuracao de:
///   1) C:\temp\brsense_config.txt
///   2) ./brsense_config.txt (mesmo diretorio do exe)
/// Retorna config preenchida; em falha usa defaults.
LoaderConfig LoadConfig();
