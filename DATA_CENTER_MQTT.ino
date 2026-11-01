// ============================================================
// MCU1 - DATA CENTER (6 DHT11) + MQTT PUBLISHER
// Kirim alert ke MQTT jika suhu > 30°C ATAU kelembaban > 50%
// ============================================================

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ========== KONFIGURASI WIFI ==========
const char* ssid = " ";
const char* password = " ";

// ========== KONFIGURASI MQTT ==========
const char* mqtt_server = "10.33.109.55";
const int mqtt_port = 1883;
const char* mqtt_client_id = "MCU1_DC";

// ========== MQTT TOPICS ==========
const char* topic_alert = "bank/dc/alert";
const char* topic_temperature = "bank/dc/temperature";
const char* topic_humidity = "bank/dc/humidity";
const char* topic_heartbeat = "bank/dc/heartbeat";

const char* topic_suhu1 = "bank/dc/sensor1/suhu";
const char* topic_hum1  = "bank/dc/sensor1/hum";
const char* topic_suhu2 = "bank/dc/sensor2/suhu";
const char* topic_hum2  = "bank/dc/sensor2/hum";
const char* topic_suhu3 = "bank/dc/sensor3/suhu";
const char* topic_hum3  = "bank/dc/sensor3/hum";
const char* topic_suhu4 = "bank/dc/sensor4/suhu";
const char* topic_hum4  = "bank/dc/sensor4/hum";
const char* topic_suhu5 = "bank/dc/sensor5/suhu";
const char* topic_hum5  = "bank/dc/sensor5/hum";
const char* topic_suhu6 = "bank/dc/sensor6/suhu";
const char* topic_hum6  = "bank/dc/sensor6/hum";
// ========== PIN 6 DHT11 ==========
#define DHTPIN1 16
#define DHTPIN2 17
#define DHTPIN3 18
#define DHTPIN4 25
#define DHTPIN5 26
#define DHTPIN6 27
#define DHTTYPE DHT11

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);
DHT dht3(DHTPIN3, DHTTYPE);
DHT dht4(DHTPIN4, DHTTYPE);
DHT dht5(DHTPIN5, DHTTYPE);
DHT dht6(DHTPIN6, DHTTYPE);

const float SUHU_ALERT = 30.0;
const float HUMIDITY_ALERT = 50.0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 30000;

float bacaSuhu(DHT &dht) {
  float suhu = dht.readTemperature();
  return isnan(suhu) ? 0 : suhu;
}

float bacaHumidity(DHT &dht) {
  float hum = dht.readHumidity();
  return isnan(hum) ? 0 : hum;
}

bool cekAlert(float suhu[], float hum[]) {
  for (int i = 0; i < 6; i++) {
    if (suhu[i] > SUHU_ALERT || hum[i] > HUMIDITY_ALERT) return true;
  }
  return false;
}

float hitungRataRata(float arr[]) {
  float sum = 0;
  for (int i = 0; i < 6; i++) sum += arr[i];
  return sum / 6;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  dht1.begin(); dht2.begin(); dht3.begin();
  dht4.begin(); dht5.begin(); dht6.begin();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("\n✅ WiFi connected");
  
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    if (client.connect(mqtt_client_id)) Serial.println("✅ MQTT connected");
    else delay(5000);
  }
  
  Serial.println("MCU1 - Data Center Ready");
}

void loop() {
  if (!client.connected()) client.connect(mqtt_client_id);
  client.loop();
  
  float suhu[6] = {bacaSuhu(dht1), bacaSuhu(dht2), bacaSuhu(dht3),
                   bacaSuhu(dht4), bacaSuhu(dht5), bacaSuhu(dht6)};
  float hum[6] = {bacaHumidity(dht1), bacaHumidity(dht2), bacaHumidity(dht3),
                  bacaHumidity(dht4), bacaHumidity(dht5), bacaHumidity(dht6)};
  
  bool alert = cekAlert(suhu, hum);
  float avgTemp = hitungRataRata(suhu);
  float avgHum = hitungRataRata(hum);
  
  client.publish(topic_alert, alert ? "1" : "0");
  client.publish(topic_temperature, String(avgTemp).c_str());
  client.publish(topic_humidity, String(avgHum).c_str());
  
  client.publish(topic_suhu1, String(suhu[0]).c_str());
  client.publish(topic_hum1,  String(hum[0]).c_str());
  client.publish(topic_suhu2, String(suhu[1]).c_str());
  client.publish(topic_hum2,  String(hum[1]).c_str());
  client.publish(topic_suhu3, String(suhu[2]).c_str());
  client.publish(topic_hum3,  String(hum[2]).c_str());
  client.publish(topic_suhu4, String(suhu[3]).c_str());
  client.publish(topic_hum4,  String(hum[3]).c_str());
  client.publish(topic_suhu5, String(suhu[4]).c_str());
  client.publish(topic_hum5,  String(hum[4]).c_str());
  client.publish(topic_suhu6, String(suhu[5]).c_str());
  client.publish(topic_hum6,  String(hum[5]).c_str());
  
  if (millis() - lastHeartbeat > heartbeatInterval) {
    client.publish(topic_heartbeat, "1");
    lastHeartbeat = millis();
  }
  
  Serial.print("DC Alert: ");
  Serial.println(alert ? "YES" : "NO");
  
  delay(3000);
}
