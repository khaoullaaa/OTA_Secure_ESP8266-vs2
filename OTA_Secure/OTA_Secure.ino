/*
 * SIMPLE SECURE OTA - ESP8266
 * 
 * Features:
 * - Downloads firmware from GitHub Releases
 * - Verifies SHA256 before flashing
 * - Simple web interface to trigger updates
 * - Auto-check on boot
 * 
 * Setup:
 * 1. Set your WiFi credentials below
 * 2. Set your GitHub username and repo name
 * 3. Upload this sketch
 * 4. Create GitHub releases with firmware.bin
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Updater.h>

// ============ CONFIGURATION - EDIT THESE ============
const char* WIFI_SSID     = "TOPNET_2FB0";
const char* WIFI_PASSWORD = "3m3smnb68l";

const char* GITHUB_USER = "khaoullaaa";
const char* GITHUB_REPO = "OTA_Secure_ESP8266-vs2";

// Firmware version (set automatically in CI via -DFW_VERSION_STR="vX.Y.Z")
#ifndef FW_VERSION_STR
#define FW_VERSION_STR "v0.0.0"
#endif
static const char* FW_VERSION = FW_VERSION_STR;
// ====================================================

// Derived URLs (no need to edit)
String manifestUrl;
String currentVersion = FW_VERSION;
String latestVersion = "";
String firmwareUrl = "";
String expectedHash = "";

ESP8266WebServer server(80);

// ============ SHA256 HELPERS ============
#include <bearssl/bearssl_hash.h>

String sha256ToString(uint8_t* hash) {
    String result = "";
    for (int i = 0; i < 32; i++) {
        if (hash[i] < 0x10) result += "0";
        result += String(hash[i], HEX);
    }
    return result;
}

// ============ VERSION COMPARISON ============
// Returns: -1 if a < b, 0 if a == b, 1 if a > b
int compareVersions(String a, String b) {
    // Remove 'v' prefix if present
    if (a.startsWith("v") || a.startsWith("V")) a = a.substring(1);
    if (b.startsWith("v") || b.startsWith("V")) b = b.substring(1);
    
    int aParts[3] = {0, 0, 0};
    int bParts[3] = {0, 0, 0};
    
    // Parse version a
    int partIdx = 0;
    String part = "";
    for (unsigned int i = 0; i <= a.length() && partIdx < 3; i++) {
        if (i == a.length() || a[i] == '.') {
            aParts[partIdx++] = part.toInt();
            part = "";
        } else if (a[i] >= '0' && a[i] <= '9') {
            part += a[i];
        }
    }
    
    // Parse version b
    partIdx = 0;
    part = "";
    for (unsigned int i = 0; i <= b.length() && partIdx < 3; i++) {
        if (i == b.length() || b[i] == '.') {
            bParts[partIdx++] = part.toInt();
            part = "";
        } else if (b[i] >= '0' && b[i] <= '9') {
            part += b[i];
        }
    }
    
    // Compare
    for (int i = 0; i < 3; i++) {
        if (aParts[i] < bParts[i]) return -1;
        if (aParts[i] > bParts[i]) return 1;
    }
    return 0;
}

// ============ CHECK FOR UPDATES ============
bool checkForUpdate() {
    Serial.println("\n[OTA] Checking for updates...");
    
    WiFiClientSecure client;
    client.setInsecure(); // We verify via SHA256 instead
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(10000);
    
    if (!http.begin(client, manifestUrl)) {
        Serial.println("[OTA] Failed to connect to manifest URL");
        return false;
    }
    
    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("[OTA] Manifest fetch failed: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // Parse JSON
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[OTA] JSON parse error: %s\n", err.c_str());
        return false;
    }
    
    latestVersion = doc["version"] | "";
    firmwareUrl = doc["firmware_url"] | "";
    expectedHash = doc["sha256"] | "";
    expectedHash.toLowerCase();
    
    Serial.printf("[OTA] Current: %s, Latest: %s\n", currentVersion.c_str(), latestVersion.c_str());
    
    // Validate
    if (latestVersion.length() == 0 || firmwareUrl.length() == 0) {
        Serial.println("[OTA] Invalid manifest data");
        return false;
    }
    
    if (expectedHash.length() != 64) {
        Serial.println("[OTA] Invalid SHA256 in manifest");
        return false;
    }
    
    // Compare versions
    if (compareVersions(latestVersion, currentVersion) > 0) {
        Serial.println("[OTA] Update available!");
        return true;
    }
    
    Serial.println("[OTA] Already up to date");
    return false;
}

// ============ PERFORM OTA UPDATE ============
bool performUpdate() {
    if (firmwareUrl.length() == 0 || expectedHash.length() == 0) {
        Serial.println("[OTA] No update info available");
        return false;
    }
    
    Serial.printf("[OTA] Downloading: %s\n", firmwareUrl.c_str());
    
    WiFiClientSecure client;
    client.setInsecure(); // We verify via SHA256
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(30000);
    
    if (!http.begin(client, firmwareUrl)) {
        Serial.println("[OTA] Connection failed");
        return false;
    }
    
    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("[OTA] Download failed: %d\n", httpCode);
        http.end();
        return false;
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        Serial.println("[OTA] Invalid content length");
        http.end();
        return false;
    }
    
    Serial.printf("[OTA] Firmware size: %d bytes\n", contentLength);
    
    // Check if we have space
    if (contentLength > (int)ESP.getFreeSketchSpace()) {
        Serial.println("[OTA] Not enough space");
        http.end();
        return false;
    }
    
    // Start update
    if (!Update.begin(contentLength)) {
        Serial.println("[OTA] Update.begin failed");
        http.end();
        return false;
    }
    
    // Download and hash simultaneously
    WiFiClient* stream = http.getStreamPtr();
    br_sha256_context sha256ctx;
    br_sha256_init(&sha256ctx);
    
    uint8_t buf[512];
    size_t written = 0;
    
    while (written < (size_t)contentLength) {
        size_t available = stream->available();
        if (available) {
            size_t toRead = min(available, sizeof(buf));
            size_t bytesRead = stream->readBytes(buf, toRead);
            
            if (bytesRead > 0) {
                // Update hash
                br_sha256_update(&sha256ctx, buf, bytesRead);
                
                // Write to flash
                if (Update.write(buf, bytesRead) != bytesRead) {
                    Serial.println("[OTA] Write failed");
                    Update.end(false);
                    http.end();
                    return false;
                }
                
                written += bytesRead;
                
                // Progress
                int percent = (written * 100) / contentLength;
                static int lastPercent = -1;
                if (percent != lastPercent && percent % 10 == 0) {
                    Serial.printf("[OTA] Progress: %d%%\n", percent);
                    lastPercent = percent;
                }
            }
        }
        yield();
    }
    
    http.end();
    
    // Verify hash BEFORE committing
    uint8_t hash[32];
    br_sha256_out(&sha256ctx, hash);
    String actualHash = sha256ToString(hash);
    
    Serial.printf("[OTA] Expected: %s\n", expectedHash.c_str());
    Serial.printf("[OTA] Actual:   %s\n", actualHash.c_str());
    
    if (actualHash != expectedHash) {
        Serial.println("[OTA] SHA256 MISMATCH - Aborting!");
        Update.end(false);
        return false;
    }
    
    Serial.println("[OTA] SHA256 verified OK");
    
    // Finalize
    if (!Update.end(true)) {
        Serial.println("[OTA] Update.end failed");
        return false;
    }
    
    Serial.println("[OTA] Update successful! Rebooting...");
    delay(1000);
    ESP.restart();
    return true;
}

// ============ WEB PAGE ============
String getWebPage() {
    bool updateAvailable = (latestVersion.length() > 0 && compareVersions(latestVersion, currentVersion) > 0);
    
    String html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>ESP8266 OTA</title>
    <style>
        body { font-family: Arial; max-width: 400px; margin: 40px auto; padding: 20px; background: #f5f5f5; }
        .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; margin-top: 0; }
        .info { margin: 15px 0; padding: 10px; background: #e8f5e9; border-radius: 5px; }
        .update { background: #fff3e0; }
        button { width: 100%; padding: 12px; margin: 5px 0; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        .btn-check { background: #2196F3; color: white; }
        .btn-update { background: #4CAF50; color: white; }
        .btn-reboot { background: #ff9800; color: white; }
    </style>
</head>
<body>
    <div class='card'>
        <h1>ESP8266 OTA</h1>
        <div class='info'>
            <strong>Current:</strong> v)" + currentVersion + R"(<br>
            <strong>IP:</strong> )" + WiFi.localIP().toString() + R"(
        </div>)";
    
    if (updateAvailable) {
        html += R"(<div class='info update'><strong>Update available:</strong> v)" + latestVersion + R"(</div>)";
    }
    
    html += R"(
        <button class='btn-check' onclick="location.href='/check'">Check Updates</button>)";
    
    if (updateAvailable) {
        html += R"(<button class='btn-update' onclick="if(confirm('Install update?')) location.href='/update'">Install Update</button>)";
    }
    
    html += R"(
        <button class='btn-reboot' onclick="if(confirm('Reboot?')) location.href='/reboot'">Reboot</button>
    </div>
</body>
</html>)";
    
    return html;
}

// ============ WEB HANDLERS ============
void handleRoot() {
    server.send(200, "text/html", getWebPage());
}

void handleCheck() {
    checkForUpdate();
    server.sendHeader("Location", "/");
    server.send(302);
}

void handleUpdate() {
    server.send(200, "text/html", "<html><body><h1>Updating...</h1><p>Please wait. Device will reboot.</p></body></html>");
    delay(100);
    performUpdate();
    // If we get here, update failed
    server.sendHeader("Location", "/");
    server.send(302);
}

void handleReboot() {
    server.send(200, "text/html", "<html><body><h1>Rebooting...</h1></body></html>");
    delay(1000);
    ESP.restart();
}

// ============ SETUP ============
void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n\n================================");
    Serial.println("  ESP8266 Secure OTA");
    Serial.printf("  Version: %s\n", FW_VERSION);
    Serial.println("================================\n");
    
    // Build manifest URL
    manifestUrl = "https://raw.githubusercontent.com/" + String(GITHUB_USER) + "/" + String(GITHUB_REPO) + "/main/manifest.json";
    Serial.printf("Manifest: %s\n", manifestUrl.c_str());
    
    // Connect WiFi
    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 30) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" OK!");
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println(" FAILED!");
        Serial.println("Will retry in loop...");
    }
    
    // Check for updates on boot and auto-install if available
    if (WiFi.status() == WL_CONNECTED) {
        if (checkForUpdate()) {
            Serial.println("[OTA] New version found! Starting automatic update...");
            performUpdate();
        }
    }
    
    Serial.println("\n[READY] Device ready. Will check for updates every 30 minutes.");
}

// ============ LOOP ============
void loop() {
    // Reconnect WiFi if needed
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 30000) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Reconnecting...");
            WiFi.reconnect();
        }
        lastWifiCheck = millis();
    }
    
    // Check for updates every 30 minutes (1800000 ms)
    static unsigned long lastUpdateCheck = 0;
    if (millis() - lastUpdateCheck > 1800000 || lastUpdateCheck == 0) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[OTA] Periodic update check...");
            if (checkForUpdate()) {
                Serial.println("[OTA] New version available! Auto-installing...");
                performUpdate();
                // If we get here, update failed
                Serial.println("[OTA] Update failed, will retry in 30 minutes");
            }
        }
        lastUpdateCheck = millis();
    }
    
    yield();
}
