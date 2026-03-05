const express = require('express');

const authRoutes = require('./admin/auth');
const cheatsRoutes = require('./admin/cheats');
const usersRoutes = require('./admin/users');
const sessionsRoutes = require('./admin/sessions');
const hwidResetsRoutes = require('./admin/hwid-resets');
const panelRoutes = require('./admin/panel');

const router = express.Router();

router.use(authRoutes);
router.use(cheatsRoutes);
router.use(usersRoutes);
router.use(sessionsRoutes);
router.use(hwidResetsRoutes);
router.use(panelRoutes);

module.exports = router;
