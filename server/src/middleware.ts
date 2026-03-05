import { Request, Response, NextFunction } from 'express';
import jwt from 'jsonwebtoken';
import bcrypt from 'bcrypt';
import config = require('./config');
import pool = require('./database');

// ── Rate Limiter ────────────────────────────────────────────────────────────
const rateCounters = new Map<string, { count: number; reset: number }>();

export function rateLimit(routeKey: string, limit: number, windowMs: number) {
    return (req: Request, res: Response, next: NextFunction): void => {
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
            res.status(429).json({ success: false, error: 'Too many requests. Try again later.' });
            return;
        }
        next();
    };
}

// ── JWT Authentication ──────────────────────────────────────────────────────
export interface AuthUser {
    id: number;
    username: string;
    plan: string;
}

export interface AuthRequest extends Request {
    user?: AuthUser;
    session?: Record<string, unknown>;
    token?: string;
}

export async function authenticateToken(req: AuthRequest, res: Response, next: NextFunction): Promise<void> {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1];

    if (!token) {
        res.status(401).json({ success: false, error: 'Token missing.' });
        return;
    }

    try {
        const authUser = jwt.verify(token, config.JWT_SECRET) as AuthUser;

        const [sessions] = await pool.execute(
            'SELECT * FROM sessions WHERE token = ?',
            [token]
        );

        if ((sessions as unknown[]).length === 0) {
            res.status(403).json({
                success: false,
                error: 'Session expired or invalidated by another login.'
            });
            return;
        }

        req.user = authUser;
        req.session = (sessions as Record<string, unknown>[])[0];
        req.token = token;
        next();
    } catch (err: unknown) {
        const error = err as { name?: string; message?: string };
        if (error.name === 'TokenExpiredError') {
            res.status(403).json({ success: false, error: 'Token expired.' });
            return;
        }
        if (error.name === 'JsonWebTokenError') {
            res.status(403).json({ success: false, error: 'Invalid token.' });
            return;
        }
        console.error('[ERROR] authenticateToken:', error.message);
        res.status(500).json({ success: false, error: 'Internal server error.' });
    }
}

// ── Admin Key ───────────────────────────────────────────────────────────────
export function requireAdmin(req: Request, res: Response, next: NextFunction): void {
    const key = req.headers['x-admin-key'] as string | undefined;
    if (!key || key !== config.ADMIN_KEY) {
        res.status(401).json({ success: false, error: 'Admin key missing or invalid.' });
        return;
    }
    next();
}

// ── Helper: Issue JWT ───────────────────────────────────────────────────────
export function issueToken(user: { id: number; username: string; plan: string }): string {
    return jwt.sign(
        { id: user.id, username: user.username, plan: user.plan },
        config.JWT_SECRET,
        { expiresIn: config.JWT_EXPIRES }
    );
}

// ── Bcrypt Helpers ──────────────────────────────────────────────────────────
export async function hashPassword(plaintext: string): Promise<string> {
    return bcrypt.hash(plaintext, config.BCRYPT_ROUNDS);
}

export async function verifyPassword(plaintext: string, hash: string): Promise<boolean> {
    if (!hash || !hash.startsWith('$2')) {
        return plaintext === hash;
    }
    return bcrypt.compare(plaintext, hash);
}

// ── Input Sanitization ─────────────────────────────────────────────────────
export function sanitize(str: unknown, maxLen = 255): string {
    if (typeof str !== 'string') return '';
    return str.trim().substring(0, maxLen);
}

export function requireFields(obj: Record<string, unknown>, fields: string[]): string | null {
    for (const f of fields) {
        if (!obj[f] || (typeof obj[f] === 'string' && (obj[f] as string).trim() === '')) {
            return f;
        }
    }
    return null;
}

// ── Audit Logger ────────────────────────────────────────────────────────────
export function auditLog(action: string, adminUser: string, details = ''): void {
    const ts = new Date().toISOString();
    console.log(`[AUDIT] ${ts} | ${action} | admin:${adminUser} | ${details}`);
}
