const jwt = require('jsonwebtoken');
const config = require('../config');

function requireAdminJWT(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1];

    if (!token) {
        return res.status(401).json({ success: false, error: 'Token missing.' });
    }

    try {
        const decoded = jwt.verify(token, config.JWT_SECRET);
        if (decoded.role !== 'admin') {
            return res.status(403).json({ success: false, error: 'Admin access required.' });
        }

        req.adminUser = decoded;
        return next();
    } catch (err) {
        return res.status(403).json({ success: false, error: 'Invalid or expired token.' });
    }
}

module.exports = {
    requireAdminJWT,
};
