// =============================================================================
// config.js — Configuração centralizada do servidor BR Sense
// =============================================================================

require('dotenv').config({ path: require('path').join(__dirname, '..', '.env') });
const path = require('path');

module.exports = {
    // Servidor
    PORT: parseInt(process.env.SERVER_PORT) || 3000,

    // Banco de dados
    DB_HOST: process.env.DB_HOST || 'localhost',
    DB_USER: process.env.DB_USER || 'root',
    DB_PASS: process.env.DB_PASS || '123321',
    DB_NAME: process.env.DB_NAME || 'brsense',
    DB_PORT: parseInt(process.env.DB_PORT) || 3306,

    // JWT
    JWT_SECRET: process.env.JWT_SECRET || 'fallback_secret_key',
    JWT_EXPIRES: process.env.JWT_EXPIRES || '4h',

    // Admin
    ADMIN_KEY: process.env.ADMIN_KEY || 'change_me_admin',

    // Payloads (DLLs dos cheats)
    PAYLOADS_DIR: path.join(__dirname, '..', 'payloads'),
};
