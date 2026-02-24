// =============================================================================
// routes/download.js — Download de payloads (DLL) dos cheats
// =============================================================================

const express = require('express');
const router = express.Router();
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const pool = require('../database');
const config = require('../config');
const { authenticateToken } = require('../middleware');

// =============================================================================
// GET /api/download/:cheat_id — Baixar payload do cheat
// =============================================================================
router.get('/download/:cheat_id', authenticateToken, async (req, res) => {
    try {
        const cheatId = parseInt(req.params.cheat_id);

        if (isNaN(cheatId)) {
            return res.status(400).json({ success: false, error: 'Invalid cheat ID.' });
        }

        // Verificar licença ativa
        const [licenses] = await pool.execute(
            'SELECT uc.status, uc.expires_at FROM user_cheats uc WHERE uc.user_id = ? AND uc.cheat_id = ?',
            [req.user.id, cheatId]
        );

        if (licenses.length === 0) {
            return res.status(403).json({
                success: false,
                error: 'You do not own a license for this cheat.'
            });
        }

        const license = licenses[0];
        if (license.status !== 'active') {
            return res.status(403).json({
                success: false,
                error: 'Your license for this cheat is not active.'
            });
        }

        if (license.expires_at) {
            if (new Date() > new Date(license.expires_at)) {
                return res.status(403).json({
                    success: false,
                    error: 'Your license for this cheat has expired.'
                });
            }
        }

        // Buscar metadados do cheat
        const [cheats] = await pool.execute(
            'SELECT payload_path, payload_sha256 FROM cheats WHERE id = ?',
            [cheatId]
        );

        if (cheats.length === 0) {
            return res.status(404).json({ success: false, error: 'Cheat not found.' });
        }

        const cheat = cheats[0];

        if (!cheat.payload_path) {
            return res.status(404).json({
                success: false,
                error: 'No payload file configured for this cheat.'
            });
        }

        // Resolver caminho do payload (relativo à pasta payloads/)
        const payloadFullPath = path.resolve(config.PAYLOADS_DIR, cheat.payload_path);

        // Segurança: garantir que o caminho está dentro de payloads/
        if (!payloadFullPath.startsWith(path.resolve(config.PAYLOADS_DIR))) {
            return res.status(403).json({ success: false, error: 'Invalid payload path.' });
        }

        if (!fs.existsSync(payloadFullPath)) {
            return res.status(404).json({
                success: false,
                error: 'Payload file not found on server.'
            });
        }

        // Ler o payload
        const payloadBuffer = fs.readFileSync(payloadFullPath);

        // Calcular hash se não estiver no banco
        let payloadHash = cheat.payload_sha256;
        if (!payloadHash) {
            payloadHash = crypto.createHash('sha256').update(payloadBuffer).digest('hex');
        }

        console.log(`[DOWNLOAD] User ID ${req.user.id} downloaded cheat ID ${cheatId} (${payloadBuffer.length} bytes)`);

        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Disposition', `attachment; filename="payload_${cheatId}.dll"`);
        res.setHeader('x-sha256', payloadHash);
        return res.send(payloadBuffer);

    } catch (err) {
        console.error('[ERROR] Download:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

module.exports = router;
