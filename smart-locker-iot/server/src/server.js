// server.js
// Entry point: REST API (untuk NodeMCU & dashboard) + menyajikan web dashboard statis.

require('dotenv').config();
const path = require('path');
const express = require('express');
const cors = require('cors');

const { seedLockers } = require('./db');
const cardsRouter = require('./routes/cards');
const lockersRouter = require('./routes/lockers');
const logsRouter = require('./routes/logs');

const PORT = process.env.PORT || 3000;
const TOTAL_BOX = parseInt(process.env.TOTAL_BOX, 10) || 4;

seedLockers(TOTAL_BOX);

const app = express();
app.use(cors());
app.use(express.json());

// REST API
app.use('/api/cards', cardsRouter);
app.use('/api/lockers', lockersRouter);
app.use('/api/logs', logsRouter);

app.get('/api/health', (req, res) => res.json({ ok: true, time: new Date().toISOString() }));

// Web dashboard statis (folder ../../web)
app.use(express.static(path.join(__dirname, '..', '..', 'web')));

app.listen(PORT, () => {
  console.log(`Smart Locker server berjalan di http://localhost:${PORT}`);
  console.log(`NodeMCU harus diarahkan ke IP komputer/server ini pada port ${PORT}`);
});
