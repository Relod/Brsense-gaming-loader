import mysql from 'mysql2/promise';
import { config } from './config';

const pool = mysql.createPool({
    host: config.DB_HOST,
    user: config.DB_USER,
    password: config.DB_PASS,
    database: config.DB_NAME,
    port: config.DB_PORT,
    waitForConnections: true,
    connectionLimit: 10,
    connectTimeout: 10000,
    enableKeepAlive: true,
    keepAliveInitialDelay: 30000,
});

export default pool;
