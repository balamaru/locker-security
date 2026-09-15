// esp8266.ino  (NodeMCU ESP8266 - RFID Reader, WiFi/Server Bridge & Telegram Notifier)
//
// PERUBAHAN ARSITEKTUR (implementasi saran no. 2 & 3):
//  - UID yang terbaca TIDAK lagi dicocokkan secara lokal; NodeMCU memanggil
//    endpoint server "GET /api/cards/verify?uid=..." (database) untuk
//    memvalidasi, lalu meneruskan hasilnya ke Arduino via Serial.
//  - Setiap event dari Arduino (buka/tutup box) di-POST ke "POST /api/logs"
//    supaya tercatat di database dan tampil di web dashboard, SEKALIGUS
//    tetap dikirim ke Telegram seperti sebelumnya.
//
// LIBRARY TAMBAHAN yang perlu diinstall lewat Arduino Library Manager:
//  - ArduinoJson (by Benoit Blanchon)
//
// PROTOKOL SERIAL (lihat juga arduino_uno.ino):
//   NodeMCU -> Arduino :  "V,1,<uid>,<name>"  |  "V,0"
//   Arduino -> NodeMCU :  "L,<box>,<action>,<uid>,<name>"

#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#define SS_PIN D4
#define RST_PIN D3
MFRC522 mfrc522(SS_PIN, RST_PIN);

SoftwareSerial arduinoSerial(D1, D2); // RX, TX ke Arduino Uno

const char* ssid = "wizard";
const char* password = "12345678";

const char* botToken = "432156789:AAFriuvyaveyvYVYVSVubdjbcyqbwuqdu";
#define CHAT_ID "00000000"

// Alamat server database + web monitoring (lihat folder /server).
// Ganti sesuai IP komputer/Raspberry Pi tempat server dijalankan di jaringan lokal.
const char* SERVER_HOST = "192.168.1.100";
const int   SERVER_PORT = 3000;

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(botToken, secured_client);

void setup() {
  Serial.begin(9600);
  arduinoSerial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  connectWiFi();
  Serial.println("Ready to scan RFID cards!");
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getUID();
    Serial.println("UID terbaca: " + uid);
    verifyCardWithServer(uid);
    mfrc522.PICC_HaltA();
  }

  String response = readArduinoResponse();
  if (response.length() > 0) {
    handleArduinoEvent(response);
  }
}

String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    uid.toUpperCase();
    uid += " ";
  }
  uid.trim();
  return uid;
}

String urlEncodeUid(String uid) {
  String encoded = uid;
  encoded.replace(" ", "%20");
  return encoded;
}

// Panggil server untuk verifikasi kartu, lalu kirim hasilnya ke Arduino
void verifyCardWithServer(String uid) {
  if (WiFi.status() != WL_CONNECTED) {
    arduinoSerial.println("V,0"); // gagal konek -> anggap tidak valid
    return;
  }

  WiFiClient client;
  HTTPClient http;
  String url = String("http://") + SERVER_HOST + ":" + SERVER_PORT +
               "/api/cards/verify?uid=" + urlEncodeUid(uid);

  http.begin(client, url);
  int code = http.GET();

  bool valid = false;
  String name = "";

  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      valid = doc["valid"] | false;
      if (valid) name = doc["name"].as<String>();
    }
  } else {
    Serial.println("Verifikasi ke server gagal, kode: " + String(code));
  }
  http.end();

  if (valid) {
    arduinoSerial.println("V,1," + uid + "," + name);
    postLog(uid, name, 0, "GRANTED", "Akses diterima");
    sendTelegramMessage("Access Granted - UID: " + uid + " (" + name + ")");
  } else {
    arduinoSerial.println("V,0");
    postLog(uid, "", 0, "DENIED", "Kartu tidak dikenali");
    sendTelegramMessage("Access Denied - Invalid Card (UID: " + uid + ")");
  }
}

String readArduinoResponse() {
  String response = "";
  while (arduinoSerial.available()) {
    response += arduinoSerial.readStringUntil('\n');
  }
  response.trim();
  return response;
}

// Format dari Arduino: "L,<box>,<action>,<uid>,<name>"
void handleArduinoEvent(String line) {
  if (!line.startsWith("L,")) return;

  int p1 = line.indexOf(',', 2);
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);

  int box = line.substring(2, p1).toInt();
  String action = line.substring(p1 + 1, p2);
  String uid = line.substring(p2 + 1, p3);
  String name = line.substring(p3 + 1);

  String message = buildMessageForAction(box, action, name);

  postLog(uid, name, box, action, message);
  sendTelegramMessage(message);
}

String buildMessageForAction(int box, String action, String name) {
  if (action == "TAKE") {
    return "Loker " + String(box) + " terbuka, kunci diambil oleh " + name;
  } else if (action == "RETURN") {
    return "Loker " + String(box) + " terbuka, kunci dikembalikan oleh " + name;
  } else if (action == "BUSY") {
    return "Loker " + String(box) + " sedang digunakan, percobaan akses oleh " + name;
  } else if (action == "INVALID_BOX") {
    return "Pemilihan box tidak valid oleh " + name;
  }
  return "Event: " + action;
}

// Kirim log ke server (tersimpan di database, tampil di web dashboard)
void postLog(String uid, String name, int box, String action, String message) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;
  String url = String("http://") + SERVER_HOST + ":" + SERVER_PORT + "/api/logs";

  StaticJsonDocument<256> doc;
  doc["uid"] = uid;
  doc["name"] = name;
  doc["box"] = box;
  doc["action"] = action;
  doc["message"] = message;

  String body;
  serializeJson(doc, body);

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  if (code < 0) {
    Serial.println("Gagal mengirim log ke server: " + String(code));
  }
  http.end();
}

void sendTelegramMessage(String message) {
  bot.sendMessage(CHAT_ID, message, "");
  Serial.println(message);
}

void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  secured_client.setTrustAnchors(&cert);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
  bot.sendMessage(CHAT_ID, "Bot has started.", "");
}
