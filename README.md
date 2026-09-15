# (Enchant Version) Deskripsi Sistem, Alur, dan Saran

## Deskripsi Sistem

NodeMCU ESP8266 dan Arduino Uno R3 bertindak sebagai dua pengendali utama yang saling berkomunikasi secara serial melalui jalur RX/TX. Pembagian tugas keduanya sebagai berikut:

- **NodeMCU ESP8266** bertugas membaca kartu RFID melalui modul RFID (RC522), meneruskan UID kartu yang terbaca ke Arduino Uno melalui komunikasi serial, serta mengirimkan seluruh hasil proses—baik status pembacaan kartu maupun status pergerakan motor servo—ke aplikasi Telegram melalui Telegram Bot. NodeMCU juga menyediakan koneksi internet (WiFi) sebagai jembatan komunikasi antara sistem dan Telegram.
- **Arduino Uno R3** bertugas sebagai pengendali utama pemrosesan logika sistem. Arduino menerima UID dari NodeMCU, kemudian **melakukan proses verifikasi** dengan mencocokkan UID tersebut terhadap daftar kartu terdaftar (`validCards`). Jika UID valid, Arduino mengizinkan input berikutnya dari modul keypad, yang digunakan pengguna untuk memilih box mana yang hendak dibuka. Berdasarkan pilihan tersebut, Arduino mengendalikan pergerakan motor servo yang sesuai serta indikator LED, lalu mengirimkan status prosesnya kembali ke NodeMCU melalui jalur serial untuk diteruskan sebagai notifikasi Telegram.

Modul RFID digunakan untuk membaca identitas objek (card ID) yang telah didaftarkan, sedangkan modul keypad digunakan sebagai input oleh pengguna untuk memilih box yang akan dibuka. Setelah seluruh data input diproses oleh mikrokontroler, sistem mengeluarkan perintah output berupa: indikator LED (hijau untuk akses diterima, merah untuk akses ditolak), notifikasi pada Telegram yang memberitahukan aktivitas pada input, serta pergerakan motor servo yang dikendalikan langsung oleh Arduino Uno. Pergerakan motor servo merupakan hasil akhir dari keseluruhan rangkaian proses input–proses–output (IPO) pada sistem.

> **Catatan:** Pada versi sebelumnya (versi yang diimplementasi pada skripsi), deskripsi menyatakan bahwa proses verifikasi RFID dilakukan oleh NodeMCU ESP8266. Berdasarkan tinjauan kode program, proses pencocokan UID terhadap kartu terdaftar sesungguhnya dilakukan pada Arduino Uno R3; NodeMCU hanya berperan sebagai pembaca RFID dan penerus data (bridge) serta pengirim notifikasi Telegram. Bagian ini telah disesuaikan agar konsisten dengan implementasi program.

## Alur Sistem

Ketika sistem prototipe dijalankan, kontroler utama (NodeMCU ESP8266) akan terlebih dahulu terkoneksi dengan internet sebagai implementasi dari konsep *Internet of Things* (IoT). Selanjutnya, modul RFID melakukan pendeteksian terhadap objek (card ID). Dalam penelitian ini, digunakan 5 objek kartu, di mana 3 di antaranya didaftarkan agar dikenali oleh sistem.

Ketika modul RFID mendeteksi sebuah kartu, UID kartu tersebut diteruskan ke Arduino Uno untuk diverifikasi, dan terdapat dua kemungkinan hasil:

1. **Kartu tidak dikenali** — indikator LED merah menyala, dan sistem mengirimkan notifikasi Telegram yang memberitahukan bahwa terdapat upaya akses oleh pengguna yang tidak dikenali ("Access Denied – Invalid Card").
2. **Kartu dikenali** — indikator LED hijau menyala, dan sistem mengirimkan notifikasi Telegram yang memberitahukan bahwa pengguna dengan UID terdaftar berhasil terverifikasi ("Access Granted – UID: XXXXXXXX"). Pengguna kemudian dapat memilih box yang ingin dibuka dengan menekan tombol pada keypad.

Penelitian ini membatasi jumlah box yang digunakan sebanyak 4 buah. Setelah pengguna memilih box melalui keypad, sistem mengirimkan notifikasi lanjutan pada Telegram yang menyatakan bahwa box dengan nomor terpilih telah terbuka. Motor servo pada box tersebut bergerak sejauh 90° (dari posisi vertikal ke horizontal) dan bertahan pada posisi terbuka selama 10 detik, sebelum kembali ke posisi semula (vertikal) untuk menutup box tersebut.

**Catatan penting:** kondisi awal setiap box/loker berisi kunci (tidak kosong)—berbeda dengan kebanyakan penelitian sejenis yang umumnya mengasumsikan loker kosong pada kondisi awal. Karena itu, sistem juga menangani logika pengembalian kunci: jika UID yang memilih box tertentu sama dengan UID yang sebelumnya mengambil kunci dari box tersebut, box akan terbuka untuk pengembalian kunci dan status box kembali menjadi "available". Jika box sedang digunakan oleh UID lain, sistem akan menolak permintaan dan menampilkan notifikasi bahwa box tersebut sedang digunakan oleh pengguna lain.

## Saran

Beberapa saran yang dapat diberikan untuk pengembangan sistem selanjutnya:

1. **Aktuator pintu.** Untuk mengendalikan lebih banyak pintu, disarankan menggunakan modul relay yang diintegrasikan dengan *door lock* elektrik. Apabila tetap ingin menggunakan motor servo, disarankan menggunakan mikrokontroler lain dengan jumlah pin PWM yang lebih banyak agar dapat mengakomodasi lebih banyak box.
2. **Penyimpanan data.** Pencatatan hasil pembacaan RFID sebaiknya menggunakan basis data (database) tersendiri. Dengan adanya database, pendaftaran objek baru maupun penghapusan objek terdaftar dapat dilakukan langsung pada database tanpa perlu meng-*upload* ulang program ke mikrokontroler setiap kali terjadi pembaruan data pengguna. ✅
3. **Monitoring berbasis web.** Sebagai kelanjutan dari poin kedua, disarankan untuk merancang sistem monitoring berbasis web guna memantau aktivitas penggunaan box kunci, baik untuk memperbarui data RFID maupun untuk memantau riwayat hasil pembacaan sistem secara real-time. ✅

---
## Publikasi
- [Edu Elektrika Journal (Original Publikasi)](https://journal.unnes.ac.id/journals/eduel/article/view/13630)
- [Jurnal UMK](https://jurnal.umk.ac.id/index.php/simet/article/view/12291)
- [Research Gate](https://www.researchgate.net/publication/398851505_Prototype_Secure_Locker_System_Menggunakan_RFID_pada_Laboratorium_Universitas_Global_Jakarta)