const express = require('express');
const router = express.Router();
const { rateLimit, authenticateToken } = require('../middleware');
const authController = require('../controllers/auth-controller');

router.post('/login', rateLimit('login', 10, 60_000), authController.login);
router.get('/cheats', authenticateToken, authController.refreshCheats);
router.post('/heartbeat', authenticateToken, authController.heartbeat);
router.post('/refresh', authenticateToken, authController.refreshToken);
router.post('/hwid_reset', authenticateToken, rateLimit('hwid_reset', 5, 60_000), authController.requestHwidReset);
router.post('/hwid_reset_by_creds', rateLimit('hwid_reset_creds', 3, 120_000), authController.requestHwidResetByCreds);

module.exports = router;
