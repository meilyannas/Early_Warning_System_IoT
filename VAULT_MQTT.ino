// ============================================================
// MCU2 - VAULT (RFID + Getar + Pintu + Servo) + MQTT
// GETARAN: ALERT 5 DETIK (tidak langsung berhenti)
// ============================================================

#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// ========== PIN DEFINITION ==========
#define PIN_VIBRATION 34
#define PIN_DOOR 35
#define SS_PIN 4
#define RST_PIN 22
#define SCK_PIN 18
#define MOSI_PIN 23
#define MISO_PIN 19
#define SERVO_PIN 13

// ========== SERVO ==========
Servo myServo;
const int SERVO_TERTUTUP = 0;
const int SERVO_TERBUKA = 90;

// ========== WIFI & MQTT ==========
const char* ssid = " ";
const char* password = " ";
const char* mqtt_server = "10.33.109.55";
const int mqtt_port = 1883;
const char* mqtt_client = "MCU2_Vault";

// ========== MQTT TOPICS ==========
const char* topic_vibration = "bank/vault/vibration";
const char* topic_door = "bank/vault/door";
const char* topic_rfid = "bank/vault/access";
const char* topic_alert = "bank/vault/alert";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// UID kartu valid
byte validUID[][4] = {{0xF6, 0x10, 0xEC, 0x05}};
int jumlahKartuValid = 1;

// State variables
bool aksesAman = false;
unsigned long lastAccessTime = 0;
const unsigned long accessDuration = 5000;
bool illegalCardScanned = false;
unsigned long lastIllegalWarningTime = 0;
const unsigned long illegalWarningCooldown = 3000;
unsigned long lastScanTime = 0;
const unsigned long scanCooldown = 2000;

// ========== GETARAN DENGAN TIMER 5 DETIK ==========
bool vibrationAlertActive = false;
unsigned long lastVibrationTriggerTime = 0;
const unsigned long vibrationAlertDuration = 5000;

// ========== SENSOR PINTU DENGAN DEBOUNCE ==========
bool lastDoorState = false;
bool lastPublishedDoorState = false;  // Status terakhir yang dipublish
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;  // 50ms debounce

// Anti-spam illegal warning
bool illegalWarningPrinted = false;

void bukaPintu() {
  Serial.println("🔓 Servo: Membuka pintu...");
  myServo.write(SERVO_TERBUKA);
}

void tutupPintu() {
  Serial.println("🔒 Servo: Menutup pintu...");
  myServo.write(SERVO_TERTUTUP);
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("✅ WiFi connected");
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(mqtt_client)) Serial.println("✅ MQTT connected");
    else delay(5000);
  }
}

void publishMQTT(const char* topic, String payload) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic, payload.c_str());
  }
}

bool isCardValid() {
  for (int i = 0; i < jumlahKartuValid; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (mfrc522.uid.uidByte[j] != validUID[i][j]) match = false;
    }
    if (match) return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_TERTUTUP);
  
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();
  
  pinMode(PIN_VIBRATION, INPUT);
  pinMode(PIN_DOOR, INPUT_PULLUP);
  
  // Baca status awal pintu
  lastDoorState = (digitalRead(PIN_DOOR) == LOW);
  lastPublishedDoorState = lastDoorState;
  
  connectWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  connectMQTT();
  
  Serial.println("======================================");
  Serial.println("  MCU2 - Vault Ready");
  Serial.println("======================================");
  Serial.println("Getaran: Alert 5 detik");
  Serial.println("======================================");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();
  
  unsigned long now = millis();
  
  // ========== RFID ==========
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (now - lastScanTime >= scanCooldown) {
      lastScanTime = now;
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
      Serial.print("UID: "); Serial.println(uid);
      
      if (isCardValid()) {
        Serial.println("✅ AKSES DITERIMA!");
        aksesAman = true;
        lastAccessTime = now;
        illegalCardScanned = false;
        bukaPintu();
        publishMQTT(topic_rfid, "granted");
      } else {
        Serial.println("❌ AKSES DITOLAK!");
        illegalCardScanned = true;
        lastIllegalWarningTime = now;
        publishMQTT(topic_rfid, "denied");
        publishMQTT(topic_alert, "akses_illegal");
      }
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
  
  // ========== Update akses aman ==========
  if (aksesAman && (now - lastAccessTime > accessDuration)) {
    aksesAman = false;
    tutupPintu();
    publishMQTT(topic_rfid, "akses_berakhir");
  }
  
  // ========== SENSOR GETAR (ALERT 5 DETIK) ==========
  bool currentVibration = (digitalRead(PIN_VIBRATION) == HIGH);
  
  if (currentVibration) {
    lastVibrationTriggerTime = now;
    if (!vibrationAlertActive) {
      vibrationAlertActive = true;
      Serial.println("🚨 GETARAN TERDETEKSI! (Alert 5 detik)");
      publishMQTT(topic_vibration, "1");
    }
  }
  
  bool alertActive = (now - lastVibrationTriggerTime < vibrationAlertDuration);
  if (!alertActive && vibrationAlertActive) {
    vibrationAlertActive = false;
    Serial.println("✅ Alert getaran berakhir (5 detik selesai)");
    publishMQTT(topic_vibration, "0");
  }
  
  // ========== SENSOR PINTU DENGAN DEBOUNCE (TIDAK SPAM) ==========
  bool rawDoorState = (digitalRead(PIN_DOOR) == LOW);  // LOW = terbuka
  
  // Debounce: hanya berubah jika status stabil selama debounceDelay
  if (rawDoorState != lastDoorState) {
    lastDebounceTime = now;
    lastDoorState = rawDoorState;
  }
  
  // Jika status stabil dan berbeda dengan yang sudah dipublish
  if ((now - lastDebounceTime) > debounceDelay) {
    if (rawDoorState != lastPublishedDoorState) {
      lastPublishedDoorState = rawDoorState;
      
      if (rawDoorState) {
        // Pintu terbuka
        if (aksesAman) {
          Serial.println("🔓 Pintu Terbuka - Akses Sah");
          publishMQTT(topic_door, "open_sah");
        } else {
          Serial.println("🚪 WARNING! Pintu Terbuka Tanpa Otorisasi!");
          publishMQTT(topic_door, "open_tanpa_akses");
          publishMQTT(topic_alert, "pintu_tanpa_otorisasi");
        }
      } else {
        // Pintu tertutup
        Serial.println("🔒 Pintu Tertutup");
        publishMQTT(topic_door, "closed");
      }
    }
  }
  
  // ========== Warning Illegal (Anti-Spam) ==========
  if (illegalCardScanned && rawDoorState && !aksesAman) {
    if (!illegalWarningPrinted) {
      Serial.println("🔴 PERINGATAN! Akses Illegal - Pintu Seharusnya Tertutup!");
      publishMQTT(topic_alert, "akses_illegal_pintu_terbuka");
      illegalWarningPrinted = true;
    }
  } else {
    illegalWarningPrinted = false;
  }
  
  delay(10);
}
