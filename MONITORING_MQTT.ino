// ============================================================
// MCU3 - MONITORING (2 BUZZER + LED PERUBAHAN)
// Pin: 
//   G4  = LED Merah Getaran Vault (ALERT GETARAN)
//   G16 = LED Merah DC Alert
//   G17 = Buzzer DC
//   G21 = LED Hijau Vault (Akses Sah)
//   G22 = LED Merah Vault (Pintu Tanpa Akses / Alert Lain)
//   G23 = Buzzer Vault
// ============================================================

#include <WiFi.h>
#include <PubSubClient.h>

// ========== PIN DEFINISI ==========
#define PIN_LED_MERAH_GETARAN  21   
#define PIN_LED_MERAH_DC       16  
#define PIN_BUZZER_DC          17  
#define PIN_LED_HIJAU_VAULT    32  
#define PIN_LED_MERAH_VAULT    33  
#define PIN_BUZZER_VAULT       25  

// ========== KONFIGURASI ==========
const char* ssid = " ";
const char* password = " ";
const char* mqtt_server = "10.33.109.55";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// ========== STATUS ==========
bool alertDC = false;
bool alertVibration = false;
bool pintuTerbukaTanpaAkses = false;
bool aksesSah = false;
unsigned long aksesSahStartTime = 0;
const unsigned long aksesDurasi = 5000;

unsigned long aksesDitolakStartTime = 0;
bool aksesDitolak = false;
const unsigned long aksesDitolakDurasi = 3000;

unsigned long alertDCStartTime = 0;
unsigned long alertVibrationStartTime = 0;
bool alertDCActive = false;
bool alertVibrationActive = false;
const unsigned long alertDuration = 5000;

unsigned long lastBlinkTimeDC = 0;
unsigned long lastBlinkTimeVault = 0;
bool blinkStateDC = false;
bool blinkStateVault = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  pinMode(PIN_LED_MERAH_GETARAN, OUTPUT);
  pinMode(PIN_LED_MERAH_DC, OUTPUT);
  pinMode(PIN_BUZZER_DC, OUTPUT);
  pinMode(PIN_LED_HIJAU_VAULT, OUTPUT);
  pinMode(PIN_LED_MERAH_VAULT, OUTPUT);
  pinMode(PIN_BUZZER_VAULT, OUTPUT);
  
  // Matikan semua
  digitalWrite(PIN_LED_MERAH_GETARAN, LOW);
  digitalWrite(PIN_LED_MERAH_DC, LOW);
  digitalWrite(PIN_BUZZER_DC, LOW);
  digitalWrite(PIN_LED_HIJAU_VAULT, LOW);
  digitalWrite(PIN_LED_MERAH_VAULT, LOW);
  digitalWrite(PIN_BUZZER_VAULT, LOW);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("✅ WiFi connected");
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  connectMQTT();
  
  Serial.println("MCU3 - Monitoring Ready");
  Serial.println("G4 = LED Merah Getaran Vault");
  Serial.println("G16 = LED Merah DC Alert");
  Serial.println("G21 = LED Hijau Vault (Akses Sah)");
  Serial.println("G22 = LED Merah Vault (Pintu Tanpa Akses)");
}

void connectMQTT() {
  while (!client.connected()) {
    if (client.connect("MCU3_Monitor")) {
      client.subscribe("bank/dc/alert");
      client.subscribe("bank/vault/alert");
      client.subscribe("bank/vault/vibration");
      client.subscribe("bank/vault/access");
      client.subscribe("bank/vault/door");
      Serial.println("✅ MQTT connected & subscribed");
    } else delay(5000);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];
  
  Serial.print("📩 Topic: ");
  Serial.print(topic);
  Serial.print(" | Msg: ");
  Serial.println(message);
  
  String topicStr = String(topic);
  
  // ========== DC ALERT ==========
  if (topicStr == "bank/dc/alert") {
    if (message == "1") {
      alertDC = true;
      alertDCActive = true;
      alertDCStartTime = millis();
      Serial.println("🔴 DC ALERT!");
    } else {
      alertDC = false;
    }
  }
  
  // ========== GETARAN (LED di G4) ==========
  if (topicStr == "bank/vault/vibration" && message == "1") {
    alertVibration = true;
    alertVibrationActive = true;
    alertVibrationStartTime = millis();
    Serial.println("🚨 GETARAN! (LED Merah G4 menyala)");
  }
  
  // ========== AKSES RFID ==========
  if (topicStr == "bank/vault/access") {
    if (message == "granted") {
      aksesSah = true;
      aksesSahStartTime = millis();
      aksesDitolak = false;
      Serial.println("✅ AKSES SAH");
      digitalWrite(PIN_LED_HIJAU_VAULT, HIGH);
      delay(200);
      digitalWrite(PIN_LED_HIJAU_VAULT, LOW);
      digitalWrite(PIN_BUZZER_VAULT, HIGH);
      delay(200);
      digitalWrite(PIN_BUZZER_VAULT, LOW);
    } 
    else if (message == "denied") {
      aksesDitolak = true;
      aksesDitolakStartTime = millis();
      aksesSah = false;
      Serial.println("❌ AKSES DITOLAK");
      
      for (int i = 0; i < 2; i++) {
        digitalWrite(PIN_LED_MERAH_VAULT, HIGH);
        digitalWrite(PIN_BUZZER_VAULT, HIGH);
        delay(200);
        digitalWrite(PIN_LED_MERAH_VAULT, LOW);
        digitalWrite(PIN_BUZZER_VAULT, LOW);
        delay(200);
      }
    }
  }
  
  // ========== PINTU ==========
  if (topicStr == "bank/vault/door") {
    if (message == "open_tanpa_akses") {
      pintuTerbukaTanpaAkses = true;
      Serial.println("🚪 PINTU TERBUKA TANPA AKSES!");
    } else if (message == "open_sah") {
      Serial.println("🔓 PINTU TERBUKA (Akses sah)");
      digitalWrite(PIN_LED_HIJAU_VAULT, HIGH);
      digitalWrite(PIN_BUZZER_VAULT, HIGH);
      delay(300);
      digitalWrite(PIN_BUZZER_VAULT, LOW);
    } else if (message == "closed") {
      pintuTerbukaTanpaAkses = false;
      digitalWrite(PIN_LED_HIJAU_VAULT, LOW);
      digitalWrite(PIN_LED_MERAH_VAULT, LOW);
      digitalWrite(PIN_BUZZER_VAULT, LOW);
    }
  }
}

