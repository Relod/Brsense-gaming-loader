// =============================================================================
// routes/auth.js — Login, Heartbeat, Refresh
// =============================================================================

const express = require('express');
const router = express.Router();
const pool = require('../database');
const { rateLimit, authenticateToken, issueToken } = require('../middleware');

// =============================================================================
// POST /api/login — Autenticação com HWID Binding
// =============================================================================
router.post('/login', rateLimit('login', 10, 60_000), async (req, res) => {
    try {
        const { username, password, hwid, mac, ip } = req.body;

        if (!username || !password) {
            return res.json({ success: false, error: 'Username and password are required.' });
        }

        // Buscar usuario
        const [users] = await pool.execute(
            'SELECT id, username, nickname, plan, hwid, mac, ip FROM users WHERE username = ? AND password = ?',
            [username, password]
        );

        if (users.length === 0) {
            return res.json({ success: false, error: 'Invalid username or password.' });
        }

        const user = users[0];

        // ── HWID Binding ──────────────────────────────────────────────────
        if (hwid) {
            if (!user.hwid || user.hwid === '' || user.hwid === 'NOT SET') {
                // Primeiro login — vincular HWID/MAC/IP
                await pool.execute(
                    'UPDATE users SET hwid = ?, mac = ?, ip = ? WHERE id = ?',
                    [hwid, mac || '', ip || '', user.id]
                );
                user.hwid = hwid;
                user.mac = mac || '';
                user.ip = ip || '';
                console.log(`[HWID] Bound ${hwid} to user ${user.nickname}`);
            } else if (user.hwid !== hwid) {
                // HWID diferente — bloquear
                console.log(`[HWID] MISMATCH for ${user.nickname}: expected ${user.hwid}, got ${hwid}`);
                return res.json({
                    success: false,
                    error: 'HWID mismatch! This account is linked to another device.'
                });
            } else {
                // HWID correto — atualizar MAC e IP
                await pool.execute(
                    'UPDATE users SET mac = ?, ip = ? WHERE id = ?',
                    [mac || user.mac, ip || user.ip, user.id]
                );
                user.mac = mac || user.mac;
                user.ip = ip || user.ip;
            }
        }

        // ── Criar Sessão / JWT ────────────────────────────────────────────
        const token = issueToken(user);

        // Remover sessões anteriores (prevenir account sharing)
        await pool.execute('DELETE FROM sessions WHERE user_id = ?', [user.id]);

        // Registrar nova sessão
        await pool.execute(
            'INSERT INTO sessions (user_id, token, hwid) VALUES (?, ?, ?)',
            [user.id, token, hwid || 'Unknown']
        );

        // ── Buscar cheats licenciados ─────────────────────────────────────
        const [cheats] = await pool.execute(
            `SELECT c.id, c.game, c.name, c.icon_color, c.process_name,
                    c.payload_sha256, c.notes, uc.expires_at, uc.status
             FROM user_cheats uc
             JOIN cheats c ON uc.cheat_id = c.id
             WHERE uc.user_id = ?
             ORDER BY c.game ASC`,
            [user.id]
        );

        // Formatar cheats — calcular tempo restante
        const formattedCheats = cheats.map((ch) => {
            let timeLeft = 'LIFETIME';
            let isExpired = false;

            if (ch.expires_at) {
                const now = new Date();
                const exp = new Date(ch.expires_at);
                const diff = exp - now;

                if (diff <= 0) {
                    timeLeft = 'EXPIRED';
                    isExpired = true;
                } else {
                    const days = Math.floor(diff / (1000 * 60 * 60 * 24));
                    const hours = Math.floor((diff % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60));
                    if (days > 0) timeLeft = `${days}d ${hours}h`;
                    else {
                        const mins = Math.floor((diff % (1000 * 60 * 60)) / (1000 * 60));
                        timeLeft = `${hours}h ${mins}m`;
                    }
                }
            }

            return {
                id: ch.id,
                game: ch.game,
                name: ch.name,
                icon_color: ch.icon_color,
                hash: ch.payload_sha256 || '',
                process: ch.process_name || '',
                notes: ch.notes || '',
                time_left: timeLeft,
                status: isExpired ? 'expired' : ch.status,
            };
        });

        console.log(`[LOGIN] ${user.nickname} (${user.plan}) — ${formattedCheats.length} cheats`);

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
            cheats: formattedCheats,
        });
    } catch (err) {
        console.error('[ERROR] Login:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// GET /api/cheats — Refresh da lista de cheats (autenticado)
// =============================================================================
router.get('/cheats', authenticateToken, async (req, res) => {
    try {
        const [cheats] = await pool.execute(
            `SELECT c.id, c.game, c.name, c.icon_color, c.process_name,
                    c.payload_sha256, c.notes, uc.expires_at, uc.status
             FROM user_cheats uc
             JOIN cheats c ON uc.cheat_id = c.id
             WHERE uc.user_id = ?
             ORDER BY c.game ASC`,
            [req.user.id]
        );

        const formattedCheats = cheats.map((ch) => {
            let timeLeft = 'LIFETIME';
            let isExpired = false;

            if (ch.expires_at) {
                const now = new Date();
                const exp = new Date(ch.expires_at);
                const diff = exp - now;

                if (diff <= 0) {
                    timeLeft = 'EXPIRED';
                    isExpired = true;
                } else {
                    const days = Math.floor(diff / (1000 * 60 * 60 * 24));
                    const hours = Math.floor((diff % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60));
                    if (days > 0) timeLeft = `${days}d ${hours}h`;
                    else {
                        const mins = Math.floor((diff % (1000 * 60 * 60)) / (1000 * 60));
                        timeLeft = `${hours}h ${mins}m`;
                    }
                }
            }

            return {
                id: ch.id,
                game: ch.game,
                name: ch.name,
                icon_color: ch.icon_color,
                hash: ch.payload_sha256 || '',
                process: ch.process_name || '',
                notes: ch.notes || '',
                time_left: timeLeft,
                status: isExpired ? 'expired' : ch.status,
            };
        });

        return res.json({ success: true, cheats: formattedCheats });
    } catch (err) {
        console.error('[ERROR] Refresh cheats:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

// =============================================================================
// POST /api/heartbeat — Manter sessão ativa
// =============================================================================
router.post('/heartbeat', authenticateToken, async (req, res) => {
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
});

// =============================================================================
// POST /api/refresh — Renovar token JWT
// =============================================================================
router.post('/refresh', authenticateToken, async (req, res) => {
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
});

// =============================================================================
// POST /api/hwid_reset — Solicitar reset de HWID
// =============================================================================
router.post('/hwid_reset', authenticateToken, rateLimit('hwid_reset', 5, 60_000), async (req, res) => {
    try {
        const { reason } = req.body;

        // Verificar se já tem solicitação pendente
        const [existing] = await pool.execute(
            'SELECT id FROM hwid_reset_requests WHERE user_id = ? AND status = "pending"',
            [req.user.id]
        );

        if (existing.length > 0) {
            return res.json({
                success: false,
                error: 'You already have a pending HWID reset request.'
            });
        }

        await pool.execute(
            'INSERT INTO hwid_reset_requests (user_id, reason) VALUES (?, ?)',
            [req.user.id, reason || 'No reason provided']
        );

        console.log(`[HWID RESET] User ID ${req.user.id} requested a reset: ${reason}`);
        return res.json({
            success: true,
            message: 'HWID reset requested. Please wait for admin approval.'
        });
    } catch (err) {
        console.error('[ERROR] HWID Reset:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

module.exports = router;
