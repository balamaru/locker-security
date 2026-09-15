// routes/lockers.js
// Status box/loker saat ini (available / in-use, dan siapa yang memegang kunci).

const express = require('express');
const { db } = require('../db');

const router = express.Router();

// GET /api/lockers -> status semua box
router.get('/', (req, res) => {
  const rows = db.prepare(`
    SELECT l.box, l.status, l.current_uid, c.name AS current_name, l.updated_at
    FROM lockers l
    LEFT JOIN cards c ON c.uid = l.current_uid
    ORDER BY l.box ASC
  `).all();
  res.json(rows);
});

module.exports = router;