void loop() {
  if (!client.connected()) connectMQTT();
  client.loop();
  
  unsigned long now = millis();
  
  // ========== RESET AKSES DITOLAK ==========
  if (aksesDitolak && (now - aksesDitolakStartTime > aksesDitolakDurasi)) {
    aksesDitolak = false;
    digitalWrite(PIN_LED_MERAH_VAULT, LOW);
    Serial.println("✅ Status akses ditolak berakhir");
  }
  
  // ========== UPDATE AKSES SAH ==========
  if (aksesSah && (now - aksesSahStartTime > aksesDurasi)) {
    aksesSah = false;
    digitalWrite(PIN_LED_HIJAU_VAULT, LOW);
    Serial.println("⏰ Masa akses sah berakhir");
  }
  
  // ========== ALERT DC (LED Merah G16 + Buzzer G17) ==========
  if (alertDCActive) {
    if (now - alertDCStartTime < alertDuration) {
      if (now - lastBlinkTimeDC > 100) {
        blinkStateDC = !blinkStateDC;
        digitalWrite(PIN_LED_MERAH_DC, blinkStateDC);
        digitalWrite(PIN_BUZZER_DC, blinkStateDC);
        lastBlinkTimeDC = now;
      }
    } else {
      alertDCActive = false;
      digitalWrite(PIN_LED_MERAH_DC, LOW);
      digitalWrite(PIN_BUZZER_DC, LOW);
      Serial.println("✅ DC kembali normal");
    }
  }
  
  // ========== ALERT GETARAN (LED Merah G4 + Buzzer G23) ==========
  if (alertVibrationActive) {
    if (now - alertVibrationStartTime < alertDuration) {
      if (now - lastBlinkTimeVault > 100) {
        blinkStateVault = !blinkStateVault;
        digitalWrite(PIN_LED_MERAH_GETARAN, blinkStateVault);  // LED Getaran di G4
        digitalWrite(PIN_BUZZER_VAULT, blinkStateVault);
        lastBlinkTimeVault = now;
      }
    } else {
      alertVibrationActive = false;
      digitalWrite(PIN_LED_MERAH_GETARAN, LOW);
      digitalWrite(PIN_BUZZER_VAULT, LOW);
      Serial.println("✅ Getaran selesai");
    }
  }
  
  // ========== PINTU TERBUKA TANPA AKSES (LED Merah G22 + Buzzer G23) ==========
  if (pintuTerbukaTanpaAkses && !aksesSah) {
    if (now - lastBlinkTimeVault > 500) {
      blinkStateVault = !blinkStateVault;
      digitalWrite(PIN_LED_MERAH_VAULT, blinkStateVault);
      digitalWrite(PIN_BUZZER_VAULT, blinkStateVault);
      lastBlinkTimeVault = now;
    }
  }
  
  // ========== AKSES SAH + PINTU TERBUKA ==========
  if (aksesSah && pintuTerbukaTanpaAkses) {
    digitalWrite(PIN_LED_HIJAU_VAULT, HIGH);
  }
  
  // ========== PASTIKAN LED MATI JIKA TIDAK ADA STATUS ==========
  if (!aksesDitolak && !alertVibrationActive && !pintuTerbukaTanpaAkses && !aksesSah) {
    digitalWrite(PIN_LED_MERAH_GETARAN, LOW);
    digitalWrite(PIN_LED_MERAH_VAULT, LOW);
    digitalWrite(PIN_LED_HIJAU_VAULT, LOW);
    digitalWrite(PIN_BUZZER_VAULT, LOW);
  }
  
  delay(10);
}
