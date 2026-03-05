const pool = require('../database');

async function hasPendingHwidReset(userId) {
    const [existing] = await pool.execute(
        'SELECT id FROM hwid_reset_requests WHERE user_id = ? AND status = "pending"',
        [userId]
    );

    return existing.length > 0;
}

async function createHwidResetRequest(userId, reason) {
    await pool.execute(
        'INSERT INTO hwid_reset_requests (user_id, reason) VALUES (?, ?)',
        [userId, reason]
    );
}

module.exports = {
    hasPendingHwidReset,
    createHwidResetRequest,
};
