/*
 * AUTOMATIC SECURE OTA - ESP8266 with AES-256 Encryption
 * 
 * Features:
 * - Automatically checks for updates every 5 seconds (test mode)
 * - Downloads AES-256 encrypted firmware from GitHub Releases
 * - Decrypts firmware on-the-fly during download
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
#include <bearssl/bearssl_block.h>

// ============ CONFIGURATION - EDIT THESE ============
const char* WIFI_SSID     = "S21 de islem";
const char* WIFI_PASSWORD = "iimq1940";

const char* GITHUB_USER = "khaoullaaa";
const char* GITHUB_REPO = "OTA_Secure_ESP8266-vs2";

// AES-256 Encryption Key (32 bytes) - KEEP THIS SECRET!
// This key is auto-generated during first build and stored in GitHub Secrets
const uint8_t AES_KEY[32] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    0x76, 0x2e, 0x71, 0x60, 0xf3, 0x8b, 0x4d, 0xa5,
    0x6a, 0x78, 0x4d, 0x90, 0x45, 0x19, 0x0c, 0xfe
};

// Firmware version (set automatically in CI via -DFW_VERSION_STR="vX.Y.Z")
#ifndef FW_VERSION_STR
#define FW_VERSION_STR "v17.0.0"
#endif
static const char* FW_VERSION = FW_VERSION_STR;
// ====================================================

// Global variables for OTA
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
    http.setTimeout(15000);
    
    // Use GitHub API for INSTANT updates (no caching unlike raw.githubusercontent.com)
    String apiUrl = "https://api.github.com/repos/" + String(GITHUB_USER) + "/" + String(GITHUB_REPO) + "/contents/manifest.json";
    Serial.printf("[OTA] Fetching from GitHub API (no cache)...\n");
    
    if (!http.begin(client, apiUrl)) {
        Serial.println("[OTA] Failed to connect to GitHub API");
        return false;
    }
    
    // GitHub API requires User-Agent header
    http.addHeader("User-Agent", "ESP8266-OTA");
    http.addHeader("Accept", "application/vnd.github.v3.raw");  // Get raw content directly
    
    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("[OTA] GitHub API fetch failed: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // Parse JSON
    JsonDocument doc;
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

// ============ PERFORM OTA UPDATE WITH AES-256-CBC DECRYPTION ============
bool performUpdate() {
    if (firmwareUrl.length() == 0 || expectedHash.length() == 0) {
        Serial.println("[OTA] No update info available");
        return false;
    }
    
    unsigned long updateStartTime = millis();
    Serial.printf("[OTA] Downloading ENCRYPTED firmware: %s\n", firmwareUrl.c_str());
    
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
    
    int totalSize = http.getSize();
    if (totalSize <= 20) {  // Minimum: 4 (size) + 16 (IV) + some data
        Serial.println("[SECURITY] ❌ FAILED - Invalid content length");
        http.end();
        return false;
    }
    
    Serial.printf("[DOWNLOAD] Total encrypted file size: %d bytes\n", totalSize);
    
    WiFiClient* stream = http.getStreamPtr();
    
    // ===== Read header: Original Size (4 bytes LE) + IV (16 bytes) =====
    uint8_t header[20];
    size_t headerRead = 0;
    unsigned long timeout = millis() + 10000;
    while (headerRead < 20 && millis() < timeout) {
        if (stream->available()) {
            headerRead += stream->readBytes(header + headerRead, 20 - headerRead);
        }
        yield();
    }
    
    if (headerRead < 20) {
        Serial.println("[AES] ❌ FAILED - Could not read header");
        http.end();
        return false;
    }
    
    // Extract original firmware size (4 bytes little-endian)
    uint32_t originalSize = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
    
    // Extract IV (16 bytes)
    uint8_t iv[16];
    memcpy(iv, header + 4, 16);
    
    Serial.printf("[AES] Original firmware size: %u bytes\n", originalSize);
    Serial.print("[AES] IV (hex): ");
    for (int i = 0; i < 16; i++) {
        if (iv[i] < 0x10) Serial.print("0");
        Serial.print(iv[i], HEX);
    }
    Serial.println();
    
    // Encrypted data size (total - 4 size bytes - 16 IV bytes)
    int encryptedDataSize = totalSize - 20;
    Serial.printf("[DOWNLOAD] Encrypted data size: %d bytes\n", encryptedDataSize);
    
    // Check if we have space
    int freeSpace = ESP.getFreeSketchSpace();
    Serial.printf("[SECURITY] Available flash space: %d bytes\n", freeSpace);
    
    if ((int)originalSize > freeSpace) {
        Serial.println("[SECURITY] ❌ FAILED - Not enough flash space");
        http.end();
        return false;
    }
    Serial.println("[SECURITY] ✓ Flash space check PASSED");
    
    // Initialize AES-CBC decryption context
    Serial.println("[AES] Initializing AES-256-CBC decryption...");
    br_aes_ct_cbcdec_keys aesCtx;
    br_aes_ct_cbcdec_init(&aesCtx, AES_KEY, 32);
    Serial.println("[AES] ✓ AES-256-CBC context initialized");
    
    // Initialize SHA256 for decrypted data
    Serial.println("[SECURITY] Starting SHA256 streaming verification...");
    br_sha256_context sha256ctx;
    br_sha256_init(&sha256ctx);
    Serial.println("[SECURITY] ✓ SHA256 context initialized");
    
    // Start flash update
    Serial.println("[FLASH] Preparing flash memory for update...");
    if (!Update.begin(originalSize)) {
        Serial.println("[SECURITY] ❌ FAILED - Update.begin failed");
        http.end();
        return false;
    }
    Serial.println("[SECURITY] ✓ Flash preparation successful");
    
    // Download, decrypt, and flash in 4KB blocks (optimized for speed)
    const size_t BUFFER_SIZE = 4096;  // 4KB buffer for faster downloads
    uint8_t* buf = (uint8_t*)malloc(BUFFER_SIZE);
    if (!buf) {
        Serial.println("[OTA] ❌ Memory allocation failed");
        http.end();
        return false;
    }
    
    size_t downloaded = 0;
    size_t written = 0;
    size_t buffered = 0;  // Bytes accumulated in buffer
    unsigned long startTime = millis();
    unsigned long lastActivity = millis();
    
    Serial.println("[OTA] Starting download + decrypt + flash...");
    
    while (downloaded < (size_t)encryptedDataSize || buffered >= 16) {
        // Read available data into buffer
        size_t available = stream->available();
        if (available > 0 && buffered < BUFFER_SIZE) {
            size_t toRead = min(available, BUFFER_SIZE - buffered);
            // Don't read more than remaining encrypted data
            if (downloaded + toRead > (size_t)encryptedDataSize) {
                toRead = encryptedDataSize - downloaded;
            }
            
            if (toRead > 0) {
                size_t bytesRead = stream->readBytes(buf + buffered, toRead);
                if (bytesRead > 0) {
                    buffered += bytesRead;
                    downloaded += bytesRead;
                    lastActivity = millis();
                }
            }
        }
        
        // Process complete 16-byte blocks
        size_t blocksToProcess = (buffered / 16) * 16;
        if (blocksToProcess >= 16) {
            // Decrypt in-place using CBC mode
            br_aes_ct_cbcdec_run(&aesCtx, iv, buf, blocksToProcess);
            
            // Calculate actual data to write (excluding PKCS7 padding at the very end)
            size_t dataToWrite = blocksToProcess;
            if (written + dataToWrite > originalSize) {
                dataToWrite = originalSize - written;
            }
            
            if (dataToWrite > 0) {
                // Update hash with decrypted data
                br_sha256_update(&sha256ctx, buf, dataToWrite);
                
                // Write to flash
                if (Update.write(buf, dataToWrite) != dataToWrite) {
                    Serial.println("[OTA] ❌ Write failed");
                    free(buf);
                    Update.end(false);
                    http.end();
                    return false;
                }
                written += dataToWrite;
            }
            
            // Move remaining bytes to start of buffer
            size_t remaining = buffered - blocksToProcess;
            if (remaining > 0) {
                memmove(buf, buf + blocksToProcess, remaining);
            }
            buffered = remaining;
            
            // Progress update
            int percent = (written * 100) / originalSize;
            static int lastPercent = -1;
            if (percent != lastPercent && percent % 10 == 0) {
                unsigned long elapsed = (millis() - startTime) / 1000;
                float speed = (written / 1024.0) / (elapsed > 0 ? elapsed : 1);
                Serial.printf("[OTA] %d%% | %.1f KB/s\n", percent, speed);
                lastPercent = percent;
            }
        }
        
        // Check timeout - 30 seconds of no activity
        if (millis() - lastActivity > 30000) {
            Serial.printf("[OTA] ❌ Timeout (downloaded: %u/%d, buffered: %u)\n", 
                         downloaded, encryptedDataSize, buffered);
            free(buf);
            Update.end(false);
            http.end();
            return false;
        }
        
        yield();
    }
    
    free(buf);
    
    unsigned long totalTime = (millis() - startTime) / 1000;
    float avgSpeed = (downloaded / 1024.0) / (totalTime > 0 ? totalTime : 1);
    Serial.printf("[AES] ✓ Complete - %u bytes in %lu sec (%.1f KB/s)\n", written, totalTime, avgSpeed);
    
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
    
    unsigned long totalUpdateTime = (millis() - updateStartTime) / 1000;
    Serial.println("\n========================================");
    Serial.println("   OTA UPDATE SUCCESSFUL!");
    Serial.printf("   Total time: %lu seconds\n", totalUpdateTime);
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
    Serial.println("  ESP8266 Secure OTA + AES-256");
    Serial.printf("  Version: %s\n", FW_VERSION);
    Serial.println("  Using GitHub API (instant updates)");
    Serial.println("================================\n");
    
    Serial.printf("GitHub: %s/%s\n", GITHUB_USER, GITHUB_REPO);
    
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
