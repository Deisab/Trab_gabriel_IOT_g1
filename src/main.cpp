#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// ======== CONFIG Wi-Fi ========
const char* ssid = "AMF-CORP";
const char* password = "@MF$4515";

// ======== CONFIG MQTT ========
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* pub_topic = "grupoGabriel/dados";  
const char* sub_topic = "grupoGabriel/cmd";    

WiFiClient espClient;
PubSubClient client(espClient);

// ======== CONFIG CLOUD ========
const char* cloud_url = "https://kv-teste.deisab12.workers.dev/insert";
const char* device_id = "esp32_01";  // id única do ESP32

// ======== PINOS ========
#define SENSOR_CHUVA_DIGITAL 25
#define SENSOR_UMIDADE_DIGITAL 35
#define LED_SECO   27
#define LED_UMIDO  14
#define LED_CHUVOSO_UMIDO 26

// ======== Variáveis ========
unsigned long lastSend = 0;
bool chuva = false;
bool umido = false;

// ======== Conectar Wi-Fi ========
void setup_wifi() {
  Serial.print("Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ======== Função Resetar LEDs ========
void resetarLeds() {
  // Desligar todos
  digitalWrite(LED_SECO, LOW);
  digitalWrite(LED_UMIDO, LOW);
  digitalWrite(LED_CHUVOSO_UMIDO, LOW);

  // Piscar 3 vezes
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_SECO, HIGH);
    digitalWrite(LED_UMIDO, HIGH);
    digitalWrite(LED_CHUVOSO_UMIDO, HIGH);
    delay(500);
    digitalWrite(LED_SECO, LOW);
    digitalWrite(LED_UMIDO, LOW);
    digitalWrite(LED_CHUVOSO_UMIDO, LOW);
    delay(500);
  }

  // Publicar confirmação
  client.publish(pub_topic, "{\"status\":\"RESET_OK\"}");
}

// ======== Callback MQTT ========
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (msg == "LED_SECO_ON") digitalWrite(LED_SECO, HIGH);
  if (msg == "LED_SECO_OFF") digitalWrite(LED_SECO, LOW);
  if (msg == "RESET_LEDS") resetarLeds();
}

// ======== Reconectar MQTT ========
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(sub_topic);
      Serial.println(" conectado!");
    } else {
      Serial.print(" falhou, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_CHUVA_DIGITAL, INPUT);
  pinMode(SENSOR_UMIDADE_DIGITAL, INPUT);

  pinMode(LED_SECO, OUTPUT);
  pinMode(LED_UMIDO, OUTPUT);
  pinMode(LED_CHUVOSO_UMIDO, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // ======== LER SENSORES ========
  int chuvaDigital = digitalRead(SENSOR_CHUVA_DIGITAL);
  int umidadeDigital = digitalRead(SENSOR_UMIDADE_DIGITAL);

  chuva = (chuvaDigital == LOW);
  umido = (umidadeDigital == LOW);

  // Lógica LEDs
  if (!chuva && !umido) { // Seco
    digitalWrite(LED_SECO, HIGH);
    digitalWrite(LED_UMIDO, LOW);
    digitalWrite(LED_CHUVOSO_UMIDO, LOW);
  } else if (chuva && umido) { // Chuvoso e úmido
    digitalWrite(LED_SECO, LOW);
    digitalWrite(LED_UMIDO, LOW);
    digitalWrite(LED_CHUVOSO_UMIDO, HIGH);
  } else { // Umido ou chovendo
    digitalWrite(LED_SECO, LOW);
    digitalWrite(LED_UMIDO, HIGH);
    digitalWrite(LED_CHUVOSO_UMIDO, LOW);
  }

  // ======== PUBLICAR MQTT ========
  StaticJsonDocument<200> doc;
  doc["chuva"] = chuva;
  doc["umidade"] = umido;
  doc["led_seco"] = digitalRead(LED_SECO);
  doc["led_umido"] = digitalRead(LED_UMIDO);
  doc["led_chuvoso_umido"] = digitalRead(LED_CHUVOSO_UMIDO);

  char buffer[200];
  serializeJson(doc, buffer);
  client.publish(pub_topic, buffer);

  // ======== ENVIAR PARA CLOUDFLARE (a cada 10s) ========
  if (millis() - lastSend > 10000) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(cloud_url);
      http.addHeader("Content-Type", "application/json");

      StaticJsonDocument<200> payload;
      payload["id"] = device_id;
      payload["chuva"] = chuva;
      payload["umidade"] = umido;
      payload["timestamp"] = millis();

      String json;
      serializeJson(payload, json);

      int httpResponseCode = http.POST(json);
      Serial.print("POST Cloudflare -> ");
      Serial.println(httpResponseCode);
      http.end();
    }
    lastSend = millis();
  }

  delay(500);
}
