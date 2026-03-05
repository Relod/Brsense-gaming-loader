const express = require('express');
const router = express.Router();
const path = require('path');
const fs = require('fs');
const config = require('../config');

// Current client version — update this when releasing a new build
const CURRENT_VERSION = '1.0.0';

// GET /api/check-update?v=X.X.X
router.get('/check-update', (req, res) => {
    const clientVersion = req.query.v || '0.0.0';

    const updateAvailable = compareVersions(CURRENT_VERSION, clientVersion) > 0;

    const updatePayload = {
        success: true,
        update_available: updateAvailable,
        latest_version: CURRENT_VERSION,
        client_version: clientVersion,
    };

    // Only include download info if update is available
    if (updateAvailable) {
        const updateFile = path.join(config.PAYLOADS_DIR, 'updates', `BRSense_${CURRENT_VERSION}.exe`);
        updatePayload.download_url = `/api/download-update`;
        updatePayload.changelog = 'Bug fixes and performance improvements.';
        updatePayload.file_exists = fs.existsSync(updateFile);
    }

    return res.json(updatePayload);
});

// GET /api/download-update — download latest exe
router.get('/download-update', (req, res) => {
    const updateFile = path.join(config.PAYLOADS_DIR, 'updates', `BRSense_${CURRENT_VERSION}.exe`);

    if (!fs.existsSync(updateFile)) {
        return res.status(404).json({ success: false, error: 'Update file not found.' });
    }

    // Security: ensure path is within payloads directory
    const resolved = path.resolve(updateFile);
    if (!resolved.startsWith(path.resolve(config.PAYLOADS_DIR))) {
        return res.status(403).json({ success: false, error: 'Invalid path.' });
    }

    res.setHeader('Content-Type', 'application/octet-stream');
    res.setHeader('Content-Disposition', `attachment; filename="BRSense_${CURRENT_VERSION}.exe"`);
    return res.sendFile(resolved);
});

// Semver compare: returns >0 if a > b, 0 if equal, <0 if a < b
function compareVersions(a, b) {
    const pa = a.split('.').map(Number);
    const pb = b.split('.').map(Number);
    for (let i = 0; i < 3; i++) {
        const diff = (pa[i] || 0) - (pb[i] || 0);
        if (diff !== 0) return diff;
    }
    return 0;
}

module.exports = router;
