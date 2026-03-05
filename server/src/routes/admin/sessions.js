const express = require('express');
const pool = require('../../database');
const { requireAdminJWT } = require('../../middleware/admin-jwt');

const router = express.Router();

router.get('/sessions', requireAdminJWT, async (req, res) => {
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

router.post('/sessions/revoke', requireAdminJWT, async (req, res) => {
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

module.exports = router;
