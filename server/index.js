// =============================================================================
// index.js - Entry point do servidor BR Sense
// =============================================================================

const express = require('express');
const fs = require('fs');
const config = require('./src/config');
const { ensureSchema } = require('./src/schema');

// Garantir que a pasta de payloads existe
if (!fs.existsSync(config.PAYLOADS_DIR)) {
    fs.mkdirSync(config.PAYLOADS_DIR, { recursive: true });
}

const app = express();
app.use(require('cors')());
app.use(express.json());

// Rotas
app.use('/api', require('./src/routes/auth'));
app.use('/api', require('./src/routes/status'));
app.use('/api', require('./src/routes/download'));
app.use('/api/admin', require('./src/routes/admin'));
app.use('/admin', require('./src/routes/admin'));

// Iniciar servidor (com checagem de schema nao-destrutiva)
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
