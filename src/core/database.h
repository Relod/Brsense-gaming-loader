// =============================================================================
// database.h — API Client para o Servidor BR Sense
// =============================================================================
// Comunicacao HTTP com o servidor backend via WinHTTP.
// Substitui a conexao direta com MariaDB por chamadas REST API.
//
// Endpoints:
//   POST /api/login   — Autenticacao + cheats licenciados
//   GET  /api/status  — Status do servidor
// =============================================================================

#pragma once

#include "hwid.h"

#include <string>
#include <vector>

// ── Struct de um cheat licenciado ───────────────────────────────────────────
struct CheatLicense {
  int id = 0;            ///< ID do cheat
  std::string game;      ///< Nome do jogo (ex: "Counter Strike 2")
  std::string name;      ///< Nome do cheat (ex: "Aimbot - Wallhack - ESP")
  std::string iconColor; ///< Cor do icone hex (ex: "#4FC3F7")
  std::string process;   ///< Nome do processo alvo (ex: "cs2.exe")
  std::string hash;      ///< SHA-256 esperado do payload
  std::string notes;     ///< Notas do admin sobre o cheat
  std::string
      timeLeft;       ///< Tempo restante (ex: "23d 14h", "LIFETIME", "EXPIRED")
  std::string status; ///< Status: "active", "expired", "suspended"
};

// ── Struct de informacoes do usuario ────────────────────────────────────────
struct UserInfo {
  std::string nickname; ///< Apelido do usuario
  std::string plan;     ///< Plano: "Free", "Premium", "VIP", "Lifetime"
  std::string hwid;     ///< Hardware ID vinculado
  std::string mac;      ///< MAC address vinculado
  std::string ip;       ///< IP registrado
};

// ── Struct de status do servidor ────────────────────────────────────────────
struct ServerStatus {
  bool online = false;
  std::string version;
  std::string region;
  std::string lastUpdate;
  int ping = 0;
};

// ── API Client ──────────────────────────────────────────────────────────────
class Database {
public:
  Database();
  ~Database();

  /// Conecta ao servidor backend (verifica se esta online).
  bool Connect(const std::string &host = "localhost", unsigned int port = 3000);

  /// Desconecta (limpa estado).
  void Disconnect();

  /// Retorna se o servidor esta acessivel.
  bool IsConnected() const;

  /// Autentica o usuario via POST /api/login.
  /// Preenche userInfo e cheats se bem-sucedido.
  /// Envia HardwareInfo para vinculacao/verificacao de HWID.
  bool Authenticate(const std::string &username, const std::string &password,
                    const HardwareInfo &hwInfo);

  /// Consulta status do servidor via GET /api/status.
  ServerStatus GetServerStatus();

  /// Envia ping de heartbeat via POST /api/heartbeat para manter sessao ativa.
  bool Heartbeat();

  /// Solicita reset de HWID via POST /api/hwid_reset.
  bool RequestHwidReset(const std::string &reason);

  /// Faz o download do payload do cheat via GET /api/download/:cheat_id.
  /// expectedSha256: se preenchido, valida integridade.
  bool DownloadCheat(int cheatId, std::vector<uint8_t> &outBuffer,
                     const std::string &expectedSha256 = "");

  /// Atualiza a lista de cheats do usuario via GET /api/cheats.
  /// Retorna true se os cheats foram atualizados com sucesso.
  bool RefreshCheats();

  /// Dados retornados apos login bem-sucedido
  const UserInfo &GetUserInfo() const { return m_userInfo; }
  const std::vector<CheatLicense> &GetCheats() const { return m_cheats; }
  const std::string &GetJwtToken() const { return m_jwtToken; }

  /// Ultima mensagem de erro
  const std::string &GetLastError() const { return m_lastError; }

private:
  /// Faz uma requisicao HTTP e retorna o body da resposta ou bytes.
  /// authType: 0 = Nenhum, 1 = Bearer Token
  std::string HttpRequest(const std::string &method, const std::string &path,
                          const std::string &body = "", int authType = 0);

  /// Requisicao HTTP que retorna bytes ignorando parsing de string
  bool HttpDownloadRequest(const std::string &path,
                           std::vector<uint8_t> &outBuffer);

  std::string m_host = "localhost";
  unsigned int m_port = 3000;
  bool m_connected = false;
  std::string m_lastError;

  // Dados do usuario logado
  UserInfo m_userInfo;
  std::vector<CheatLicense> m_cheats;
  std::string m_jwtToken; // Token JWT para rotas autenticadas
};
