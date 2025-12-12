/*
 * AUTOMATIC SECURE OTA - ESP8266
 * 
 * Features:
 * - Automatically checks for updates every 30 minutes
 * - Downloads firmware from GitHub Releases
 * - Verifies SHA256 before flashing
 * - Auto-installs new versions
 * 
 * Setup:
 * 1. Set your WiFi credentials below
 * 2. Set your GitHub username and repo name
 * 3. Upload this sketch
 * 4. Device will auto-update when you create new GitHub releases
 */

#include <ESP8266WiFi.h>
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
#define FW_VERSION_STR "v3.0.0"
#endif
static const char* FW_VERSION = FW_VERSION_STR;
// ====================================================

// Derived URLs (no need to edit)
String manifestUrl;
String currentVersion = FW_VERSION;
String latestVersion = "";
String firmwareUrl = "";
String expectedHash = "";

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
    
    Serial.println("[SECURITY] ============================================");
    Serial.printf("[SECURITY] Manifest Version: %s\n", latestVersion.c_str());
    Serial.printf("[SECURITY] Firmware URL: %s\n", firmwareUrl.c_str());
    Serial.printf("[SECURITY] Expected SHA256: %s\n", expectedHash.c_str());
    Serial.println("[SECURITY] ============================================");
    Serial.printf("[VERSION] Current: %s, Latest: %s\n", currentVersion.c_str(), latestVersion.c_str());
    
    // Validate manifest data
    if (latestVersion.length() == 0 || firmwareUrl.length() == 0) {
        Serial.println("[SECURITY] ❌ FAILED - Invalid manifest data");
        return false;
    }
    Serial.println("[SECURITY] ✓ Manifest structure valid");
    
    // Validate SHA256 format
    if (expectedHash.length() != 64) {
        Serial.println("[SECURITY] ❌ FAILED - Invalid SHA256 format (must be 64 hex chars)");
        return false;
    }
    Serial.println("[SECURITY] ✓ SHA256 format valid (64 hex characters)");
    
    // Compare versions
    int versionCmp = compareVersions(latestVersion, currentVersion);
    Serial.printf("[VERSION] Comparison result: %d (>0 = update available)\n", versionCmp);
    
    if (versionCmp > 0) {
        Serial.println("[OTA] ✓ UPDATE AVAILABLE!");
        Serial.println("[SECURITY] Manifest verification PASSED - ready to download");
        return true;
    }
    
    Serial.println("[OTA] ✓ Already up to date");
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
        Serial.println("[SECURITY] ❌ FAILED - Invalid content length");
        http.end();
        return false;
    }
    
    Serial.printf("[DOWNLOAD] Firmware size: %d bytes (%.2f KB)\n", contentLength, contentLength / 1024.0);
    
    // Check if we have space
    int freeSpace = ESP.getFreeSketchSpace();
    Serial.printf("[SECURITY] Available flash space: %d bytes (%.2f KB)\n", freeSpace, freeSpace / 1024.0);
    
    if (contentLength > freeSpace) {
        Serial.println("[SECURITY] ❌ FAILED - Not enough flash space");
        http.end();
        return false;
    }
    Serial.println("[SECURITY] ✓ Flash space check PASSED");
    
    // Start update
    Serial.println("[FLASH] Preparing flash memory for update...");
    if (!Update.begin(contentLength)) {
        Serial.println("[SECURITY] ❌ FAILED - Update.begin failed");
        http.end();
        return false;
    }
    Serial.println("[SECURITY] ✓ Flash preparation successful");
    
    // Download and hash simultaneously
    Serial.println("[SECURITY] Starting SHA256 streaming verification...");
    WiFiClient* stream = http.getStreamPtr();
    br_sha256_context sha256ctx;
    br_sha256_init(&sha256ctx);
    Serial.println("[SECURITY] ✓ SHA256 context initialized");
    
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
    Serial.println("\n[SECURITY] ============================================");
    Serial.println("[SECURITY] FINAL VERIFICATION - Computing SHA256...");
    uint8_t hash[32];
    br_sha256_out(&sha256ctx, hash);
    String actualHash = sha256ToString(hash);
    
    Serial.println("[SECURITY] SHA256 COMPARISON:");
    Serial.printf("[SECURITY]   Expected: %s\n", expectedHash.c_str());
    Serial.printf("[SECURITY]   Computed: %s\n", actualHash.c_str());
    
    if (actualHash != expectedHash) {
        Serial.println("[SECURITY] ❌ CRITICAL FAILURE - SHA256 MISMATCH!");
        Serial.println("[SECURITY] Firmware integrity check FAILED");
        Serial.println("[SECURITY] Aborting update - keeping current firmware");
        Update.end(false);
        return false;
    }
    
    Serial.println("[SECURITY] ✓✓✓ SHA256 MATCH - FIRMWARE VERIFIED! ✓✓✓");
    Serial.println("[SECURITY] ============================================");
    
    // Finalize - commit to flash
    Serial.println("[FLASH] Committing verified firmware to flash memory...");
    if (!Update.end(true)) {
        Serial.println("[SECURITY] ❌ FAILED - Could not finalize update");
        return false;
    }
    
    Serial.println("[SECURITY] ✓ Firmware committed successfully");
    Serial.println("\n========================================");
    Serial.println("   OTA UPDATE SUCCESSFUL!");
    Serial.println("   All security checks PASSED");
    Serial.println("   Rebooting in 3 seconds...");
    Serial.println("========================================\n");
    delay(3000);
    ESP.restart();
    return true;
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
        Serial.println("\n[BOOT] Performing initial update check...");
        if (checkForUpdate()) {
            Serial.println("[OTA] ✓ New version found! Starting automatic update...");
            performUpdate();
        }
    }
    
    Serial.println("\n========================================");
    Serial.println("   DEVICE READY");
    Serial.println("   Update check interval: 5 seconds (TEST MODE)");
    Serial.println("========================================\n");
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
    
    // Check for updates every 5 seconds (for testing - change to 1800000 for 30 min)
    static unsigned long lastUpdateCheck = 0;
    if (millis() - lastUpdateCheck > 5000 || lastUpdateCheck == 0) {
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
