// db.js
// Inisialisasi database SQLite (file lokal, tidak perlu server DB terpisah).
// Ini menggantikan pendekatan lama: daftar kartu hardcode di source code
// Arduino. Sekarang pendaftaran/penghapusan kartu cukup lewat API/dashboard,
// tanpa perlu re-upload firmware.

const path = require('path');
const Database = require('better-sqlite3');

const DB_PATH = path.join(__dirname, '..', 'database', 'locker.db');
const db = new Database(DB_PATH);

db.pragma('journal_mode = WAL');

db.exec(`
  CREATE TABLE IF NOT EXISTS cards (
    uid        TEXT PRIMARY KEY,
    name       TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
  );

  CREATE TABLE IF NOT EXISTS lockers (
    box         INTEGER PRIMARY KEY,
    status      TEXT NOT NULL DEFAULT 'available',   -- 'available' | 'in-use'
    current_uid TEXT,
    updated_at  TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
  );

  CREATE TABLE IF NOT EXISTS logs (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    uid       TEXT,
    name      TEXT,
    box       INTEGER,
    action    TEXT NOT NULL,   -- 'GRANTED' | 'DENIED' | 'TAKE' | 'RETURN' | 'BUSY' | 'INVALID_BOX'
    message   TEXT,
    timestamp TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
  );
`);

function seedLockers(totalBox) {
  const insert = db.prepare(
    `INSERT OR IGNORE INTO lockers (box, status, current_uid) VALUES (?, 'available', NULL)`
  );
  const seedMany = db.transaction((n) => {
    for (let i = 1; i <= n; i++) insert.run(i);
  });
  seedMany(totalBox);
}

module.exports = { db, seedLockers };
