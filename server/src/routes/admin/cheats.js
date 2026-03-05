const express = require('express');
const pool = require('../../database');
const { auditLog } = require('../../middleware');
const { requireAdminJWT } = require('../../middleware/admin-jwt');
const { uploadPayload, hashFile, renamePayload, safeUnlink, payloadFullPath } = require('../../services/payload-service');

const router = express.Router();

router.get('/cheats', requireAdminJWT, async (req, res) => {
    try {
        const [rows] = await pool.execute(
            `SELECT id, game, name, icon_color, process_name,
                    injection_method, kill_processes, launch_params,
                    steam_app_id, requires_admin, enabled,
                    payload_path, payload_sha256, notes
             FROM cheats ORDER BY id ASC`
        );
        return res.json({ success: true, cheats: rows });
    } catch (err) {
        console.error('[ERROR] List cheats:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

router.post('/cheats', requireAdminJWT, uploadPayload.single('payload'), async (req, res) => {
    try {
        const {
            game,
            name,
            icon_color,
            process_name,
            injection_method,
            kill_processes,
            launch_params,
            steam_app_id,
            requires_admin,
            enabled,
            notes,
        } = req.body;

        if (!game || !name) {
            if (req.file) safeUnlink(req.file.path);
            return res.status(400).json({
                success: false,
                error: 'Fields "game" and "name" are required.'
            });
        }

        const [result] = await pool.execute(
            `INSERT INTO cheats (game, name, icon_color, process_name, injection_method, kill_processes, launch_params, steam_app_id, requires_admin, enabled, payload_path, payload_sha256, notes)
             VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NULL, NULL, ?)`,
            [
                game,
                name,
                icon_color || '#FFFFFF',
                process_name || null,
                injection_method || 'LoadLibrary',
                kill_processes || null,
                launch_params || null,
                steam_app_id || null,
                requires_admin === 'true' || requires_admin === '1' ? 1 : 0,
                enabled === 'false' || enabled === '0' ? 0 : 1,
                notes || null,
            ]
        );

        const cheatId = result.insertId;

        let payloadPath = null;
        let payloadHash = null;
        if (req.file) {
            payloadHash = hashFile(req.file.path);
            payloadPath = renamePayload(req.file.path, cheatId);

            await pool.execute(
                'UPDATE cheats SET payload_path = ?, payload_sha256 = ? WHERE id = ?',
                [payloadPath, payloadHash, cheatId]
            );
        }

        try {
            const [admins] = await pool.execute("SELECT id FROM users WHERE role = 'admin'");
            for (const admin of admins) {
                await pool.execute(
                    `INSERT INTO user_cheats (user_id, cheat_id, expires_at, status)
                     VALUES (?, ?, NULL, 'active')
                     ON DUPLICATE KEY UPDATE status = 'active', expires_at = NULL`,
                    [admin.id, cheatId]
                );
            }
            console.log(`[ADMIN] Auto-assigned cheat ${cheatId} to ${admins.length} admin(s)`);
        } catch (autoErr) {
            console.error('[WARN] Auto-assign failed:', autoErr.message);
        }

        console.log(`[ADMIN] Created cheat ID ${cheatId}: ${game} - ${name} (payload: ${payloadPath || 'none'})`);
        return res.json({ success: true, id: cheatId, payload_path: payloadPath, payload_sha256: payloadHash });
    } catch (err) {
        if (req.file) safeUnlink(req.file.path);
        console.error('[ERROR] Create cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

router.put('/cheats/:id', requireAdminJWT, uploadPayload.single('payload'), async (req, res) => {
    try {
        const id = parseInt(req.params.id, 10);
        if (Number.isNaN(id)) {
            if (req.file) safeUnlink(req.file.path);
            return res.status(400).json({ success: false, error: 'Invalid cheat ID.' });
        }

        const [existing] = await pool.execute('SELECT id, payload_path FROM cheats WHERE id = ?', [id]);
        if (existing.length === 0) {
            if (req.file) safeUnlink(req.file.path);
            return res.status(404).json({ success: false, error: 'Cheat not found.' });
        }

        const {
            game,
            name,
            icon_color,
            process_name,
            injection_method,
            kill_processes,
            launch_params,
            steam_app_id,
            requires_admin,
            enabled,
            notes,
        } = req.body;

        await pool.execute(
            `UPDATE cheats SET
                game = COALESCE(?, game),
                name = COALESCE(?, name),
                icon_color = COALESCE(?, icon_color),
                process_name = COALESCE(?, process_name),
                injection_method = COALESCE(?, injection_method),
                kill_processes = COALESCE(?, kill_processes),
                launch_params = COALESCE(?, launch_params),
                steam_app_id = COALESCE(?, steam_app_id),
                requires_admin = ?,
                enabled = ?,
                notes = COALESCE(?, notes)
             WHERE id = ?`,
            [
                game || null,
                name || null,
                icon_color || null,
                process_name || null,
                injection_method || null,
                kill_processes || null,
                launch_params || null,
                steam_app_id || null,
                requires_admin === 'true' || requires_admin === '1'
                    ? 1
                    : (requires_admin === 'false' || requires_admin === '0' ? 0 : null),
                enabled === 'false' || enabled === '0' ? 0 : 1,
                notes !== undefined ? notes : null,
                id,
            ]
        );

        if (req.file) {
            const oldPayload = existing[0].payload_path;
            if (oldPayload) {
                safeUnlink(payloadFullPath(oldPayload));
            }

            const payloadHash = hashFile(req.file.path);
            const payloadPath = renamePayload(req.file.path, id);

            await pool.execute(
                'UPDATE cheats SET payload_path = ?, payload_sha256 = ? WHERE id = ?',
                [payloadPath, payloadHash, id]
            );
        }

        console.log(`[ADMIN] Updated cheat ID ${id}`);
        return res.json({ success: true });
    } catch (err) {
        if (req.file) safeUnlink(req.file.path);
        console.error('[ERROR] Update cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

router.delete('/cheats/:id', requireAdminJWT, async (req, res) => {
    try {
        const id = parseInt(req.params.id, 10);
        if (Number.isNaN(id)) {
            return res.status(400).json({ success: false, error: 'Invalid cheat ID.' });
        }

        const [rows] = await pool.execute('SELECT payload_path FROM cheats WHERE id = ?', [id]);
        if (rows.length === 0) {
            return res.status(404).json({ success: false, error: 'Cheat not found.' });
        }

        const payloadPath = rows[0].payload_path;

        await pool.execute('DELETE FROM cheats WHERE id = ?', [id]);

        if (payloadPath) {
            safeUnlink(payloadFullPath(payloadPath));
            console.log(`[ADMIN] Deleted payload file: ${payloadPath}`);
        }

        auditLog('DELETE_CHEAT', req.adminUser.nickname, `Cheat ID ${id}`);
        return res.json({ success: true });
    } catch (err) {
        console.error('[ERROR] Delete cheat:', err.message);
        return res.status(500).json({ success: false, error: 'Internal server error.' });
    }
});

module.exports = router;
