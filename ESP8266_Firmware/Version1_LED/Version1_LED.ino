// Version 1.0.0 - Contr�le LED
#include <ESP8266WiFi.h>

const char* ssid = "TOPNET_ELML";
const char* password = "sdwl4yna8k";

#define LED_PIN D4

void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED éteinte (actif bas)
  
  Serial.println("\n=== Version 1.0.0 - LED Control ===");
  Serial.println("Fonction: Contrôle LED");
  
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
  } else {
    Serial.println("\n❌ WiFi timeout - Continuer sans connexion");
  }
}

void loop() {
  // Clignotement LED
  digitalWrite(LED_PIN, LOW);
  delay(500);
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  
  // Affichage p�riode
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println("s");
    lastPrint = millis();
  }
}
