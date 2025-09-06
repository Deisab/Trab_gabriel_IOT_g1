#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

#define SS_PIN 21
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

// ======= Config WiFi =======
const char* ssid = "AMF";
const char* password = "amf@2025";

// ======= URL do Worker =======
String workerURL = "https://kv-teste.deisab12.workers.dev/get";

// ======= LEDs =======
#define LED_VERDE 25
#define LED_VERMELHO 26

void setup() {
  Serial.begin(115200);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  // Conectar WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi conectado!");

  // Inicializar RFID
  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Aproxime o cartão/tag do leitor...");
}

void loop() {
  // Verifica se há novo cartão
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Monta UID com dois pontos
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) uidString += ":";
    if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();

  Serial.println("UID lido: " + uidString);

  // Verifica se UID está cadastrado
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String requestURL = workerURL + "?uid=" + uidString;
    http.begin(requestURL);

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      payload.trim();

      if (payload.startsWith("{")) {
        // Parse JSON retornado
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          String id = doc["id"].as<String>();

          // ✅ Verifica se o UID lido corresponde ao ID cadastrado
          if (id == uidString) {
            Serial.println("✅ UID lido está CADASTRADO no banco!");
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_VERMELHO, LOW);
          } else {
            Serial.println("❌ UID lido NÃO corresponde ao banco!");
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_VERMELHO, HIGH);
          }

        } else {
          Serial.println("⚠️ Erro ao interpretar JSON: " + String(error.c_str()));
          digitalWrite(LED_VERDE, LOW);
          digitalWrite(LED_VERMELHO, HIGH);
        }

      } else {
        Serial.println("❌ UID NÃO está no banco.");
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_VERMELHO, HIGH);
      }

    } else {
      Serial.println("❌ Erro na requisição HTTP: " + String(httpResponseCode));
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_VERMELHO, HIGH);
    }

    http.end();

  } else {
    Serial.println("⚠️ WiFi desconectado!");
  }

  // Finaliza leitura do cartão
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // LEDs acesos por 2 segundos e depois apagam
  delay(2000);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
}