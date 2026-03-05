const jwt = require('jsonwebtoken');
const bcrypt = require('bcrypt');
const config = require('./config');
const pool = require('./database');

// ── Rate Limiter ────────────────────────────────────────────────────────────
const rateCounters = new Map();

function rateLimit(routeKey, limit, windowMs) {
    return (req, res, next) => {
        const now = Date.now();
        const bucketKey = `${routeKey}:${req.ip}`;
        const bucket = rateCounters.get(bucketKey) || { count: 0, reset: now + windowMs };

        if (now > bucket.reset) {
            bucket.count = 0;
            bucket.reset = now + windowMs;
        }

        bucket.count += 1;
        rateCounters.set(bucketKey, bucket);

        if (bucket.count > limit) {
            return res.status(429).json({ success: false, error: 'Too many requests. Try again later.' });
        }
        next();
    };
}

// ── JWT Authentication ──────────────────────────────────────────────────────
async function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1];

    if (!token) {
        return res.status(401).json({ success: false, error: 'Token missing.' });
    }

    try {
        const authUser = jwt.verify(token, config.JWT_SECRET);

        const [sessions] = await pool.execute(
            'SELECT * FROM sessions WHERE token = ?',
            [token]
        );

        if (sessions.length === 0) {
            return res.status(403).json({
                success: false,
                error: 'Session expired or invalidated by another login.'
            });
        }

        req.user = authUser;
        req.session = sessions[0];
        req.token = token;
        next();
    } catch (err) {
        if (err.name === 'TokenExpiredError') {
            return res.status(403).json({ success: false, error: 'Token expired.' });
        }
        if (err.name === 'JsonWebTokenError') {
            return res.status(403).json({ success: false, error: 'Invalid token.' });
        }
        console.error('[ERROR] authenticateToken:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

// ── Admin Key ───────────────────────────────────────────────────────────────
function requireAdmin(req, res, next) {
    const key = req.headers['x-admin-key'];
    if (!key || key !== config.ADMIN_KEY) {
        return res.status(401).json({ success: false, error: 'Admin key missing or invalid.' });
    }
    next();
}

// ── Helper: Issue JWT ───────────────────────────────────────────────────────
function issueToken(user) {
    return jwt.sign(
        { id: user.id, username: user.username, plan: user.plan },
        config.JWT_SECRET,
        { expiresIn: config.JWT_EXPIRES }
    );
}

// ── Bcrypt Helpers ──────────────────────────────────────────────────────────
async function hashPassword(plaintext) {
    return bcrypt.hash(plaintext, config.BCRYPT_ROUNDS);
}

async function verifyPassword(plaintext, hash) {
    // Backward compat: if hash doesn't look like bcrypt ($2b$), do plaintext compare
    if (!hash || !hash.startsWith('$2')) {
        return plaintext === hash;
    }
    return bcrypt.compare(plaintext, hash);
}

// ── Input Sanitization ─────────────────────────────────────────────────────
function sanitize(str, maxLen = 255) {
    if (typeof str !== 'string') return '';
    return str.trim().substring(0, maxLen);
}

function requireFields(obj, fields) {
    for (const f of fields) {
        if (!obj[f] || (typeof obj[f] === 'string' && obj[f].trim() === '')) {
            return f;
        }
    }
    return null;
}

// ── Audit Logger ────────────────────────────────────────────────────────────
function auditLog(action, adminUser, details = '') {
    const ts = new Date().toISOString();
    console.log(`[AUDIT] ${ts} | ${action} | admin:${adminUser} | ${details}`);
}

module.exports = {
    rateLimit,
    authenticateToken,
    requireAdmin,
    issueToken,
    hashPassword,
    verifyPassword,
    sanitize,
    requireFields,
    auditLog,
};
