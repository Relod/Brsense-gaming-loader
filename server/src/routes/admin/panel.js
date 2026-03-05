const express = require('express');
const fs = require('fs');
const path = require('path');

const router = express.Router();

router.get('/', (req, res) => {
    const htmlPath = path.join(__dirname, '..', '..', 'admin-panel.html');
    if (fs.existsSync(htmlPath)) {
        return res.sendFile(htmlPath);
    }

    return res.status(500).send('Admin panel HTML file not found.');
});

module.exports = router;
