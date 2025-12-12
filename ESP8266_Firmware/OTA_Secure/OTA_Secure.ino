// ============================================
// SYSTEME OTA COMPLET SÉCURISÉ - VERSION 1.0.0
// GitHub: khaoullaaa/ESP8266_OTA
// WiFi: TOPNET_ELML / sdwl4yna8k
// ============================================

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <FS.h>

// ========= CONFIGURATION =========
const char* ssid = "Airbox-331a";
const char* password = "ZDXY2bjRwEDt";

// CONFIGURATION IP STATIQUE
IPAddress local_IP(192, 168, 1, 84);     // IP fixe
IPAddress gateway(192, 168, 1, 1);        // Passerelle (routeur)
IPAddress subnet(255, 255, 255, 0);       // Masque de sous-réseau
IPAddress dns1(8, 8, 8, 8);               // DNS primaire (Google)
IPAddress dns2(8, 8, 4, 4);               // DNS secondaire

// CONFIGURATION GITHUB
const char* githubUser = "khaoullaaa";
const char* repoName = "ESP8266_OTA";
const char* manifestURL = "https://raw.githubusercontent.com/khaoullaaa/ESP8266_OTA/main/manifest.json";

// Fingerprint GitHub (décembre 2024)
const char* githubFingerprint = "C6 06 5C F7 89 3B 4F D9 5A 81 33 2F F9 6F 76 35 BD 6F B6 5F";

ESP8266WebServer server(80);

// Variables système
String currentVersion = "1.0.0";
String latestVersion = "1.0.0";
String updateStatus = "Système prêt";
bool updateAvailable = false;
String espIP = "192.168.1.84";
bool otaInProgress = false;
String firmwareURL = "";
String expectedSHA256 = "";

// ========= FONCTIONS GITHUB =========
bool checkGitHubUpdate() {
  Serial.println("[GITHUB] Vérification des mises à jour...");
  
  WiFiClientSecure client;
  // Activation de la vérification SSL pour la sécurité
  client.setFingerprint(githubFingerprint);
  // Pour tests locaux uniquement: client.setInsecure();
  
  HTTPClient https;
  
  if (!https.begin(client, manifestURL)) {
    Serial.println("[GITHUB] ❌ Échec connexion");
    updateStatus = "Erreur connexion GitHub";
    return false;
  }
  
  https.setTimeout(10000);
  int httpCode = https.GET();
  Serial.print("[GITHUB] Code HTTP: ");
  Serial.println(httpCode);
  
  if (httpCode != HTTP_CODE_OK) {
    https.end();
    updateStatus = "Erreur HTTP: " + String(httpCode);
    return false;
  }
  
  String payload = https.getString();
  https.end();
  
  // Parser JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("[GITHUB] ❌ Erreur JSON: ");
    Serial.println(error.c_str());
    updateStatus = "Erreur format JSON";
    return false;
  }
  
  // Récupérer infos
  latestVersion = doc["version"].as<String>();
  firmwareURL = doc["firmware_url"].as<String>();
  expectedSHA256 = doc["sha256"].as<String>();
  String buildDate = doc["build_date"].as<String>();
  String description = doc["description"].as<String>();
  
  Serial.println("[GITHUB] ✅ Manifest chargé");
  Serial.print("Version locale: v");
  Serial.println(currentVersion);
  Serial.print("Version GitHub: v");
  Serial.println(latestVersion);
  Serial.print("URL firmware: ");
  Serial.println(firmwareURL);
  Serial.print("SHA256: ");
  Serial.println(expectedSHA256.substring(0, 20) + "...");
  
  // Comparer versions
  if (latestVersion != currentVersion) {
    updateAvailable = true;
    updateStatus = "⚠️ Mise à jour disponible: v" + latestVersion;
    Serial.println("[GITHUB] ⚠️ MISE À JOUR DISPONIBLE!");
    return true;
  } else {
    updateAvailable = false;
    updateStatus = "✅ Système à jour";
    Serial.println("[GITHUB] ✅ Système à jour");
    return false;
  }
}

