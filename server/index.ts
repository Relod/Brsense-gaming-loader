import express from 'express';
import cors from 'cors';
import helmet from 'helmet';
import fs from 'fs';
import path from 'path';
import config = require('./src/config');
import { ensureSchema } from './src/schema';

if (!fs.existsSync(config.PAYLOADS_DIR)) {
    fs.mkdirSync(config.PAYLOADS_DIR, { recursive: true });
}

const app = express();

// ── Security Headers ────────────────────────────────────────────────────────
app.use(helmet({
    contentSecurityPolicy: false,
    crossOriginEmbedderPolicy: false,
}));
app.use(cors({
    origin: config.CORS_ORIGINS,
    methods: ['GET', 'POST', 'PUT', 'DELETE'],
    allowedHeaders: ['Content-Type', 'Authorization', 'x-admin-key'],
}));
app.use(express.json({ limit: '10mb' }));

// ── Request Timeout ─────────────────────────────────────────────────────────
app.use((req, res, next) => {
    req.setTimeout(30000);
    res.setTimeout(30000, () => {
        console.error(`[TIMEOUT] Request timeout: ${req.method} ${req.url}`);
        if (!res.headersSent) {
            res.status(408).json({ success: false, error: 'Request timeout.' });
        }
    });
    next();
});

// ── Static files ────────────────────────────────────────────────────────────
app.use('/img', express.static(path.join(__dirname, 'img')));
app.use('/assets', express.static(path.join(__dirname, 'src', 'assets')));

// ── Routes ──────────────────────────────────────────────────────────────────
app.use('/api', require('./src/routes/auth'));
app.use('/api', require('./src/routes/status'));
app.use('/api', require('./src/routes/download'));
app.use('/api', require('./src/routes/update'));
app.use('/api/admin', require('./src/routes/admin'));
app.use('/admin', require('./src/routes/admin'));

// ── Global Error Handler ────────────────────────────────────────────────────
app.use((err: Error, req: express.Request, res: express.Response, _next: express.NextFunction) => {
    console.error('[GLOBAL ERROR]', err.message);
    if (!res.headersSent) {
        res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// ── Startup ─────────────────────────────────────────────────────────────────
(async () => {
    try {
        await ensureSchema();
    } catch (err: unknown) {
        console.error('[SCHEMA] Failed to ensure schema:', (err as Error).message);
    }

    app.listen(config.PORT, () => {
        console.log(`Server running on http://localhost:${config.PORT}`);
        console.log(`Admin panel on http://localhost:${config.PORT}/admin`);
    });
})();

process.on('unhandledRejection', (reason) => {
    console.error('[UNHANDLED REJECTION]', reason);
});

process.on('uncaughtException', (err) => {
    console.error('[UNCAUGHT EXCEPTION]', err.message);
    console.error(err.stack);
});

process.on('SIGINT', () => {
    console.log('\n[SHUTDOWN] Closing server...');
    process.exit(0);
});
