const pool = require('../database');

function formatTimeLeft(expiresAt) {
    if (!expiresAt) return { timeLeft: 'LIFETIME', isExpired: false };

    const now = new Date();
    const exp = new Date(expiresAt);
    const diff = exp - now;

    if (diff <= 0) {
        return { timeLeft: 'EXPIRED', isExpired: true };
    }

    const days = Math.floor(diff / (1000 * 60 * 60 * 24));
    const hours = Math.floor((diff % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60));

    if (days > 0) {
        return { timeLeft: `${days}d ${hours}h`, isExpired: false };
    }

    const mins = Math.floor((diff % (1000 * 60 * 60)) / (1000 * 60));
    return { timeLeft: `${hours}h ${mins}m`, isExpired: false };
}

function mapCheatRow(ch) {
    const { timeLeft, isExpired } = formatTimeLeft(ch.expires_at);

    return {
        id: ch.id,
        game: ch.game,
        name: ch.name,
        icon_color: ch.icon_color,
        hash: ch.payload_sha256 || '',
        process: ch.process_name || '',
        injection_method: ch.injection_method || 'LoadLibrary',
        kill_processes: ch.kill_processes || '',
        launch_params: ch.launch_params || '',
        steam_app_id: ch.steam_app_id || '',
        requires_admin: !!ch.requires_admin,
        notes: ch.notes || '',
        time_left: timeLeft,
        status: isExpired ? 'expired' : ch.status,
    };
}

async function listLicensedCheatsForUser(userId) {
    const [rows] = await pool.execute(
        `SELECT c.id, c.game, c.name, c.icon_color, c.process_name,
                c.injection_method, c.kill_processes, c.launch_params,
                c.steam_app_id, c.requires_admin, c.enabled,
                c.payload_sha256, c.notes, uc.expires_at, uc.status
         FROM user_cheats uc
         JOIN cheats c ON uc.cheat_id = c.id
         WHERE uc.user_id = ? AND c.enabled = 1
         ORDER BY c.game ASC`,
        [userId]
    );

    return rows.map(mapCheatRow);
}

module.exports = {
    listLicensedCheatsForUser,
};
