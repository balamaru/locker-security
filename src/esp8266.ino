// esp8266.ino  (NodeMCU ESP8266 - RFID Reader, WiFi Bridge & Telegram Notifier)

#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define SS_PIN D4
#define RST_PIN D3
MFRC522 mfrc522(SS_PIN, RST_PIN);

SoftwareSerial arduinoSerial(D1, D2); // RX, TX ke Arduino Uno

const char* ssid = "wizard";        // Nama SSID WiFi yang dihubungkan
const char* password = "12345678";  // Kata sandi WiFi yang dihubungkan

const char* botToken = "432156789:AAFriuvyaveyvYVYVSVubdjbcyqbwuqdu"; // Token bot Telegram
#define CHAT_ID "00000000"                                           // ID chat Telegram penerima

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
  // Periksa kartu RFID
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getUID();
    Serial.println("UID: " + uid);
    arduinoSerial.println(uid); // Kirim UID ke Arduino Uno
    mfrc522.PICC_HaltA();
  }

  String response = readArduinoResponse();
  if (response.length() > 5) {
    sendTelegramMessage(response); // Kirim respon dari Arduino ke Telegram
  }
}

String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(mfrc522.uid.uidByte[i], HEX);
    uid.toUpperCase();
    uid += " ";
  }
  uid.trim();
  return uid;
}

String readArduinoResponse() {
  String response = "";
  while (arduinoSerial.available()) {
    response += arduinoSerial.readStringUntil('\n');
  }
  response.trim();
  return response;
}

void sendTelegramMessage(String message) {
  bot.sendMessage(CHAT_ID, message, ""); // Mengirim pesan
  Serial.println(message);               // Print pesan di serial monitor
}

void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  secured_client.setTrustAnchors(&cert); // Root certificate untuk api.telegram.org

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "pool.ntp.org"); // ambil UTC time via NTP (wajib untuk TLS)
  time_t now = time(nullptr);

  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
  bot.sendMessage(CHAT_ID, "Bot has started.", ""); // Notifikasi bot aktif
}