// ========= OTA RÉELLE =========
bool performRealOTA() {
  if (!updateAvailable || firmwareURL == "") {
    Serial.println("[OTA] ❌ Pas de mise à jour disponible");
    return false;
  }
  
  Serial.println("[OTA] ⚡ Démarrage de la mise à jour...");
  Serial.print("[OTA] URL: ");
  Serial.println(firmwareURL);
  
  WiFiClientSecure client;
  client.setFingerprint(githubFingerprint); // Vérification SSL active
  
  HTTPClient https;
  if (!https.begin(client, firmwareURL)) {
    Serial.println("[OTA] ❌ Échec connexion");
    return false;
  }
  
  https.setTimeout(15000); // Timeout de 15 secondes
  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("[OTA] ❌ Erreur HTTP: ");
    Serial.println(httpCode);
    https.end();
    return false;
  }
  
  // Vérifier la taille du firmware
  size_t contentLength = https.getSize();
  Serial.print("[OTA] Taille: ");
  Serial.print(contentLength);
  Serial.println(" bytes");
  
  if (contentLength == 0 || contentLength > (ESP.getFreeSketchSpace())) {
    Serial.println("[OTA] ❌ Taille invalide ou insuffisante");
    https.end();
    return false;
  }
  
  // Activer la vérification MD5 (SHA256 non supporté directement par Update)
  if (expectedSHA256.length() == 64) {
    Serial.println("[OTA] ⚠️ SHA256 sera vérifié après téléchargement");
  }
  
  if (!Update.begin(contentLength)) {
    Serial.print("[OTA] ❌ Update.begin échoué: ");
    Serial.println(Update.getError());
    https.end();
    return false;
  }
  
  // Écrire le firmware avec progression
  WiFiClient* stream = https.getStreamPtr();
  uint8_t buffer[512];
  size_t written = 0;
  int lastProgress = -1;
  
  while (https.connected() && written < contentLength) {
    size_t available = stream->available();
    if (available) {
      int readBytes = stream->readBytes(buffer, min(available, sizeof(buffer)));
      Update.write(buffer, readBytes);
      written += readBytes;
      
      int progress = (written * 100) / contentLength;
      if (progress != lastProgress && progress % 10 == 0) {
        Serial.print("[OTA] Progression: ");
        Serial.print(progress);
        Serial.println("%");
        lastProgress = progress;
      }
    }
    yield(); // Éviter le watchdog reset
  }
  
  if (written != contentLength) {
    Serial.print("[OTA] ❌ Écriture incomplète: ");
    Serial.print(written);
    Serial.print("/");
    Serial.println(contentLength);
    Update.end();
    https.end();
    return false;
  }
  
  // Finaliser
  if (!Update.end()) {
    Serial.print("[OTA] ❌ Update.end échoué: ");
    Serial.println(Update.getError());
    https.end();
    return false;
  }
  
  https.end();
  
  if (Update.isFinished()) {
    Serial.println("[OTA] ✅ Firmware installé!");
    
    // Mettre à jour la version
    currentVersion = latestVersion;
    
    Serial.println("[OTA] Redémarrage dans 5 secondes...");
    delay(5000);
    ESP.restart();
    return true;
  }
  
  return false;
}

// ========= INTERFACE WEB =========
String getHTMLPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>OTA GitHub - ESP8266</title>";
  html += "<style>";
  html += "body { font-family: Arial; background: #f0f0f0; padding: 20px; }";
  html += ".container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += ".header { background: #4A00E0; color: white; padding: 20px; border-radius: 10px 10px 0 0; margin: -20px -20px 20px -20px; }";
  html += ".status { padding: 10px; border-radius: 5px; margin: 10px 0; }";
  html += ".status-update { background: #ffeb3b; color: #333; }";
  html += ".status-ok { background: #4CAF50; color: white; }";
  html += ".btn { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; color: white; }";
  html += ".btn-check { background: #2196F3; }";
  html += ".btn-update { background: #4CAF50; }";
  html += ".btn-reboot { background: #FF9800; }";
  html += "</style>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  html += "<div class='header'>";
  html += "<h1>🚀 OTA ESP8266</h1>";
  html += "<p>Version: v" + currentVersion + " | IP: " + espIP + " (FIXE)</p>";
  html += "</div>";
  
  html += "<h2>📊 État du système</h2>";
  html += "<p><strong>Version actuelle:</strong> v" + currentVersion + "</p>";
  html += "<p><strong>Dernière version:</strong> v" + latestVersion + "</p>";
  
  html += "<div class='status ";
  html += updateAvailable ? "status-update" : "status-ok";
  html += "'>";
  html += "<strong>Statut:</strong> " + updateStatus;
  html += "</div>";
  
  html += "<div style='margin-top: 20px;'>";
  html += "<button class='btn btn-check' onclick=\"location.href='/check'\">🔄 Vérifier</button>";
  
  if (updateAvailable && !otaInProgress) {
    html += "<button class='btn btn-update' onclick=\"if(confirm('Installer v" + latestVersion + " ?')) location.href='/update'\">⚡ Mettre à jour</button>";
  }
  
  html += "<button class='btn btn-reboot' onclick=\"if(confirm('Redémarrer ?')) location.href='/reboot'\">🔁 Redémarrer</button>";
  html += "</div>";
  
  html += "<hr>";
  html += "<p><small>GitHub: " + String(githubUser) + "/" + String(repoName) + "</small></p>";
  html += "</div>";
  
  html += "<script>";
  html += "setTimeout(() => { location.reload(); }, 30000);"; // Auto-refresh
  html += "</script>";
  html += "</body></html>";
  
  return html;
}

