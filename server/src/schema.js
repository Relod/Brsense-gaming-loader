// =============================================================================
// schema.js - Small schema guard/migrations (non-destructive)
// =============================================================================

const pool = require('./database');
const config = require('./config');

async function columnExists(table, column) {
  const [rows] = await pool.execute(
    `SELECT 1
       FROM information_schema.COLUMNS
      WHERE TABLE_SCHEMA = ?
        AND TABLE_NAME = ?
        AND COLUMN_NAME = ?
      LIMIT 1`,
    [config.DB_NAME, table, column]
  );
  return rows.length > 0;
}

async function ensureCheatsColumns() {
  const adds = [
    {
      name: 'icon_color',
      ddl: "ALTER TABLE cheats ADD COLUMN icon_color VARCHAR(16) DEFAULT '#FFFFFF'",
    },
    {
      name: 'process_name',
      ddl: 'ALTER TABLE cheats ADD COLUMN process_name VARCHAR(128) DEFAULT NULL',
    },
    {
      name: 'payload_path',
      ddl: 'ALTER TABLE cheats ADD COLUMN payload_path VARCHAR(255) DEFAULT NULL',
    },
    {
      name: 'payload_sha256',
      ddl: 'ALTER TABLE cheats ADD COLUMN payload_sha256 VARCHAR(64) DEFAULT NULL',
    },
    {
      name: 'notes',
      ddl: 'ALTER TABLE cheats ADD COLUMN notes TEXT DEFAULT NULL',
    },
  ];

  for (const col of adds) {
    if (!(await columnExists('cheats', col.name))) {
      await pool.execute(col.ddl);
      console.log(`[SCHEMA] Added cheats.${col.name}`);
    }
  }
}

async function ensureSessionsColumns() {
  const adds = [
    {
      name: 'last_active',
      ddl: 'ALTER TABLE sessions ADD COLUMN last_active TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP',
    },
  ];

  for (const col of adds) {
    if (!(await columnExists('sessions', col.name))) {
      await pool.execute(col.ddl);
      console.log(`[SCHEMA] Added sessions.${col.name}`);
    }
  }
}

async function ensureSchema() {
  await ensureCheatsColumns();
  await ensureSessionsColumns();
}

module.exports = { ensureSchema };
