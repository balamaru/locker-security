# Smart Locker IoT — RFID, Keypad, Database & Web Monitoring

Sistem loker pintar berbasis RFID dan Keypad untuk penitipan/pengambilan
kunci, dikembangkan sebagai bagian dari tugas akhir (skripsi) S1. Sistem ini
menggabungkan Arduino Uno R3, NodeMCU ESP8266, notifikasi Telegram, serta
**database dan web dashboard monitoring** sebagai pengembangan lanjutan dari
prototipe awal.

## Arsitektur

```
 [Modul RFID]      [Modul Keypad]
      │                  │
      ▼                  ▼
 [NodeMCU ESP8266] ───▲───[Arduino Uno R3]
      │  (Serial RX/TX, protokol lihat firmware/)
      │
      ├──► Server (Node.js + Express + SQLite)
      │        ├─ REST API (verifikasi kartu, catat log, status loker)
      │        └─ Web Dashboard (monitoring & CRUD kartu)
      │
      └──► Telegram Bot (notifikasi real-time)

 [Arduino Uno R3] ──► Motor Servo (4x) + LED Indikator
```

**Perubahan dari prototipe awal:** validasi kartu RFID sebelumnya
di-*hardcode* di source code Arduino. Pada versi ini, validasi dipindahkan ke
**database di server** — pendaftaran dan penghapusan kartu dilakukan lewat
web dashboard, tanpa perlu meng-*upload* ulang firmware setiap ada perubahan
data pengguna (lihat bagian Saran pada laporan skripsi, poin 2 & 3).

Kondisi awal setiap box berisi kunci (bukan kosong), sehingga sistem
membedakan dua aksi: **TAKE** (mengambil kunci) dan **RETURN**
(mengembalikan kunci, hanya bisa oleh kartu yang sama yang mengambilnya).

## Struktur Folder

```
smart-locker-iot/
├── firmware/
│   ├── arduino_uno/arduino_uno.ino   # Kontrol keypad, servo, LED
│   └── esp8266/esp8266.ino           # RFID, WiFi, panggil API server, Telegram
├── server/
│   ├── src/
│   │   ├── server.js                 # Entry point Express
│   │   ├── db.js                     # Setup SQLite + skema tabel
│   │   └── routes/
│   │       ├── cards.js              # CRUD kartu + endpoint verifikasi
│   │       ├── lockers.js            # Status box saat ini
│   │       └── logs.js               # Riwayat aktivitas
│   ├── database/                     # File locker.db dibuat otomatis di sini
│   ├── package.json
│   └── .env.example
├── web/
│   ├── index.html                    # Dashboard monitoring
│   ├── style.css
│   └── app.js                        # Polling API, render tabel
├── docs/images/                      # Taruh diagram/flowchart dari skripsi
├── .gitignore
└── README.md
```

## Menjalankan Server (Database + Web Dashboard)

Prasyarat: [Node.js](https://nodejs.org/) versi 18 ke atas.

```bash
cd server
cp .env.example .env      # sesuaikan PORT & TOTAL_BOX bila perlu
npm install
npm start
```

Server berjalan di `http://<IP-komputer-anda>:3000`. Buka alamat tersebut di
browser untuk mengakses **web dashboard** (daftar kartu, status box, riwayat
aktivitas). Catat IP komputer ini (`ipconfig` / `ifconfig`) karena akan
dipakai di firmware NodeMCU.

### Endpoint API Utama

| Method | Endpoint              | Keterangan                                  |
|--------|------------------------|----------------------------------------------|
| GET    | `/api/cards`           | Daftar kartu terdaftar                       |
| POST   | `/api/cards`           | Daftarkan kartu baru `{ uid, name }`         |
| DELETE | `/api/cards/:uid`      | Hapus kartu                                  |
| GET    | `/api/cards/verify?uid=`| Verifikasi kartu (dipanggil NodeMCU)        |
| GET    | `/api/lockers`         | Status semua box saat ini                    |
| GET    | `/api/logs?limit=50`   | Riwayat aktivitas terbaru                    |
| POST   | `/api/logs`             | Catat event baru (dipanggil NodeMCU)        |

## Meng-upload Firmware

1. **Arduino Uno** (`firmware/arduino_uno/arduino_uno.ino`)
   Library yang dibutuhkan: `Keypad`, `Servo` (bawaan Arduino IDE / instal
   lewat Library Manager).

2. **NodeMCU ESP8266** (`firmware/esp8266/esp8266.ino`)
   Library yang dibutuhkan (instal lewat Library Manager):
   `MFRC522`, `ESP8266WiFi`, `ESP8266HTTPClient`, `UniversalTelegramBot`,
   **`ArduinoJson`**.

   Sebelum upload, sesuaikan di bagian atas file:
   ```cpp
   const char* ssid = "...";
   const char* password = "...";
   const char* botToken = "...";
   #define CHAT_ID "..."
   const char* SERVER_HOST = "192.168.1.100"; // IP komputer server
   const int   SERVER_PORT = 3000;
   ```

## Alur Kerja Singkat

1. Kartu ditempelkan → NodeMCU membaca UID → memanggil `GET /api/cards/verify`.
2. Server mengecek UID di database, membalas valid/tidak beserta nama.
3. NodeMCU meneruskan hasil ke Arduino via Serial, sekaligus mencatat log &
   mengirim notifikasi Telegram.
4. Jika valid, pengguna memilih box lewat keypad → Arduino menggerakkan
   servo & mengirim event (TAKE/RETURN/BUSY) balik ke NodeMCU.
5. NodeMCU mencatat event tersebut ke server (`POST /api/logs`) dan
   mengirim notifikasi Telegram lanjutan.
6. Semua aktivitas dapat dipantau real-time lewat **web dashboard**.


## Saran Pengembangan Lanjutan

- Autentikasi untuk web dashboard (saat ini masih terbuka di jaringan lokal).
- Migrasi ke relay + door lock elektrik untuk jumlah pintu yang lebih banyak.
- Enkripsi komunikasi NodeMCU ↔ server (HTTPS) bila diakses di luar jaringan lokal.

---