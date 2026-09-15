// ino.ino  (Arduino Uno R3 - Controller & Actuator)

#include <Keypad.h>
#include <Servo.h>

const byte ROW_NUM    = 4; // jumlah baris keypad
const byte COLUMN_NUM = 4; // jumlah kolom keypad
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pin_rows[ROW_NUM] = {A0, A1, A2, A3};    // pin baris keypad
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2};   // pin kolom keypad

String fixUid = "";

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

Servo servoBox1;
Servo servoBox2;
Servo servoBox3;
Servo servoBox4;

const int greenLED = 6; // pin untuk LED hijau
const int redLED = 7;   // pin untuk LED merah

const int servoPin1 = 8;   // pin servo untuk Box 1
const int servoPin2 = 9;   // pin servo untuk Box 2
const int servoPin3 = 10;  // pin servo untuk Box 3
const int servoPin4 = 11;  // pin servo untuk Box 4

const int servoOpenAngle = 75;  // sudut servo saat membuka pintu
const int servoCloseAngle = 0;  // sudut servo saat menutup pintu
const int servoDelay = 10000;   // jeda waktu (ms) sebelum menutup pintu kembali

// FIX: validCardsCount sebelumnya 5
const int validCardsCount = 3; // jumlah kartu yang benar-benar didaftarkan
char validCards[validCardsCount][16] = {
  "DE 82 5B 6E", // Kartu 1
  "BE E0 54 6E", // Kartu 2
  "3E 5A 5B 6E"  // Kartu 3
};

char userName[validCardsCount][20] = {
  "Luffy",
  "Kizaru",
  "Akainu"
};

const int totalBox = 4;
String usedCards[totalBox] = {
  "available",
  "available",
  "available",
  "available"
};

boolean accessGranted = false; // status akses setelah scanning kartu
int selectedBox = -1;          // nomor loker yang dipilih

void setup() {
  Serial.begin(9600); // jalur komunikasi ke NodeMCU ESP8266 (RX0/TX1)

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  // FIX: attach dilakukan sekali di setup untuk masing-masing servo,
  // bukan berulang kali di dalam openLocker()/closeLocker().
  servoBox1.attach(servoPin1);
  servoBox2.attach(servoPin2);
  servoBox3.attach(servoPin3);
  servoBox4.attach(servoPin4);

  servoBox1.write(servoCloseAngle);
  servoBox2.write(servoCloseAngle);
  servoBox3.write(servoCloseAngle);
  servoBox4.write(servoCloseAngle);
}

void loop() {
  // Periksa kartu RFID melalui input serial (dikirim oleh NodeMCU ESP8266)
  if (!accessGranted && Serial.available()) {
    String uid = Serial.readStringUntil('\n');
    uid.trim();
    bool validCard = false;

    for (int i = 0; i < validCardsCount; i++) {
      if (uid.equals(validCards[i])) {
        validCard = true;
        break;
      }
    }

    if (validCard) {
      fixUid = uid;
      accessGranted = true;
      Serial.print("Access Granted - UID : ");
      Serial.print(uid);
      Serial.print("\tusername : ");
      Serial.println(getName(uid));
      digitalWrite(greenLED, HIGH);
      delay(1000);
      digitalWrite(greenLED, LOW);
    } else {
      Serial.println("Access Denied - Invalid Card");
      digitalWrite(redLED, HIGH);
      delay(1000);
      digitalWrite(redLED, LOW);
    }
  }

  // Jika akses telah diberikan, tunggu input dari keypad
  if (accessGranted) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      selectedBox = key - '0' - 1; // ubah karakter ke indeks box (0-3)
      if (selectedBox >= 0 && selectedBox < totalBox) {
        if (usedCards[selectedBox] == "available" || fixUid.equals(usedCards[selectedBox])) {
          openLocker(selectedBox);
          if (fixUid.equals(usedCards[selectedBox])) {
            usedCards[selectedBox] = "available";
            Serial.print("Loker " + String(selectedBox + 1));
            Serial.print(" Terbuka, Kunci telah dikembalikan oleh kartu ");
            Serial.println(getName(fixUid));
          } else {
            usedCards[selectedBox] = fixUid;
            Serial.print("Loker " + String(selectedBox + 1));
            Serial.print(" Terbuka, Kunci telah diambil oleh kartu ");
            Serial.println(getName(fixUid));
          }
          digitalWrite(greenLED, HIGH);
          delay(servoDelay); // tunggu sebelum menutup kembali
          digitalWrite(greenLED, LOW);
        } else {
          Serial.print("Loker " + String(selectedBox + 1));
          Serial.print(" sedang digunakan oleh ");
          Serial.println(getName(usedCards[selectedBox]));
        }

        closeLocker(selectedBox);

        selectedBox = -1;
        accessGranted = false;
      } else {
        accessGranted = false;
        Serial.println("Box Tidak Valid");
        digitalWrite(redLED, HIGH);
        delay(2000);
        digitalWrite(redLED, LOW);
      }
    }
  }
}

void openLocker(int box) {
  switch (box) {
    case 0: servoBox1.write(servoOpenAngle); break;
    case 1: servoBox2.write(servoOpenAngle); break;
    case 2: servoBox3.write(servoOpenAngle); break;
    case 3: servoBox4.write(servoOpenAngle); break;
    default: break;
  }
}

void closeLocker(int box) {
  switch (box) {
    case 0: servoBox1.write(servoCloseAngle); break;
    case 1: servoBox2.write(servoCloseAngle); break;
    case 2: servoBox3.write(servoCloseAngle); break;
    case 3: servoBox4.write(servoCloseAngle); break;
    default: break;
  }
}

String getName(String uid) {
  for (int i = 0; i < validCardsCount; i++) {
    if (uid.equals(validCards[i])) {
      return userName[i];
    }
  }
  return "Unknown";
}
