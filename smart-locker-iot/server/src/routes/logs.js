// routes/logs.js
// Implementasi saran no. 3 (bagian pencatatan): setiap aktivitas RFID/box
// dikirim NodeMCU ke sini lalu disimpan permanen di database, sehingga bisa
// dipantau lewat web dashboard tanpa bergantung pada Serial Monitor.

const express = require('express');
const { db } = require('../db');

const router = express.Router();

const VALID_ACTIONS = ['GRANTED', 'DENIED', 'TAKE', 'RETURN', 'BUSY', 'INVALID_BOX'];

// GET /api/logs?limit=50 -> riwayat aktivitas terbaru
router.get('/', (req, res) => {
  const limit = Math.min(parseInt(req.query.limit, 10) || 50, 500);
  const rows = db.prepare(
    'SELECT id, uid, name, box, action, message, timestamp FROM logs ORDER BY id DESC LIMIT ?'
  ).all(limit);
  res.json(rows);
});

// POST /api/logs  { uid, name, box, action, message } -> dikirim oleh NodeMCU
router.post('/', (req, res) => {
  const { uid = null, name = null, box = null, action, message = null } = req.body;

  if (!action || !VALID_ACTIONS.includes(action)) {
    return res.status(400).json({ error: `action harus salah satu dari: ${VALID_ACTIONS.join(', ')}` });
  }

  const insertLog = db.prepare(
    'INSERT INTO logs (uid, name, box, action, message) VALUES (?, ?, ?, ?, ?)'
  );
  const updateLockerTake = db.prepare(
    "UPDATE lockers SET status = 'in-use', current_uid = ?, updated_at = datetime('now','localtime') WHERE box = ?"
  );
  const updateLockerReturn = db.prepare(
    "UPDATE lockers SET status = 'available', current_uid = NULL, updated_at = datetime('now','localtime') WHERE box = ?"
  );

  const tx = db.transaction(() => {
    insertLog.run(uid, name, box, action, message);
    if (action === 'TAKE' && box) updateLockerTake.run(uid, box);
    if (action === 'RETURN' && box) updateLockerReturn.run(box);
  });
  tx();

  res.status(201).json({ ok: true });
});

module.exports = router;
