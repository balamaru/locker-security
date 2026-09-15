// routes/cards.js
// CRUD kartu RFID terdaftar + endpoint verifikasi yang dipanggil NodeMCU.
// Ini implementasi saran no. 2: pendaftaran/penghapusan objek RFID
// dilakukan di database, bukan di source code mikrokontroler.

const express = require('express');
const { db } = require('../db');

const router = express.Router();

// GET /api/cards  -> daftar semua kartu terdaftar
router.get('/', (req, res) => {
  const cards = db.prepare('SELECT uid, name, created_at FROM cards ORDER BY created_at DESC').all();
  res.json(cards);
});

// POST /api/cards  { uid, name } -> daftarkan kartu baru
router.post('/', (req, res) => {
  const { uid, name } = req.body;
  if (!uid || !name) {
    return res.status(400).json({ error: 'uid dan name wajib diisi' });
  }
  const normalizedUid = String(uid).trim().toUpperCase();
  try {
    db.prepare('INSERT INTO cards (uid, name) VALUES (?, ?)').run(normalizedUid, name.trim());
    res.status(201).json({ uid: normalizedUid, name: name.trim() });
  } catch (err) {
    if (err.code === 'SQLITE_CONSTRAINT_PRIMARYKEY') {
      return res.status(409).json({ error: 'UID sudah terdaftar' });
    }
    res.status(500).json({ error: 'Gagal menyimpan kartu', detail: err.message });
  }
});

// DELETE /api/cards/:uid -> hapus kartu terdaftar
router.delete('/:uid', (req, res) => {
  const uid = req.params.uid.trim().toUpperCase();
  const result = db.prepare('DELETE FROM cards WHERE uid = ?').run(uid);
  if (result.changes === 0) {
    return res.status(404).json({ error: 'UID tidak ditemukan' });
  }
  res.json({ deleted: uid });
});

// GET /api/cards/verify?uid=XX%20XX%20XX%20XX -> dipanggil NodeMCU setiap tap kartu
router.get('/verify', (req, res) => {
  const uid = (req.query.uid || '').trim().toUpperCase();
  if (!uid) return res.status(400).json({ valid: false, error: 'uid kosong' });

  const card = db.prepare('SELECT uid, name FROM cards WHERE uid = ?').get(uid);
  if (card) {
    res.json({ valid: true, uid: card.uid, name: card.name });
  } else {
    res.json({ valid: false });
  }
});

module.exports = router;
