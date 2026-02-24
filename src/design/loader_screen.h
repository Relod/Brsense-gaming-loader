// =============================================================================
// loader_screen.h — Tela do Loader de Cheats
// =============================================================================
// Renderiza o painel principal do loader:
//   - TopBar com logo, nome do usuario, logout e controles
//   - Banner de boas-vindas com gradiente
//   - Cards de estatisticas (Cheats, Online, Servidor)
//   - Painel de noticias e painel de acao com botao INJECT
// =============================================================================

#pragma once

struct AppContext; // forward declaration (definido em app.h)

/// Tela do loader de cheats.
class LoaderScreen {
public:
  LoaderScreen();

  /// Renderiza a tela do loader completa.
  /// @param ctx Contexto compartilhado da aplicacao
  void Render(AppContext &ctx);

private:
  int m_selectedCheatIdx = 0;
};
