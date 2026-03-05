const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const multer = require('multer');
const config = require('../config');

function ensurePayloadsDir() {
    if (!fs.existsSync(config.PAYLOADS_DIR)) {
        fs.mkdirSync(config.PAYLOADS_DIR, { recursive: true });
    }
}

const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        ensurePayloadsDir();
        cb(null, config.PAYLOADS_DIR);
    },
    filename: (req, file, cb) => {
        const ext = path.extname(file.originalname) || '.dll';
        cb(null, `upload_${Date.now()}${ext}`);
    },
});

const uploadPayload = multer({
    storage,
    limits: { fileSize: 50 * 1024 * 1024 },
    fileFilter: (req, file, cb) => cb(null, true),
});

function hashFile(filePath) {
    const data = fs.readFileSync(filePath);
    return crypto.createHash('sha256').update(data).digest('hex');
}

function renamePayload(tempPath, cheatId) {
    const ext = path.extname(tempPath) || '.dll';
    const finalName = `cheat_${cheatId}${ext}`;
    const finalPath = path.join(config.PAYLOADS_DIR, finalName);

    if (fs.existsSync(finalPath) && finalPath !== tempPath) {
        fs.unlinkSync(finalPath);
    }

    fs.renameSync(tempPath, finalPath);
    return finalName;
}

function safeUnlink(filePath) {
    if (filePath && fs.existsSync(filePath)) {
        fs.unlinkSync(filePath);
    }
}

function payloadFullPath(relativePath) {
    return path.join(config.PAYLOADS_DIR, relativePath);
}

module.exports = {
    uploadPayload,
    hashFile,
    renamePayload,
    safeUnlink,
    payloadFullPath,
};
