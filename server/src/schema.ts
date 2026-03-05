import pool = require('./database');
import config = require('./config');

async function columnExists(table: string, column: string): Promise<boolean> {
    const [rows] = await pool.execute(
        `SELECT 1
           FROM information_schema.COLUMNS
          WHERE TABLE_SCHEMA = ?
            AND TABLE_NAME = ?
            AND COLUMN_NAME = ?
          LIMIT 1`,
        [config.DB_NAME, table, column]
    );
    return (rows as unknown[]).length > 0;
}

async function ensureCheatsColumns(): Promise<void> {
    const adds = [
        { name: 'icon_color', ddl: "ALTER TABLE cheats ADD COLUMN icon_color VARCHAR(16) DEFAULT '#FFFFFF'" },
        { name: 'process_name', ddl: 'ALTER TABLE cheats ADD COLUMN process_name VARCHAR(128) DEFAULT NULL' },
        { name: 'payload_path', ddl: 'ALTER TABLE cheats ADD COLUMN payload_path VARCHAR(255) DEFAULT NULL' },
        { name: 'payload_sha256', ddl: 'ALTER TABLE cheats ADD COLUMN payload_sha256 VARCHAR(64) DEFAULT NULL' },
        { name: 'notes', ddl: 'ALTER TABLE cheats ADD COLUMN notes TEXT DEFAULT NULL' },
        { name: 'injection_method', ddl: "ALTER TABLE cheats ADD COLUMN injection_method ENUM('LoadLibrary','InjectModule') DEFAULT 'LoadLibrary'" },
        { name: 'kill_processes', ddl: 'ALTER TABLE cheats ADD COLUMN kill_processes VARCHAR(512) DEFAULT NULL' },
        { name: 'launch_params', ddl: 'ALTER TABLE cheats ADD COLUMN launch_params VARCHAR(256) DEFAULT NULL' },
        { name: 'steam_app_id', ddl: 'ALTER TABLE cheats ADD COLUMN steam_app_id VARCHAR(32) DEFAULT NULL' },
        { name: 'requires_admin', ddl: 'ALTER TABLE cheats ADD COLUMN requires_admin TINYINT(1) DEFAULT 0' },
        { name: 'enabled', ddl: 'ALTER TABLE cheats ADD COLUMN enabled TINYINT(1) DEFAULT 1' },
    ];

    for (const col of adds) {
        if (!(await columnExists('cheats', col.name))) {
            await pool.execute(col.ddl);
            console.log(`[SCHEMA] Added cheats.${col.name}`);
        }
    }
}

async function ensureSessionsColumns(): Promise<void> {
    const adds = [
        { name: 'last_active', ddl: 'ALTER TABLE sessions ADD COLUMN last_active TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP' },
    ];
    for (const col of adds) {
        if (!(await columnExists('sessions', col.name))) {
            await pool.execute(col.ddl);
            console.log(`[SCHEMA] Added sessions.${col.name}`);
        }
    }
}

async function ensureUsersColumns(): Promise<void> {
    const adds = [
        { name: 'role', ddl: "ALTER TABLE users ADD COLUMN role ENUM('user','admin') DEFAULT 'user'" },
        { name: 'totp_secret', ddl: 'ALTER TABLE users ADD COLUMN totp_secret VARCHAR(64) DEFAULT NULL' },
    ];
    for (const col of adds) {
        if (!(await columnExists('users', col.name))) {
            await pool.execute(col.ddl);
            console.log(`[SCHEMA] Added users.${col.name}`);
        }
    }
}

async function ensureHwidResetTable(): Promise<void> {
    await pool.execute(`
        CREATE TABLE IF NOT EXISTS hwid_reset_requests (
          id         INT AUTO_INCREMENT PRIMARY KEY,
          user_id    INT NOT NULL,
          reason     TEXT,
          status     ENUM('pending','approved','rejected') DEFAULT 'pending',
          created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
          FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        ) ENGINE=InnoDB;
    `);
}

export async function ensureSchema(): Promise<void> {
    await ensureCheatsColumns();
    await ensureSessionsColumns();
    await ensureUsersColumns();
    await ensureHwidResetTable();
}
