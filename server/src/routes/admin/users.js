const express = require('express');
const pool = require('../../database');
const { auditLog } = require('../../middleware');
const { requireAdminJWT } = require('../../middleware/admin-jwt');

const router = express.Router();

router.get('/users', requireAdminJWT, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            'SELECT id, username, nickname, plan, role, hwid, mac, ip, created_at FROM users ORDER BY id ASC'
        );
        return res.json({ success: true, users: rows });
    } catch (err) {
        console.error('[ERROR] List users:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

router.post('/users/:userId/grant', requireAdminJWT, async (req, res) => {
    try {
        const userId = parseInt(req.params.userId, 10);
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

        auditLog('GRANT_LICENSE', req.adminUser.nickname, `Cheat ${cheat_id} to user ${userId} (${days ? `${days}d` : 'LIFETIME'})`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Grant cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

router.delete('/users/:userId/revoke/:cheatId', requireAdminJWT, async (req, res) => {
    try {
        const userId = parseInt(req.params.userId, 10);
        const cheatId = parseInt(req.params.cheatId, 10);

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

module.exports = router;
