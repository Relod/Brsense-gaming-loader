const pool = require('../database');
const {
    issueToken,
    verifyPassword,
    sanitize,
} = require('../middleware');
const { listLicensedCheatsForUser } = require('../services/license-service');
const {
    hasPendingHwidReset,
    createHwidResetRequest,
} = require('../services/hwid-reset-service');

async function handleHwidBinding(user, hwid, mac, ip) {
    if (!hwid) return { ok: true };

    if (!user.hwid || user.hwid === '' || user.hwid === 'NOT SET') {
        await pool.execute(
            'UPDATE users SET hwid = ?, mac = ?, ip = ? WHERE id = ?',
            [hwid, mac || '', ip || '', user.id]
        );

        user.hwid = hwid;
        user.mac = mac || '';
        user.ip = ip || '';
        console.log(`[HWID] Bound ${hwid} to user ${user.nickname}`);
        return { ok: true };
    }

    if (user.hwid !== hwid) {
        console.log(`[HWID] MISMATCH for ${user.nickname}: expected ${user.hwid}, got ${hwid}`);
        return {
            ok: false,
            error: 'HWID mismatch! This account is linked to another device.'
        };
    }

    await pool.execute(
        'UPDATE users SET mac = ?, ip = ? WHERE id = ?',
        [mac || user.mac, ip || user.ip, user.id]
    );

    user.mac = mac || user.mac;
    user.ip = ip || user.ip;
    return { ok: true };
}

async function createSession(userId, token, hwid) {
    await pool.execute('DELETE FROM sessions WHERE user_id = ?', [userId]);
    await pool.execute(
        'INSERT INTO sessions (user_id, token, hwid) VALUES (?, ?, ?)',
        [userId, token, hwid || 'Unknown']
    );
}

async function login(req, res) {
    try {
        const { username, password, hwid, mac, ip } = req.body;

        if (!username || !password) {
            return res.json({ success: false, error: 'Username and password are required.' });
        }

        const safeUser = sanitize(username, 64);

        const [users] = await pool.execute(
            'SELECT id, username, password, nickname, plan, hwid, mac, ip FROM users WHERE username = ?',
            [safeUser]
        );

        if (users.length === 0) {
            console.log(`[LOGIN FAIL] Unknown user: ${safeUser} from IP ${req.ip}`);
            return res.json({ success: false, error: 'Invalid username or password.' });
        }

        const isValid = await verifyPassword(password, users[0].password);
        if (!isValid) {
            console.log(`[LOGIN FAIL] Wrong password for ${safeUser} from IP ${req.ip}`);
            return res.json({ success: false, error: 'Invalid username or password.' });
        }

        const user = users[0];
        const hwidResult = await handleHwidBinding(user, hwid, mac, ip);
        if (!hwidResult.ok) {
            return res.json({ success: false, error: hwidResult.error });
        }

        const token = issueToken(user);
        await createSession(user.id, token, hwid);

        const cheats = await listLicensedCheatsForUser(user.id);

        console.log(`[LOGIN] ${user.nickname} (${user.plan}) - ${cheats.length} cheats`);

        return res.json({
            success: true,
            token,
            user: {
                nickname: user.nickname,
                plan: user.plan,
                hwid: user.hwid || 'NOT SET',
                mac: user.mac || '00:00:00:00:00:00',
                ip: user.ip || '0.0.0.0',
            },
            cheats,
        });
    } catch (err) {
        console.error('[ERROR] Login:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

async function refreshCheats(req, res) {
    try {
        const cheats = await listLicensedCheatsForUser(req.user.id);
        return res.json({ success: true, cheats });
    } catch (err) {
        console.error('[ERROR] Refresh cheats:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

async function heartbeat(req, res) {
    try {
        await pool.execute(
            'UPDATE sessions SET last_active = CURRENT_TIMESTAMP WHERE id = ?',
            [req.session.id]
        );
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Heartbeat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

async function refreshToken(req, res) {
    try {
        const token = issueToken({
            id: req.user.id,
            username: req.user.username,
            plan: req.user.plan,
        });
        await pool.execute('UPDATE sessions SET token = ? WHERE id = ?', [token, req.session.id]);
        return res.json({ success: true, token });
    } catch (err) {
        console.error('[ERROR] Refresh:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

async function requestHwidReset(req, res) {
    try {
        const { reason } = req.body;

        const hasPending = await hasPendingHwidReset(req.user.id);
        if (hasPending) {
            return res.json({
                success: false,
                error: 'You already have a pending HWID reset request.'
            });
        }

        await createHwidResetRequest(req.user.id, reason || 'No reason provided');

        console.log(`[HWID RESET] User ID ${req.user.id} requested a reset: ${reason}`);
        return res.json({
            success: true,
            message: 'HWID reset requested. Please wait for admin approval.'
        });
    } catch (err) {
        console.error('[ERROR] HWID Reset:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

async function requestHwidResetByCreds(req, res) {
    try {
        const { username, password, reason } = req.body;

        if (!username || !password) {
            return res.status(400).json({ success: false, error: 'Username and password required.' });
        }

        const [users] = await pool.execute(
            'SELECT id, nickname, password AS pw_hash FROM users WHERE username = ?',
            [sanitize(username, 64)]
        );

        if (users.length === 0 || !(await verifyPassword(password, users[0].pw_hash))) {
            return res.status(401).json({ success: false, error: 'Invalid credentials.' });
        }

        const user = users[0];
        const hasPending = await hasPendingHwidReset(user.id);
        if (hasPending) {
            return res.json({
                success: false,
                error: 'You already have a pending HWID reset request.'
            });
        }

        await createHwidResetRequest(user.id, reason || 'HWID mismatch - requested from login');

        console.log(`[HWID RESET] User ${user.nickname} (ID ${user.id}) requested reset via credentials`);
        return res.json({
            success: true,
            message: 'HWID reset requested. Please wait for admin approval.'
        });
    } catch (err) {
        console.error('[ERROR] HWID Reset by creds:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

module.exports = {
    login,
    refreshCheats,
    heartbeat,
    refreshToken,
    requestHwidReset,
    requestHwidResetByCreds,
};
