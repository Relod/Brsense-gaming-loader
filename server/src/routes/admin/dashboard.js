const express = require('express');
const pool = require('../../database');
const { requireAdminJWT } = require('../../middleware/admin-jwt');

const router = express.Router();

// GET /admin/stats — Dashboard statistics
router.get('/stats', requireAdminJWT, async (req, res) => {
    try {
        // Total counts
        const [[{ totalUsers }]] = await pool.execute('SELECT COUNT(*) as totalUsers FROM users');
        const [[{ totalCheats }]] = await pool.execute('SELECT COUNT(*) as totalCheats FROM cheats');
        const [[{ activeSessions }]] = await pool.execute('SELECT COUNT(*) as activeSessions FROM sessions');
        const [[{ pendingResets }]] = await pool.execute(
            "SELECT COUNT(*) as pendingResets FROM hwid_reset_requests WHERE status = 'pending'"
        );

        // Users registered per day (last 30 days)
        const [usersByDay] = await pool.execute(`
            SELECT DATE(created_at) as date, COUNT(*) as count
            FROM users
            WHERE created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)
            GROUP BY DATE(created_at)
            ORDER BY date
        `);

        // Plan distribution
        const [planDistribution] = await pool.execute(`
            SELECT plan, COUNT(*) as count
            FROM users
            GROUP BY plan
            ORDER BY count DESC
        `);

        // Active cheats vs disabled
        const [cheatStatus] = await pool.execute(`
            SELECT
              SUM(CASE WHEN enabled = 1 THEN 1 ELSE 0 END) as active,
              SUM(CASE WHEN enabled = 0 THEN 1 ELSE 0 END) as disabled
            FROM cheats
        `);

        // Sessions per day (last 7 days)
        const [sessionsByDay] = await pool.execute(`
            SELECT DATE(last_active) as date, COUNT(*) as count
            FROM sessions
            WHERE last_active >= DATE_SUB(NOW(), INTERVAL 7 DAY)
            GROUP BY DATE(last_active)
            ORDER BY date
        `);

        // Top 5 most licensed cheats
        const [topCheats] = await pool.execute(`
            SELECT c.name, c.game, COUNT(uc.id) as licenses
            FROM cheats c
            LEFT JOIN user_cheats uc ON c.id = uc.cheat_id
            GROUP BY c.id, c.name, c.game
            ORDER BY licenses DESC
            LIMIT 5
        `);

        return res.json({
            success: true,
            stats: {
                totalUsers,
                totalCheats,
                activeSessions,
                pendingResets,
                usersByDay,
                planDistribution,
                cheatStatus: cheatStatus[0] || { active: 0, disabled: 0 },
                sessionsByDay,
                topCheats,
            }
        });
    } catch (err) {
        console.error('[ERROR] Dashboard stats:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

module.exports = router;
