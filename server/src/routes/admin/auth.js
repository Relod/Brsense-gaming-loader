const express = require('express');
const jwt = require('jsonwebtoken');
const pool = require('../../database');
const config = require('../../config');
const { verifyPassword, sanitize, auditLog, rateLimit } = require('../../middleware');

const router = express.Router();

router.post('/login', rateLimit('admin_login', 5, 60_000), async (req, res) => {
    try {
        const { username, password } = req.body;
        if (!username || !password) {
            return res.status(400).json({ success: false, error: 'Username and password are required.' });
        }

        const safeUser = sanitize(username, 64);
        const [users] = await pool.execute(
            'SELECT id, username, password, nickname, role FROM users WHERE username = ?',
            [safeUser]
        );

        if (users.length === 0) {
            auditLog('ADMIN_LOGIN_FAIL', safeUser, `Unknown user from IP ${req.ip}`);
            return res.status(401).json({ success: false, error: 'Invalid credentials.' });
        }

        const isValid = await verifyPassword(password, users[0].password);
        if (!isValid) {
            auditLog('ADMIN_LOGIN_FAIL', safeUser, `Wrong password from IP ${req.ip}`);
            return res.status(401).json({ success: false, error: 'Invalid credentials.' });
        }

        if (users[0].role !== 'admin') {
            auditLog('ADMIN_LOGIN_DENY', safeUser, 'Non-admin attempted admin login');
            return res.status(403).json({ success: false, error: 'Admin access required.' });
        }

        const token = jwt.sign(
            { id: users[0].id, username: users[0].username, nickname: users[0].nickname, role: 'admin' },
            config.JWT_SECRET,
            { expiresIn: '24h' }
        );

        auditLog('ADMIN_LOGIN', users[0].nickname, `from IP ${req.ip}`);
        return res.json({ success: true, token, nickname: users[0].nickname });
    } catch (err) {
        console.error('[ERROR] Admin login:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

module.exports = router;
