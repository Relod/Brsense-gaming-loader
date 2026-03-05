const express = require('express');
const pool = require('../../database');
const { auditLog } = require('../../middleware');
const { requireAdminJWT } = require('../../middleware/admin-jwt');

const router = express.Router();

router.get('/hwid_resets', requireAdminJWT, async (req, res) => {
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

router.post('/hwid_reset/:id/approve', requireAdminJWT, async (req, res) => {
    try {
        const reqId = parseInt(req.params.id, 10);

        const [reqs] = await pool.execute(
            'SELECT user_id FROM hwid_reset_requests WHERE id = ? AND status = "pending"',
            [reqId]
        );

        if (reqs.length === 0) {
            return res.status(404).json({ success: false, error: 'Request not found or already processed.' });
        }

        await pool.execute('UPDATE users SET hwid = "", mac = "", ip = "" WHERE id = ?', [reqs[0].user_id]);
        await pool.execute('UPDATE hwid_reset_requests SET status = "approved" WHERE id = ?', [reqId]);
        await pool.execute('DELETE FROM sessions WHERE user_id = ?', [reqs[0].user_id]);

        auditLog('APPROVE_HWID_RESET', req.adminUser.nickname, `Request #${reqId} for user ${reqs[0].user_id}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Approve HWID reset:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

router.post('/hwid_reset/:id/reject', requireAdminJWT, async (req, res) => {
    try {
        const reqId = parseInt(req.params.id, 10);
        await pool.execute('UPDATE hwid_reset_requests SET status = "rejected" WHERE id = ?', [reqId]);
        auditLog('REJECT_HWID_RESET', req.adminUser.nickname, `Request #${reqId}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Reject HWID reset:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

module.exports = router;
