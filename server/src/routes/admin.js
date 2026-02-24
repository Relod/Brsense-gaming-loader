// =============================================================================
// routes/admin.js — Rotas administrativas + painel HTML
// =============================================================================

const express = require('express');
const router = express.Router();
const multer = require('multer');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const pool = require('../database');
const config = require('../config');
const { requireAdmin } = require('../middleware');

// ── Multer: upload de DLLs para payloads/ ────────────────────────────────────
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        // Garantir que a pasta existe
        if (!fs.existsSync(config.PAYLOADS_DIR)) {
            fs.mkdirSync(config.PAYLOADS_DIR, { recursive: true });
        }
        cb(null, config.PAYLOADS_DIR);
    },
    filename: (req, file, cb) => {
        // Usar nome temporário, será renomeado após salvar no banco
        const ext = path.extname(file.originalname) || '.dll';
        const tempName = `upload_${Date.now()}${ext}`;
        cb(null, tempName);
    },
});

const upload = multer({
    storage,
    limits: { fileSize: 50 * 1024 * 1024 }, // 50 MB max
    fileFilter: (req, file, cb) => {
        // Aceitar qualquer arquivo binário (DLL, BIN, etc.)
        cb(null, true);
    },
});

// Helper: calcular SHA-256 de um arquivo
function hashFile(filePath) {
    const data = fs.readFileSync(filePath);
    return crypto.createHash('sha256').update(data).digest('hex');
}

// Helper: renomear payload para cheat_{id}.dll
function renamePayload(tempPath, cheatId) {
    const ext = path.extname(tempPath) || '.dll';
    const finalName = `cheat_${cheatId}${ext}`;
    const finalPath = path.join(config.PAYLOADS_DIR, finalName);

    // Se já existe um payload antigo com esse nome, remover
    if (fs.existsSync(finalPath) && finalPath !== tempPath) {
        fs.unlinkSync(finalPath);
    }

    fs.renameSync(tempPath, finalPath);
    return finalName; // Retorna nome relativo (não caminho completo)
}

