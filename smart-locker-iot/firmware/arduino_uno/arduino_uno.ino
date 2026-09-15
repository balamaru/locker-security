// arduino_uno.ino  (Arduino Uno R3 - Controller & Actuator)
//
// PERUBAHAN ARSITEKTUR (implementasi saran no. 2 & 3):
// Sebelumnya daftar kartu valid (validCards[]) di-hardcode di sini. Sekarang
// verifikasi dilakukan oleh server (lihat /server) melalui NodeMCU. Arduino
// tinggal menerima HASIL verifikasi dari NodeMCU lewat Serial, lalu
// mengendalikan keypad + servo seperti biasa, dan melaporkan balik setiap
// event box ke NodeMCU (yang akan meneruskannya ke server + Telegram).
//
// PROTOKOL SERIAL (satu baris, dipisah koma, diakhiri newline):
//   NodeMCU -> Arduino :  "V,1,<uid>,<name>"   (kartu valid)
//                          "V,0"                (kartu tidak dikenali)
//   Arduino -> NodeMCU :  "L,<box>,<action>,<uid>,<name>"
//       action = TAKE | RETURN | BUSY | INVALID_BOX

#include <Keypad.h>
#include <Servo.h>

const byte ROW_NUM    = 4;
const byte COLUMN_NUM = 4;
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pin_rows[ROW_NUM] = {A0, A1, A2, A3};
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

Servo servoBox1;
Servo servoBox2;
Servo servoBox3;
Servo servoBox4;

const int greenLED = 6;
const int redLED = 7;

const int servoPin1 = 8;
const int servoPin2 = 9;
const int servoPin3 = 10;
const int servoPin4 = 11;

const int servoOpenAngle = 75;
const int servoCloseAngle = 0;
const int servoDelay = 10000;

const int totalBox = 4;
String usedCards[totalBox] = {"available", "available", "available", "available"};

boolean accessGranted = false;
String fixUid = "";
String fixName = "";
int selectedBox = -1;

void setup() {
  Serial.begin(9600); // jalur komunikasi ke NodeMCU ESP8266

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

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
  // Terima hasil verifikasi dari NodeMCU
  if (!accessGranted && Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    handleVerificationLine(line);
  }

  if (accessGranted) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      handleKeypadSelection(key);
    }
  }
}

void handleVerificationLine(String line) {
  // Format: "V,1,<uid>,<name>" atau "V,0"
  if (!line.startsWith("V,")) return;

  int firstComma = line.indexOf(',', 2);
  String validFlag = (firstComma == -1) ? line.substring(2) : line.substring(2, firstComma);

  if (validFlag == "1") {
    int secondComma = line.indexOf(',', firstComma + 1);
    fixUid = line.substring(firstComma + 1, secondComma);
    fixName = line.substring(secondComma + 1);

    accessGranted = true;
    digitalWrite(greenLED, HIGH);
    delay(1000);
    digitalWrite(greenLED, LOW);
  } else {
    digitalWrite(redLED, HIGH);
    delay(1000);
    digitalWrite(redLED, LOW);
  }
}

void handleKeypadSelection(char key) {
  selectedBox = key - '0' - 1; // ubah karakter ke indeks box (0-3)

  if (selectedBox < 0 || selectedBox >= totalBox) {
    accessGranted = false;
    reportEvent(0, "INVALID_BOX", fixUid, fixName);
    digitalWrite(redLED, HIGH);
    delay(2000);
    digitalWrite(redLED, LOW);
    return;
  }

  bool isReturn = fixUid.equals(usedCards[selectedBox]);
  bool isFree = usedCards[selectedBox] == "available";

  if (isFree || isReturn) {
    openLocker(selectedBox);

    if (isReturn) {
      usedCards[selectedBox] = "available";
      reportEvent(selectedBox + 1, "RETURN", fixUid, fixName);
    } else {
      usedCards[selectedBox] = fixUid;
      reportEvent(selectedBox + 1, "TAKE", fixUid, fixName);
    }

    digitalWrite(greenLED, HIGH);
    delay(servoDelay);
    digitalWrite(greenLED, LOW);
  } else {
    // Box sedang dipakai kartu lain -> laporkan siapa pemegangnya saat ini
    reportEvent(selectedBox + 1, "BUSY", fixUid, fixName);
  }

  closeLocker(selectedBox);
  selectedBox = -1;
  accessGranted = false;
}

void reportEvent(int box, String action, String uid, String name) {
  // Dikirim ke NodeMCU, yang akan meneruskan ke server (DB) + Telegram
  Serial.print("L,");
  Serial.print(box);
  Serial.print(",");
  Serial.print(action);
  Serial.print(",");
  Serial.print(uid);
  Serial.print(",");
  Serial.println(name);
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
