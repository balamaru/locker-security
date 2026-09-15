// app.js - dashboard sederhana, polling API setiap 4 detik (tanpa framework,
// cukup untuk kebutuhan monitoring prototipe skripsi).

const REFRESH_MS = 4000;

async function fetchJSON(url, options) {
  const res = await fetch(url, options);
  if (!res.ok) {
    const body = await res.json().catch(() => ({}));
    throw new Error(body.error || `Request gagal (${res.status})`);
  }
  return res.json();
}

function fmtTime(ts) {
  return ts ? ts.replace('T', ' ') : '-';
}

async function loadLockers() {
  const lockers = await fetchJSON('/api/lockers');
  const grid = document.getElementById('lockers-grid');
  grid.innerHTML = lockers.map((l) => `
    <div class="locker-box ${l.status}">
      <div class="box-title">Box ${l.box}</div>
      <div class="box-status">${l.status === 'available' ? 'Tersedia' : `Dipegang: ${l.current_name || l.current_uid}`}</div>
    </div>
  `).join('');
}

async function loadCards() {
  const cards = await fetchJSON('/api/cards');
  const tbody = document.querySelector('#cards-table tbody');
  if (cards.length === 0) {
    tbody.innerHTML = `<tr class="empty-row"><td colspan="4">Belum ada kartu terdaftar</td></tr>`;
    return;
  }
  tbody.innerHTML = cards.map((c) => `
    <tr>
      <td class="mono">${c.uid}</td>
      <td>${c.name}</td>
      <td class="mono">${fmtTime(c.created_at)}</td>
      <td><button class="delete-btn" data-uid="${c.uid}">Hapus</button></td>
    </tr>
  `).join('');

  tbody.querySelectorAll('.delete-btn').forEach((btn) => {
    btn.addEventListener('click', async () => {
      if (!confirm(`Hapus kartu ${btn.dataset.uid}?`)) return;
      await fetchJSON(`/api/cards/${encodeURIComponent(btn.dataset.uid)}`, { method: 'DELETE' });
      loadCards();
    });
  });
}

async function loadLogs() {
  const logs = await fetchJSON('/api/logs?limit=50');
  const tbody = document.querySelector('#logs-table tbody');
  if (logs.length === 0) {
    tbody.innerHTML = `<tr class="empty-row"><td colspan="6">Belum ada aktivitas</td></tr>`;
    return;
  }
  tbody.innerHTML = logs.map((l) => `
    <tr>
      <td class="mono">${fmtTime(l.timestamp)}</td>
      <td class="mono">${l.uid || '-'}</td>
      <td>${l.name || '-'}</td>
      <td>${l.box || '-'}</td>
      <td class="action-${l.action}">${l.action}</td>
      <td>${l.message || ''}</td>
    </tr>
  `).join('');
}

async function refreshAll() {
  try {
    await Promise.all([loadLockers(), loadCards(), loadLogs()]);
    document.getElementById('status-indicator').textContent =
      `Terhubung — pembaruan terakhir ${new Date().toLocaleTimeString('id-ID')}`;
  } catch (err) {
    document.getElementById('status-indicator').textContent = `Gagal memuat data: ${err.message}`;
  }
}

document.getElementById('add-card-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const uid = document.getElementById('input-uid').value;
  const name = document.getElementById('input-name').value;
  try {
    await fetchJSON('/api/cards', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ uid, name }),
    });
    e.target.reset();
    loadCards();
  } catch (err) {
    alert(err.message);
  }
});

refreshAll();
setInterval(refreshAll, REFRESH_MS);
