// =============================================================================
// login_screen.h — Tela de Login
// =============================================================================
// Renderiza a tela de login split-screen:
//   - Esquerda (40%): Branding com logo Anubis + particulas animadas
//   - Direita (60%): Formulario de login com campos arredondados
//
// A classe LoginScreen encapsula todo o estado visual do login
// (particulas, campos de input, mensagens de erro).
// =============================================================================

#pragma once

#include <string>

struct AppContext; // forward declaration (definido em app.h)

/// Tela de login com animacoes e formulario.
class LoginScreen {
public:
  LoginScreen();

  /// Renderiza a tela de login completa.
  /// @param ctx Contexto compartilhado da aplicacao
  void Render(AppContext &ctx);

private:
  // ── Estado dos inputs ─────────────────────────────────────────────────────
  char m_username[64] = {};
  char m_password[128] = {};
  std::string m_errorMsg;
  float m_errorTimer = 0.0f;

  // ── Particulas de fundo ───────────────────────────────────────────────────
  struct Particle {
    float x, y, vx, vy, r;
    int alpha;
  };
  Particle m_particles[40] = {};
  bool m_particlesInit = false;
};
