// Version 2.0.0 - Capteur DHT11
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

const char* ssid = "TOPNET_ELML";
const char* password = "sdwl4yna8k";

#define DHTPIN D2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

void handleRoot() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // Vérifier si les lectures sont valides
  bool sensorError = (isnan(h) || isnan(t));
  if (sensorError) {
    h = 0.0;
    t = 0.0;
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>Capteur DHT11</title>";
  html += "<style>";
  html += "body {font-family: Arial, sans-serif; background: linear-gradient(135deg, #74b9ff, #0984e3);";
  html += "height: 100vh; margin: 0; display: flex; justify-content: center; align-items: center;}";
  html += ".container {background: white; padding: 40px; border-radius: 20px; text-align: center; box-shadow: 0 20px 60px rgba(0,0,0,0.3);}";
  html += "h1 {color: #333; margin-bottom: 30px;}";
  html += ".data {display: flex; justify-content: space-around; margin: 30px 0;}";
  html += ".sensor {padding: 20px; border-radius: 10px; width: 45%;}";
  html += ".temp {background: #ffeaa7; color: #d63031;}";
  html += ".hum {background: #a29bfe; color: #2d3436;}";
  html += ".error {background: #ff7675; color: white;}";
  html += ".value {font-size: 48px; font-weight: bold;}";
  html += ".unit {font-size: 20px; color: #636e72;}";
  html += ".version {margin-top: 30px; color: #636e72; font-size: 14px;}";
  html += ".status {margin-top: 10px; font-size: 12px; color: #95a5a6;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🌡️ Station Météo Version 2.0.0</h1>";
  
  if (sensorError) {
    html += "<div class='sensor error'><h3>⚠️ Erreur Capteur</h3><p>Vérifiez le DHT11</p></div>";
  } else {
    html += "<div class='data'>";
    html += "<div class='sensor temp'><h3>Température</h3><div class='value'>" + String(t, 1) + "<span class='unit'>°C</span></div></div>";
    html += "<div class='sensor hum'><h3>Humidité</h3><div class='value'>" + String(h, 1) + "<span class='unit'>%</span></div></div>";
    html += "</div>";
  }
  
  html += "<div class='version'>IP: " + WiFi.localIP().toString() + " | Version: 2.0.0</div>";
  html += "<div class='status'>Actualisation automatique toutes les 5 secondes</div>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n=== Version 2.0.0 - DHT11 Sensor ===");
  Serial.println("Fonction: Capteur DHT11");
  
  dht.begin();
  Serial.println("DHT11 initialisé");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 40) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connecté");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    server.on("/", handleRoot);
    server.begin();
    Serial.println("✅ Serveur web démarré");
  } else {
    Serial.println("\n❌ WiFi timeout - Mode dégradé");
  }
}

void loop() {
  server.handleClient();
  
  // Lecture p�riodique du capteur
  static unsigned long lastRead = 0;
  if (millis() - lastRead > 2000) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)) {
      Serial.println("Erreur lecture DHT!");
    } else {
      Serial.print("Temp�rature: ");
      Serial.print(t);
      Serial.print("�C, Humidit�: ");
      Serial.print(h);
      Serial.println("%");
    }
    lastRead = millis();
  }
}
