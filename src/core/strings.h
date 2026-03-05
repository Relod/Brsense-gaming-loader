
#pragma once

enum class Language {
  EN,
  PT
};

struct Strings {
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

  const char *loader;
  const char *logout;

  const char *nickname;
  const char *hwid;
  const char *mac;
  const char *ip;
  const char *plan;

  const char *serverStatus;
  const char *online;
  const char *offline;
  const char *region;
  const char *lastUpdate;
  const char *ping;
  const char *version;

  const char *yourCheats;
  const char *load;
  const char *update;
  const char *lifetime;
  const char *expired;
  const char *timeLeft;
  const char *noAccess;

  const char *activityLog;
  const char *welcomeBack;

  const char *cheatNotes;
  const char *back;
  const char *refresh;
  const char *downloading;
  const char *waitingProcess;
  const char *injecting;
  const char *injectionSuccess;
  const char *injectionFailed;

  const char *readyToPlay;
  const char *licenseActive;
  const char *licenseExpired;
  const char *cheatInfo;
  const char *noNotes;
  const char *requiresAdmin;
  const char *selectGame;
  const char *steamClosed;
  const char *injectPlay;
  const char *injectingText;

  const char *connecting;
  const char *serverOffline;
  const char *closingApp;
};

inline const Strings STRINGS_EN = {
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

    "Loader",
    "Sign Out",

    "Nickname",
    "HWID",
    "MAC",
    "IP",
    "Plan",

    "Server",
    "Online",
    "Offline",
    "Region",
    "Last Update",
    "Ping",
    "Version",

    "Your Cheats",
    "LOAD",
    "UPDATE",
    "LIFETIME",
    "EXPIRED",
    "Time Left",
    "No cheats available",

    "Activity Log",
    "Welcome back,",

    "Cheat Notes",
    "BACK",
    "REFRESH",
    "Downloading payload...",
    "Waiting for game process",
    "Injecting...",
    "Payload successfully injected!",
    "Injection failed",

    "Ready to play",
    "License active",
    "License expired",
    "CHEAT INFO",
    "No notes available.",
    "! Requires Admin",
    "Select your game:",
    "Steam closed for security.",
    "INJECT AND PLAY",
    "INJECTING...",

    "Connecting to server",
    "Server offline. Please start the BR Sense server.",
    "Closing application...",
};

inline const Strings STRINGS_PT = {
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

    "Loader",
    "Sair",

    "Nickname",
    "HWID",
    "MAC",
    "IP",
    "Plano",

    "Servidor",
    "Online",
    "Offline",
    "Regiao",
    "Ultima Atualizacao",
    "Ping",
    "Versao",

    "Seus Cheats",
    "CARREGAR",
    "ATUALIZAR",
    "VITALICIO",
    "EXPIRADO",
    "Tempo Restante",
    "Nenhum cheat disponivel",

    "Log de Atividade",
    "Bem-vindo de volta,",

    "Notas do Cheat",
    "VOLTAR",
    "ATUALIZAR",
    "Baixando payload...",
    "Aguardando processo do jogo",
    "Injetando...",
    "Payload injetado com sucesso!",
    "Falha na injecao",

    "Pronto para jogar",
    "Licenca ativa",
    "Licenca expirada",
    "INFO DO CHEAT",
    "Sem notas disponiveis.",
    "! Requer Admin",
    "Selecione o jogo:",
    "Steam fechada por seguranca.",
    "INJETAR E JOGAR",
    "INJETANDO...",

    "Conectando ao servidor",
    "Servidor offline. Inicie o servidor BR Sense.",
    "Fechando o aplicativo...",
};

inline const Strings &GetStrings(Language lang) {
  return lang == Language::PT ? STRINGS_PT : STRINGS_EN;
}
