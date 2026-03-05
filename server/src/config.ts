import 'dotenv/config';
import path from 'path';
import crypto from 'crypto';

export const config = {
    PORT: parseInt(process.env.SERVER_PORT || '3000'),

    DB_HOST: process.env.DB_HOST || 'localhost',
    DB_USER: process.env.DB_USER || 'root',
    DB_PASS: process.env.DB_PASS || '',
    DB_NAME: process.env.DB_NAME || 'brsense',
    DB_PORT: parseInt(process.env.DB_PORT || '3306'),

    JWT_SECRET: process.env.JWT_SECRET || (() => {
        console.warn('[SECURITY] JWT_SECRET not set in .env — using random fallback (sessions will not persist across restarts)');
        return crypto.randomBytes(32).toString('hex');
    })(),
    JWT_EXPIRES: process.env.JWT_EXPIRES || '4h',

    ADMIN_KEY: process.env.ADMIN_KEY || 'change_me_admin',

    BCRYPT_ROUNDS: parseInt(process.env.BCRYPT_ROUNDS || '12'),

    CORS_ORIGINS: process.env.CORS_ORIGINS
        ? process.env.CORS_ORIGINS.split(',').map(s => s.trim())
        : ['http://localhost:3000', 'http://127.0.0.1:3000'],

    PAYLOADS_DIR: path.join(__dirname, '..', 'payloads'),
};

export default config;