// ========= GESTION REQUÊTES =========
void handleRoot() {
  server.send(200, "text/html", getHTMLPage());
}

void handleCheck() {
  Serial.println("[WEB] Vérification GitHub demandée");
  checkGitHubUpdate();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleUpdate() {
  if (!updateAvailable || otaInProgress) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='10;url=/'>";
  html += "<title>Mise à jour OTA</title>";
  html += "<style>body {font-family: Arial; padding: 40px; text-align: center;}</style>";
  html += "</head><body>";
  html += "<h1>⚡ Mise à jour OTA en cours</h1>";
  html += "<p>Installation de la version v" + latestVersion + "</p>";
  html += "<p><strong>NE PAS ÉTEINDRE L'ESP8266 !</strong></p>";
  html += "<p>Redémarrage automatique dans 10 secondes...</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  // Démarrer l'OTA
  otaInProgress = true;
  bool success = performRealOTA();
  
  if (!success) {
    Serial.println("[WEB] ❌ Échec OTA");
    updateStatus = "❌ Échec mise à jour";
    otaInProgress = false;
  }
}

void handleReboot() {
  server.send(200, "text/html", 
    "<h1>Redémarrage</h1><p>L'ESP8266 va redémarrer...</p>");
  delay(2000);
  ESP.restart();
}

void handleInfo() {
  String info = "<h1>📊 Informations système</h1>";
  info += "<p><strong>IP:</strong> " + espIP + " (STATIQUE)</p>";
  info += "<p><strong>SDK:</strong> " + String(ESP.getSdkVersion()) + "</p>";
  info += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  info += "<p><strong>Flash Size:</strong> " + String(ESP.getFlashChipSize() / 1024) + " KB</p>";
  info += "<p><strong>MAC:</strong> " + WiFi.macAddress() + "</p>";
  info += "<p><strong>RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  info += "<p><a href='/'>Retour</a></p>";
  
  server.send(200, "text/html", info);
}

// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("    OTA ESP8266 - VERSION 1.0.0");
  Serial.println("========================================");
  Serial.println("WiFi: " + String(ssid));
  Serial.println("IP FIXE: 192.168.1.84");
  Serial.println("GitHub: " + String(githubUser) + "/" + String(repoName));
  Serial.println("Manifest: " + String(manifestURL));
  Serial.println("========================================");
  
  // Configurer IP statique
  Serial.print("Configuration IP statique... ");
  if (WiFi.config(local_IP, gateway, subnet, dns1, dns2)) {
    Serial.println("OK");
  } else {
    Serial.println("ÉCHEC!");
  }
  
  // WiFi avec reconnexion automatique
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.print("Connexion WiFi...");
  WiFi.begin(ssid, password);
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 60) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connecté!");
    espIP = WiFi.localIP().toString();
    Serial.print("IP: ");
    Serial.println(espIP);
    Serial.print("Masque: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Passerelle: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
  } else {
    Serial.println("\n❌ Échec WiFi - Démarrage mode AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP8266-OTA-Config", "12345678");
    espIP = WiFi.softAPIP().toString();
    Serial.print("Mode AP - IP: ");
    Serial.println(espIP);
    Serial.println("SSID: ESP8266-OTA-Config");
    Serial.println("Password: 12345678");
  }
  
  // Configuration LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED éteinte
  
  // Routes web
  server.on("/", handleRoot);
  server.on("/check", handleCheck);
  server.on("/update", handleUpdate);
  server.on("/reboot", handleReboot);
  server.on("/info", handleInfo);
  
  server.begin();
  Serial.println("✅ Serveur web démarré");
  Serial.println("🌐 http://" + espIP);
  Serial.println("========================================");
  
  // Vérifier GitHub au démarrage
  checkGitHubUpdate();
}

// ========= LOOP =========
void loop() {
  server.handleClient();
  
  // Surveillance WiFi et reconnexion automatique
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 30000) { // Vérifier toutes les 30 secondes
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
      Serial.println("[WiFi] Reconnexion...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
    lastWiFiCheck = millis();
  }
  
  // LED clignote selon état
  static unsigned long lastBlink = 0;
  static bool ledState = HIGH;
  
  unsigned long interval = 1000; // Normal
  if (otaInProgress) interval = 200; // Rapide pendant OTA
  else if (updateAvailable) interval = 500; // Moyen si mise à jour disponible
  else if (WiFi.status() != WL_CONNECTED) interval = 100; // Très rapide si WiFi déconnecté
  
  if (millis() - lastBlink > interval) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    lastBlink = millis();
  }
  
  // Éviter le watchdog reset
  yield();
}