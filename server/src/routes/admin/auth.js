const express = require('express');
const jwt = require('jsonwebtoken');
const pool = require('../../database');
const config = require('../../config');
const { verifyPassword, sanitize, auditLog, rateLimit } = require('../../middleware');
const { generateSecret, verifyTOTP, generateOTPAuthURI } = require('../../utils/totp');
const { requireAdminJWT } = require('../../middleware/admin-jwt');

const router = express.Router();

// POST /admin/login — Admin login with optional 2FA
router.post('/login', rateLimit('admin_login', 5, 60_000), async (req, res) => {
    try {
        const { username, password, totp_code } = req.body;
        if (!username || !password) {
            return res.status(400).json({ success: false, error: 'Username and password are required.' });
        }

        const safeUser = sanitize(username, 64);
        const [users] = await pool.execute(
            'SELECT id, username, password, nickname, role, totp_secret FROM users WHERE username = ?',
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

        // 2FA check: if user has TOTP enabled, require code
        if (users[0].totp_secret) {
            if (!totp_code) {
                return res.json({
                    success: false,
                    requires_2fa: true,
                    error: '2FA code required.'
                });
            }

            if (!verifyTOTP(users[0].totp_secret, totp_code)) {
                auditLog('ADMIN_2FA_FAIL', users[0].nickname, `Invalid 2FA code from IP ${req.ip}`);
                return res.status(401).json({ success: false, error: 'Invalid 2FA code.' });
            }
        }

        const token = jwt.sign(
            { id: users[0].id, username: users[0].username, nickname: users[0].nickname, role: 'admin' },
            config.JWT_SECRET,
            { expiresIn: '24h' }
        );

        auditLog('ADMIN_LOGIN', users[0].nickname, `from IP ${req.ip}`);
        return res.json({
            success: true,
            token,
            nickname: users[0].nickname,
            has_2fa: !!users[0].totp_secret,
        });
    } catch (err) {
        console.error('[ERROR] Admin login:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// POST /admin/2fa/setup — Generate new TOTP secret
router.post('/2fa/setup', requireAdminJWT, async (req, res) => {
    try {
        const secret = generateSecret();
        const uri = generateOTPAuthURI(secret, req.adminUser.username);

        // Store secret temporarily — only committed after verification
        return res.json({
            success: true,
            secret,
            otpauth_uri: uri,
            message: 'Scan the QR code or enter the secret in your authenticator app, then verify with /2fa/verify.'
        });
    } catch (err) {
        console.error('[ERROR] 2FA setup:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// POST /admin/2fa/verify — Verify and enable 2FA
router.post('/2fa/verify', requireAdminJWT, async (req, res) => {
    try {
        const { secret, code } = req.body;
        if (!secret || !code) {
            return res.status(400).json({ success: false, error: 'Secret and code are required.' });
        }

        if (!verifyTOTP(secret, code)) {
            return res.status(400).json({ success: false, error: 'Invalid code. Please try again.' });
        }

        // Save secret to DB — 2FA is now enabled
        await pool.execute(
            'UPDATE users SET totp_secret = ? WHERE id = ?',
            [secret, req.adminUser.id]
        );

        auditLog('2FA_ENABLED', req.adminUser.nickname, 'TOTP 2FA enabled');
        return res.json({ success: true, message: '2FA enabled successfully!' });
    } catch (err) {
        console.error('[ERROR] 2FA verify:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// POST /admin/2fa/disable — Disable 2FA
router.post('/2fa/disable', requireAdminJWT, async (req, res) => {
    try {
        await pool.execute(
            'UPDATE users SET totp_secret = NULL WHERE id = ?',
            [req.adminUser.id]
        );

        auditLog('2FA_DISABLED', req.adminUser.nickname, 'TOTP 2FA disabled');
        return res.json({ success: true, message: '2FA disabled.' });
    } catch (err) {
        console.error('[ERROR] 2FA disable:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

module.exports = router;