// =============================================================================
// GET /api/admin/cheats — Listar todos os cheats
// =============================================================================
router.get('/cheats', requireAdmin, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            'SELECT id, game, name, icon_color, process_name, payload_path, payload_sha256, notes FROM cheats ORDER BY id ASC'
        );
        return res.json({ success: true, cheats: rows });
    } catch (err) {
        console.error('[ERROR] List cheats:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// POST /api/admin/cheats — Criar cheat (com upload opcional de DLL)
// =============================================================================
router.post('/cheats', requireAdmin, upload.single('payload'), async (req, res) => {
    try {
        const { game, name, icon_color, process_name, notes } = req.body;

        if (!game || !name) {
            // Limpar arquivo uploaded se os campos obrigatórios faltam
            if (req.file) fs.unlinkSync(req.file.path);
            return res.status(400).json({
                success: false,
                error: 'Fields "game" and "name" are required.'
            });
        }

        const [result] = await pool.execute(
            `INSERT INTO cheats (game, name, icon_color, process_name, payload_path, payload_sha256, notes)
             VALUES (?, ?, ?, ?, NULL, NULL, ?)`,
            [game, name, icon_color || '#FFFFFF', process_name || null, notes || null]
        );

        const cheatId = result.insertId;

        // Se um arquivo foi enviado, mover e registrar
        let payloadPath = null;
        let payloadHash = null;
        if (req.file) {
            payloadHash = hashFile(req.file.path);
            payloadPath = renamePayload(req.file.path, cheatId);

            await pool.execute(
                'UPDATE cheats SET payload_path = ?, payload_sha256 = ? WHERE id = ?',
                [payloadPath, payloadHash, cheatId]
            );
        }

        console.log(`[ADMIN] Created cheat ID ${cheatId}: ${game} — ${name} (payload: ${payloadPath || 'none'})`);
        return res.json({ success: true, id: cheatId, payload_path: payloadPath, payload_sha256: payloadHash });
    } catch (err) {
        // Limpar arquivo se deu erro
        if (req.file && fs.existsSync(req.file.path)) fs.unlinkSync(req.file.path);
        console.error('[ERROR] Create cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// PUT /api/admin/cheats/:id — Atualizar cheat (com re-upload opcional de DLL)
// =============================================================================
router.put('/cheats/:id', requireAdmin, upload.single('payload'), async (req, res) => {
    try {
        const id = parseInt(req.params.id);
        if (isNaN(id)) {
            if (req.file) fs.unlinkSync(req.file.path);
            return res.status(400).json({ success: false, error: 'Invalid cheat ID.' });
        }

        // Verificar se existe
        const [existing] = await pool.execute('SELECT id, payload_path FROM cheats WHERE id = ?', [id]);
        if (existing.length === 0) {
            if (req.file) fs.unlinkSync(req.file.path);
            return res.status(404).json({ success: false, error: 'Cheat not found.' });
        }

        const { game, name, icon_color, process_name, notes } = req.body;

        // Atualizar campos de metadados
        await pool.execute(
            `UPDATE cheats SET
                game = COALESCE(?, game),
                name = COALESCE(?, name),
                icon_color = COALESCE(?, icon_color),
                process_name = COALESCE(?, process_name),
                notes = COALESCE(?, notes)
             WHERE id = ?`,
            [game || null, name || null, icon_color || null, process_name || null, notes !== undefined ? notes : null, id]
        );

        // Se um novo payload foi enviado, substituir
        if (req.file) {
            // Remover payload antigo
            const oldPayload = existing[0].payload_path;
            if (oldPayload) {
                const oldPath = path.join(config.PAYLOADS_DIR, oldPayload);
                if (fs.existsSync(oldPath)) fs.unlinkSync(oldPath);
            }

            const payloadHash = hashFile(req.file.path);
            const payloadPath = renamePayload(req.file.path, id);

            await pool.execute(
                'UPDATE cheats SET payload_path = ?, payload_sha256 = ? WHERE id = ?',
                [payloadPath, payloadHash, id]
            );
        }

        console.log(`[ADMIN] Updated cheat ID ${id}`);
        return res.json({ success: true });
    } catch (err) {
        if (req.file && fs.existsSync(req.file.path)) fs.unlinkSync(req.file.path);
        console.error('[ERROR] Update cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// DELETE /api/admin/cheats/:id — Deletar cheat + remover DLL do disco
// =============================================================================
router.delete('/cheats/:id', requireAdmin, async (req, res) => {
    try {
        const id = parseInt(req.params.id);
        if (isNaN(id)) {
            return res.status(400).json({ success: false, error: 'Invalid cheat ID.' });
        }

        // Buscar payload_path para remover arquivo
        const [rows] = await pool.execute('SELECT payload_path FROM cheats WHERE id = ?', [id]);
        if (rows.length === 0) {
            return res.status(404).json({ success: false, error: 'Cheat not found.' });
        }

        const payloadPath = rows[0].payload_path;

        // Remover do banco (CASCADE remove as licenças também)
        await pool.execute('DELETE FROM cheats WHERE id = ?', [id]);

        // Remover arquivo do disco
        if (payloadPath) {
            const fullPath = path.join(config.PAYLOADS_DIR, payloadPath);
            if (fs.existsSync(fullPath)) {
                fs.unlinkSync(fullPath);
                console.log(`[ADMIN] Deleted payload file: ${payloadPath}`);
            }
        }

        console.log(`[ADMIN] Deleted cheat ID ${id}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Delete cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// GET /api/admin/users — Listar todos os usuarios
// =============================================================================
router.get('/users', requireAdmin, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            'SELECT id, username, nickname, plan, hwid, mac, ip, created_at FROM users ORDER BY id ASC'
        );
        return res.json({ success: true, users: rows });
    } catch (err) {
        console.error('[ERROR] List users:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// POST /api/admin/users/:userId/grant — Conceder licença de cheat
// =============================================================================
router.post('/users/:userId/grant', requireAdmin, async (req, res) => {
    try {
        const userId = parseInt(req.params.userId);
        const { cheat_id, days, status } = req.body;

        if (!cheat_id) {
            return res.status(400).json({ success: false, error: 'cheat_id is required.' });
        }

        const expires = days ? new Date(Date.now() + days * 24 * 60 * 60 * 1000) : null;
        const expiresSql = expires ? expires.toISOString().slice(0, 19).replace('T', ' ') : null;

        await pool.execute(
            `INSERT INTO user_cheats (user_id, cheat_id, expires_at, status)
             VALUES (?, ?, ?, ?)
             ON DUPLICATE KEY UPDATE expires_at = VALUES(expires_at), status = VALUES(status)`,
            [userId, cheat_id, expiresSql, status || 'active']
        );

        console.log(`[ADMIN] Granted cheat ${cheat_id} to user ${userId} (${days ? days + 'd' : 'LIFETIME'})`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Grant cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// DELETE /api/admin/users/:userId/revoke/:cheatId — Revogar licença
// =============================================================================
router.delete('/users/:userId/revoke/:cheatId', requireAdmin, async (req, res) => {
    try {
        const userId = parseInt(req.params.userId);
        const cheatId = parseInt(req.params.cheatId);

        await pool.execute(
            'DELETE FROM user_cheats WHERE user_id = ? AND cheat_id = ?',
            [userId, cheatId]
        );

        console.log(`[ADMIN] Revoked cheat ${cheatId} from user ${userId}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Revoke cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// GET /api/admin/sessions — Listar sessões ativas
// =============================================================================
router.get('/sessions', requireAdmin, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            `SELECT s.id, s.user_id, s.hwid, s.last_active, u.username, u.nickname
             FROM sessions s
             JOIN users u ON u.id = s.user_id
             ORDER BY s.last_active DESC`
        );
        return res.json({ success: true, sessions: rows });
    } catch (err) {
        console.error('[ERROR] List sessions:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// POST /api/admin/sessions/revoke — Revogar sessão
// =============================================================================
router.post('/sessions/revoke', requireAdmin, async (req, res) => {
    try {
        const { session_id } = req.body;
        if (!session_id) {
            return res.status(400).json({ success: false, error: 'session_id is required.' });
        }
        await pool.execute('DELETE FROM sessions WHERE id = ?', [session_id]);
        console.log(`[ADMIN] Revoked session ID ${session_id}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Revoke session:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// GET /api/admin/hwid_resets — Listar pedidos de HWID reset
// =============================================================================
router.get('/hwid_resets', requireAdmin, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            `SELECT h.id, h.user_id, h.reason, h.status, h.created_at, u.username, u.nickname
             FROM hwid_reset_requests h
             JOIN users u ON u.id = h.user_id
             ORDER BY h.created_at DESC`
        );
        return res.json({ success: true, resets: rows });
    } catch (err) {
        console.error('[ERROR] List HWID resets:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// POST /api/admin/hwid_reset/:id/approve — Aprovar reset (limpa HWID do user)
// =============================================================================
router.post('/hwid_reset/:id/approve', requireAdmin, async (req, res) => {
    try {
        const reqId = parseInt(req.params.id);

        // Buscar o pedido
        const [reqs] = await pool.execute(
            'SELECT user_id FROM hwid_reset_requests WHERE id = ? AND status = "pending"',
            [reqId]
        );

        if (reqs.length === 0) {
            return res.status(404).json({ success: false, error: 'Request not found or already processed.' });
        }

        // Limpar HWID do usuário
        await pool.execute('UPDATE users SET hwid = "", mac = "", ip = "" WHERE id = ?', [reqs[0].user_id]);

        // Marcar como aprovado
        await pool.execute('UPDATE hwid_reset_requests SET status = "approved" WHERE id = ?', [reqId]);

        // Invalidar sessões do usuário (forçar re-login)
        await pool.execute('DELETE FROM sessions WHERE user_id = ?', [reqs[0].user_id]);

        console.log(`[ADMIN] Approved HWID reset #${reqId} for user ${reqs[0].user_id}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Approve HWID reset:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// POST /api/admin/hwid_reset/:id/reject — Rejeitar reset
// =============================================================================
router.post('/hwid_reset/:id/reject', requireAdmin, async (req, res) => {
    try {
        const reqId = parseInt(req.params.id);
        await pool.execute('UPDATE hwid_reset_requests SET status = "rejected" WHERE id = ?', [reqId]);
        console.log(`[ADMIN] Rejected HWID reset #${reqId}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Reject HWID reset:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// GET /admin — Painel Web Admin
// =============================================================================
router.get('/', (req, res) => {
    const htmlPath = path.join(__dirname, '..', 'admin-panel.html');
    if (fs.existsSync(htmlPath)) {
        return res.sendFile(htmlPath);
    }
    return res.status(500).send('Admin panel HTML file not found.');
});

module.exports = router;
