// =============================================================================
// hwid.h — Coleta de Informacoes de Hardware (HWID, MAC, IP)
// =============================================================================
// Coleta informacoes reais do hardware da maquina antes do login:
//   - HWID: Baseado no serial do volume do disco C: + nome do computador
//   - MAC:  Endereco MAC do adaptador de rede principal
//   - IP:   Endereco IP local do adaptador de rede principal
//
// Essas informacoes sao enviadas ao servidor no login para vinculacao.
// =============================================================================

#pragma once

#include <string>

// ── Struct com dados de hardware ────────────────────────────────────────────
struct HardwareInfo {
  std::string hwid; ///< Hardware ID unico (hash do volume serial + hostname)
  std::string mac;  ///< Endereco MAC (formato XX:XX:XX:XX:XX:XX)
  std::string ip;   ///< IP local (ex: 192.168.1.100)
};

/// Coleta todas as informacoes de hardware da maquina.
/// Deve ser chamado uma vez no inicio do programa, antes da tela de login.
HardwareInfo CollectHardwareInfo();
