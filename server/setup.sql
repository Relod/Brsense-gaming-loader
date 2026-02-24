-- =============================================================================
-- setup.sql — Schema do banco de dados BR Sense (Reestruturado)
-- =============================================================================
-- Cria o banco, tabelas e dados de exemplo.
-- Executar: mysql -u root -p < setup.sql
--
-- PAYLOADS: As DLLs dos cheats ficam na pasta server/payloads/
-- O campo payload_path armazena o NOME DO ARQUIVO relativo à pasta payloads/
-- Exemplo: payload_path = 'cheat_1.dll' → server/payloads/cheat_1.dll
-- =============================================================================

CREATE DATABASE IF NOT EXISTS brsense;
USE brsense;

-- ── Ordem de DROP importa (foreign keys) ────────────────────────────────────
DROP TABLE IF EXISTS user_cheats;
DROP TABLE IF EXISTS cheats;
DROP TABLE IF EXISTS sessions;
DROP TABLE IF EXISTS hwid_reset_requests;
DROP TABLE IF EXISTS users;

-- ── Tabela de usuarios ──────────────────────────────────────────────────────
CREATE TABLE users (
  id         INT AUTO_INCREMENT PRIMARY KEY,
  username   VARCHAR(64)  NOT NULL UNIQUE,
  password   VARCHAR(256) NOT NULL,
  nickname   VARCHAR(64)  NOT NULL,
  plan       ENUM('Free','Premium','VIP','Lifetime') DEFAULT 'Free',
  hwid       VARCHAR(128) DEFAULT '',
  mac        VARCHAR(64)  DEFAULT '',
  ip         VARCHAR(64)  DEFAULT '',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- ── Tabela de cheats disponiveis ────────────────────────────────────────────
CREATE TABLE cheats (
  id             INT AUTO_INCREMENT PRIMARY KEY,
  game           VARCHAR(128) NOT NULL,
  name           VARCHAR(256) NOT NULL,
  icon_color     VARCHAR(16)  DEFAULT '#FFFFFF',
  process_name   VARCHAR(128) DEFAULT NULL COMMENT 'Nome do processo alvo (ex: cs2.exe)',
  payload_path   VARCHAR(255) DEFAULT NULL COMMENT 'Nome do arquivo na pasta payloads/',
  payload_sha256 VARCHAR(64)  DEFAULT NULL COMMENT 'SHA-256 do payload (auto-calculado)',
  notes          TEXT         DEFAULT NULL COMMENT 'Notas do admin exibidas no loader'
) ENGINE=InnoDB;

-- ── Tabela de licenças (vincula usuario <-> cheat) ──────────────────────────
CREATE TABLE user_cheats (
  id         INT AUTO_INCREMENT PRIMARY KEY,
  user_id    INT NOT NULL,
  cheat_id   INT NOT NULL,
  expires_at DATETIME DEFAULT NULL COMMENT 'NULL = lifetime',
  status     ENUM('active','expired','suspended') DEFAULT 'active',
  FOREIGN KEY (user_id)  REFERENCES users(id) ON DELETE CASCADE,
  FOREIGN KEY (cheat_id) REFERENCES cheats(id) ON DELETE CASCADE,
  UNIQUE KEY (user_id, cheat_id)
) ENGINE=InnoDB;

-- ── Tabela de sessões (Anti Account-Sharing) ────────────────────────────────
CREATE TABLE sessions (
  id          INT AUTO_INCREMENT PRIMARY KEY,
  user_id     INT NOT NULL,
  token       VARCHAR(512) NOT NULL UNIQUE,
  hwid        VARCHAR(128) NOT NULL,
  last_active TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- ── Tabela de solicitações de HWID Reset ────────────────────────────────────
CREATE TABLE hwid_reset_requests (
  id         INT AUTO_INCREMENT PRIMARY KEY,
  user_id    INT NOT NULL,
  reason     TEXT,
  status     ENUM('pending','approved','rejected') DEFAULT 'pending',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- =============================================================================
-- DADOS DE EXEMPLO
-- =============================================================================

-- Usuarios (HWID vazio = será vinculado no primeiro login)
INSERT INTO users (username, password, nickname, plan, hwid, mac, ip) VALUES
  ('admin',  'admin',   'Vandetta',  'Lifetime', '', '', ''),
  ('demo',   'demo123', 'DemoUser',  'Premium',  '', '', ''),
  ('dev',    'dev123',  'DevPlayer', 'VIP',      '', '', '');

-- Cheats (payload vazio = upload pelo painel admin)
INSERT INTO cheats (game, name, icon_color, process_name, notes) VALUES
  ('Counter Strike 2',  'Aimbot - Wallhack - ESP',    '#4FC3F7', 'cs2.exe',
   'v2.1 - Atualizado para o ultimo patch.\nAnti-cheat bypass ativo.\nNovas features: Triggerbot, Glow ESP.'),
  ('Valorant',          'Aimbot - Wallhack - ESP',    '#FF4081', 'VALORANT-Win64-Shipping.exe',
   'v1.5 - Compativel com o patch mais recente.\nRequer execucao como admin.'),
  ('League of Legends', 'Orbwalker - Evade',          '#66BB6A', 'League of Legends.exe',
   'v3.0 - Orbwalker otimizado.\nEvade com predicao avancada.'),
  ('Fortnite',          'Aimbot - ESP - BuildAssist', '#FFA726', 'FortniteClient-Win64-Shipping.exe',
   'v1.2 - Build assist melhorado.\nESP com distancia e vida.'),
  ('Apex Legends',      'Aimbot - ESP - Radar',       '#AB47BC', 'r5apex.exe',
   'v2.0 - Radar minimap integrado.\nAimbot com smooth ajustavel.');

-- Licenças do admin: CS2 (Lifetime), Valorant (23d), LoL (Expired)
INSERT INTO user_cheats (user_id, cheat_id, expires_at, status) VALUES
  (1, 1, NULL, 'active'),
  (1, 2, DATE_ADD(NOW(), INTERVAL 23 DAY), 'active'),
  (1, 3, DATE_SUB(NOW(), INTERVAL 5 DAY), 'expired');

-- Licenças do demo: CS2 (7d), Fortnite (2d)
INSERT INTO user_cheats (user_id, cheat_id, expires_at, status) VALUES
  (2, 1, DATE_ADD(NOW(), INTERVAL 7 DAY), 'active'),
  (2, 4, DATE_ADD(NOW(), INTERVAL 2 DAY), 'active');

-- Licenças do dev: CS2 (Lifetime), Valorant (30d), Apex (15d)
INSERT INTO user_cheats (user_id, cheat_id, expires_at, status) VALUES
  (3, 1, NULL, 'active'),
  (3, 2, DATE_ADD(NOW(), INTERVAL 30 DAY), 'active'),
  (3, 5, DATE_ADD(NOW(), INTERVAL 15 DAY), 'active');
