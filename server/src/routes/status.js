// =============================================================================
// routes/status.js — Status do servidor
// =============================================================================

const express = require('express');
const router = express.Router();
const pool = require('../database');

const startTime = Date.now();

// =============================================================================
// GET /api/status — Status do servidor
// =============================================================================
router.get('/status', async (req, res) => {
    try {
        await pool.execute('SELECT 1');

        const uptime = Math.floor((Date.now() - startTime) / 1000);
        const now = new Date();
        const lastUpdate = now.toLocaleDateString('pt-BR') + ' ' + now.toLocaleTimeString('pt-BR');

        return res.json({
            online: true,
            version: '1.0.0',
            region: 'South America',
            last_update: lastUpdate,
            uptime_seconds: uptime,
            ping: Math.floor(Math.random() * 30) + 5,
        });
    } catch (err) {
        return res.json({
            online: false,
            version: '1.0.0',
            region: 'South America',
            last_update: 'N/A',
            uptime_seconds: 0,
            ping: 0,
        });
    }
});

module.exports = router;
