// =============================================================================
// strings.h — Localizacao / Internacionalizacao (i18n)
// =============================================================================
// Header-only com todas as strings usadas no loader. Suporta EN e PT-BR.
// =============================================================================

#pragma once

// Idiomas suportados
enum class Language {
  EN, ///< Ingles
  PT  ///< Portugues (Brasil)
};

// Struct agregando todas as strings da UI
struct Strings {
  // Login
  const char *welcome;
  const char *signIn;
  const char *username;
  const char *password;
  const char *keepLogged;
  const char *loginBtn;
  const char *forgotPass;
  const char *fillFields;
  const char *wrongCreds;
  const char *tagline;

  // Loader — Top Bar
  const char *loader;
  const char *logout;

  // Loader — User Info
  const char *nickname;
  const char *hwid;
  const char *mac;
  const char *ip;
  const char *plan;

  // Loader — Server Status
  const char *serverStatus;
  const char *online;
  const char *offline;
  const char *region;
  const char *lastUpdate;
  const char *ping;
  const char *version;

  // Loader — Cheat List
  const char *yourCheats;
  const char *load;
  const char *update;
  const char *lifetime;
  const char *expired;
  const char *timeLeft;
  const char *noAccess;

  // Loader — Log Console
  const char *activityLog;
  const char *welcomeBack;

  // Loader — Detail View & Injection
  const char *cheatNotes;
  const char *back;
  const char *refresh;
  const char *downloading;
  const char *waitingProcess;
  const char *injecting;
  const char *injectionSuccess;
  const char *injectionFailed;

  // Connecting Screen
  const char *connecting;
  const char *serverOffline;
  const char *closingApp;
};

// ---- Strings em Ingles ------------------------------------------------------
inline const Strings STRINGS_EN = {
    // Login
    "Welcome",
    "Sign in to your account",
    "Username",
    "Password",
    "Keep me logged in",
    "SIGN IN",
    "Forgot your password?",
    "Please fill in all fields.",
    "Invalid username or password.",
    "BR Sense - Cheat Loader",

    // Loader — Top Bar
    "Loader",
    "Sign Out",

    // Loader — User Info
    "Nickname",
    "HWID",
    "MAC",
    "IP",
    "Plan",

    // Loader — Server Status
    "Server",
    "Online",
    "Offline",
    "Region",
    "Last Update",
    "Ping",
    "Version",

    // Loader — Cheat List
    "Your Cheats",
    "LOAD",
    "UPDATE",
    "LIFETIME",
    "EXPIRED",
    "Time Left",
    "No cheats available",

    // Loader — Log Console
    "Activity Log",
    "Welcome back,",

    // Loader — Detail View & Injection
    "Cheat Notes",
    "BACK",
    "REFRESH",
    "Downloading payload...",
    "Waiting for game process",
    "Injecting...",
    "Payload successfully injected!",
    "Injection failed",

    // Connecting Screen
    "Connecting to server",
    "Server offline. Please start the BR Sense server.",
    "Closing application...",
};

// ---- Strings em Portugues (Brasil) -----------------------------------------
inline const Strings STRINGS_PT = {
    // Login
    "Bem-vindo",
    "Entre na sua conta",
    "Usuario",
    "Senha",
    "Manter conectado",
    "ENTRAR",
    "Esqueceu sua senha?",
    "Preencha todos os campos.",
    "Usuario ou senha incorretos.",
    "BR Sense - Cheat Loader",

    // Loader — Top Bar
    "Loader",
    "Sair",

    // Loader — User Info
    "Nickname",
    "HWID",
    "MAC",
    "IP",
    "Plano",

    // Loader — Server Status
    "Servidor",
    "Online",
    "Offline",
    "Regiao",
    "Ultima Atualizacao",
    "Ping",
    "Versao",

    // Loader — Cheat List
    "Seus Cheats",
    "CARREGAR",
    "ATUALIZAR",
    "VITALICIO",
    "EXPIRADO",
    "Tempo Restante",
    "Nenhum cheat disponivel",

    // Loader — Log Console
    "Log de Atividade",
    "Bem-vindo de volta,",

    // Loader — Detail View & Injection
    "Notas do Cheat",
    "VOLTAR",
    "ATUALIZAR",
    "Baixando payload...",
    "Aguardando processo do jogo",
    "Injetando...",
    "Payload injetado com sucesso!",
    "Falha na injecao",

    // Connecting Screen
    "Conectando ao servidor",
    "Servidor offline. Inicie o servidor BR Sense.",
    "Fechando o aplicativo...",
};

// Helper para obter o idioma
inline const Strings &GetStrings(Language lang) {
  return lang == Language::PT ? STRINGS_PT : STRINGS_EN;
}
