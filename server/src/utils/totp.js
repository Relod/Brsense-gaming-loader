// ── TOTP (RFC 6238) implementation using Node.js crypto ──────────────────────
const crypto = require('crypto');

/**
 * Generate a random base32-encoded secret for TOTP setup
 */
function generateSecret(length = 20) {
    const buffer = crypto.randomBytes(length);
    return base32Encode(buffer);
}

/**
 * Generate TOTP code for a given secret and time
 */
function generateTOTP(secret, timeStep = 30, digits = 6) {
    const time = Math.floor(Date.now() / 1000 / timeStep);
    const timeBuffer = Buffer.alloc(8);
    timeBuffer.writeUInt32BE(0, 0);
    timeBuffer.writeUInt32BE(time, 4);

    const key = base32Decode(secret);
    const hmac = crypto.createHmac('sha1', key);
    hmac.update(timeBuffer);
    const hash = hmac.digest();

    const offset = hash[hash.length - 1] & 0x0f;
    const code = (
        ((hash[offset] & 0x7f) << 24) |
        ((hash[offset + 1] & 0xff) << 16) |
        ((hash[offset + 2] & 0xff) << 8) |
        (hash[offset + 3] & 0xff)
    ) % Math.pow(10, digits);

    return String(code).padStart(digits, '0');
}

/**
 * Verify a TOTP code (checks current + adjacent time windows for clock drift)
 */
function verifyTOTP(secret, code, window = 1) {
    const timeStep = 30;
    const now = Math.floor(Date.now() / 1000 / timeStep);

    for (let i = -window; i <= window; i++) {
        const time = now + i;
        const timeBuffer = Buffer.alloc(8);
        timeBuffer.writeUInt32BE(0, 0);
        timeBuffer.writeUInt32BE(time, 4);

        const key = base32Decode(secret);
        const hmac = crypto.createHmac('sha1', key);
        hmac.update(timeBuffer);
        const hash = hmac.digest();

        const offset = hash[hash.length - 1] & 0x0f;
        const otp = (
            ((hash[offset] & 0x7f) << 24) |
            ((hash[offset + 1] & 0xff) << 16) |
            ((hash[offset + 2] & 0xff) << 8) |
            (hash[offset + 3] & 0xff)
        ) % 1000000;

        if (String(otp).padStart(6, '0') === code) {
            return true;
        }
    }
    return false;
}

/**
 * Generate otpauth:// URI for QR code / manual entry in authenticator apps
 */
function generateOTPAuthURI(secret, username, issuer = 'BRSense') {
    return `otpauth://totp/${encodeURIComponent(issuer)}:${encodeURIComponent(username)}?secret=${secret}&issuer=${encodeURIComponent(issuer)}&algorithm=SHA1&digits=6&period=30`;
}

// ── Base32 helpers ──────────────────────────────────────────────────────────
const BASE32_CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567';

function base32Encode(buffer) {
    let bits = '';
    for (const byte of buffer) {
        bits += byte.toString(2).padStart(8, '0');
    }
    let result = '';
    for (let i = 0; i < bits.length; i += 5) {
        const chunk = bits.substring(i, i + 5).padEnd(5, '0');
        result += BASE32_CHARS[parseInt(chunk, 2)];
    }
    return result;
}

function base32Decode(base32) {
    let bits = '';
    for (const char of base32.toUpperCase()) {
        const val = BASE32_CHARS.indexOf(char);
        if (val === -1) continue;
        bits += val.toString(2).padStart(5, '0');
    }
    const bytes = [];
    for (let i = 0; i + 8 <= bits.length; i += 8) {
        bytes.push(parseInt(bits.substring(i, i + 8), 2));
    }
    return Buffer.from(bytes);
}

module.exports = {
    generateSecret,
    generateTOTP,
    verifyTOTP,
    generateOTPAuthURI,
};
