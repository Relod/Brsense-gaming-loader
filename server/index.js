const express = require('express');
const fs = require('fs');
const helmet = require('helmet');
const config = require('./src/config');
const { ensureSchema } = require('./src/schema');

if (!fs.existsSync(config.PAYLOADS_DIR)) {
    fs.mkdirSync(config.PAYLOADS_DIR, { recursive: true });
}

const app = express();

// ── Security Headers ────────────────────────────────────────────────────────
app.use(helmet({
    contentSecurityPolicy: false, // Allow inline scripts in admin panel
    crossOriginEmbedderPolicy: false,
}));
app.use(require('cors')());
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
const path = require('path');
app.use('/img', express.static(path.join(__dirname, 'img')));
app.use('/assets', express.static(path.join(__dirname, 'src', 'assets')));

// ── Routes ──────────────────────────────────────────────────────────────────
app.use('/api', require('./src/routes/auth'));
app.use('/api', require('./src/routes/status'));
app.use('/api', require('./src/routes/download'));
app.use('/api/admin', require('./src/routes/admin'));
app.use('/admin', require('./src/routes/admin'));

// ── Global Error Handler ────────────────────────────────────────────────────
app.use((err, req, res, next) => {
    console.error('[GLOBAL ERROR]', err.message);
    if (!res.headersSent) {
        res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// ── Startup ─────────────────────────────────────────────────────────────────
(async () => {
    try {
        await ensureSchema();
    } catch (err) {
        console.error('[SCHEMA] Failed to ensure schema:', err.message);
    }

    app.listen(config.PORT, () => {
        console.log(`Server running on http://localhost:${config.PORT}`);
        console.log(`Admin panel on http://localhost:${config.PORT}/admin`);
    });
})();

process.on('unhandledRejection', (reason, promise) => {
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
