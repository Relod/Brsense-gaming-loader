// =============================================================================
// session.h — Persistencia de Sessao de Login
// =============================================================================
// Salva e carrega credenciais de login em arquivo local para auto-login.
// O arquivo e armazenado em C:\temp\cs_login.dat protegido com DPAPI
// (CryptProtectData). Nada em texto plano ou XOR fraco.
//
// USO:
//   SessionSave("admin", "senha123");
//   SessionData data;
//   if (SessionLoad(data)) { /* usar data.username e data.password */ }
//   SessionClear();
// =============================================================================

#pragma once

#include <string>

/// Dados de uma sessao salva.
struct SessionData {
  std::string username; ///< Nome de usuario da sessao
  std::string password; ///< Senha da sessao (protegida no disco)
};

/// Salva credenciais no arquivo de sessao.
/// Cria o diretorio C:\temp se nao existir.
/// @param user Nome de usuario
/// @param pass Senha
void SessionSave(const char *user, const char *pass);

/// Carrega credenciais do arquivo de sessao.
/// @param out SessionData preenchido se o arquivo existir e for valido
/// @return true se as credenciais foram carregadas com sucesso
bool SessionLoad(SessionData &out);

/// Remove o arquivo de sessao (logout).
void SessionClear();
