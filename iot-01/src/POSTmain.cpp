#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SS_PIN  5  
#define RST_PIN 2 
#define LED_PIN 25   // 👉 LED no D25 (GPIO25)

MFRC522 rfid(SS_PIN, RST_PIN);

// WiFi
const char* ssid = "AMF";
const char* password = "amf@2025";

// URL do Worker
String serverName = "https://kv-teste.deisab12.workers.dev/insert";

String uidToHex(byte *buffer, byte bufferSize) {
  String hexString = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) hexString += "0";
    hexString += String(buffer[i], HEX);
    if (i < bufferSize - 1) hexString += ":";
  }
  hexString.toUpperCase();
  return hexString;
}

void sendUID(String uid, String mac) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Envia UID + MAC
    String payload = "{\"id\":\"" + uid + "\",\"mac\":\"" + mac + "\"}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Servidor respondeu: " + response);
    } else {
      Serial.println("Erro no POST: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi desconectado");
  }
}

void setup() {
  Serial.begin(115200);

  // Configura LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // começa apagado

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado!");

  SPI.begin();
  rfid.PCD_Init();
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = uidToHex(rfid.uid.uidByte, rfid.uid.size);
  String mac = WiFi.macAddress();

  Serial.println("Cartão lido: " + uid);
  Serial.println("MAC do ESP32: " + mac);

  // 👉 Acende LED ao ler cartão
  digitalWrite(LED_PIN, HIGH);

  sendUID(uid, mac);  // envia UID + MAC

  delay(1000); // mantém LED aceso por 1s

  // 👉 Apaga LED
  digitalWrite(LED_PIN, LOW);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
