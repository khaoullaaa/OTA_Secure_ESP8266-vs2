// ============================================
// CONFIGURATION EXAMPLE - RENAME TO config.h
// ============================================
// This file shows all configurable options for the OTA system
// Copy this file to config.h and update with your settings

#ifndef CONFIG_H
#define CONFIG_H

// ========= WIFI CONFIGURATION =========
// Primary WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Fallback WiFi (optional)
#define WIFI_SSID_BACKUP "BACKUP_SSID"
#define WIFI_PASSWORD_BACKUP "BACKUP_PASSWORD"

// WiFi connection timeout (seconds)
#define WIFI_TIMEOUT 60

// Enable WiFi auto-reconnect
#define WIFI_AUTO_RECONNECT true


// ========= NETWORK CONFIGURATION =========
// Static IP configuration (set to false for DHCP)
#define USE_STATIC_IP true

#if USE_STATIC_IP
  #define STATIC_IP      192, 168, 1, 84
  #define GATEWAY_IP     192, 168, 1, 1
  #define SUBNET_MASK    255, 255, 255, 0
  #define DNS_PRIMARY    8, 8, 8, 8
  #define DNS_SECONDARY  8, 8, 4, 4
#endif

// Web server port
#define WEB_SERVER_PORT 80


// ========= GITHUB CONFIGURATION =========
// Your GitHub username and repository
#define GITHUB_USER "khaoullaaa"
#define GITHUB_REPO "ESP8266_OTA"
#define GITHUB_BRANCH "main"

// GitHub certificate fingerprint (update periodically)
// Get with: openssl s_client -connect raw.githubusercontent.com:443 < /dev/null 2>/dev/null | openssl x509 -fingerprint -noout -in /dev/stdin -sha1
#define GITHUB_FINGERPRINT "C6 06 5C F7 89 3B 4F D9 5A 81 33 2F F9 6F 76 35 BD 6F B6 5F"

// Manifest URL (auto-constructed from above)
#define MANIFEST_URL "https://raw.githubusercontent.com/" GITHUB_USER "/" GITHUB_REPO "/" GITHUB_BRANCH "/ESP8266_Firmware/manifest.json"


// ========= SECURITY CONFIGURATION =========
// Enable SSL certificate validation (RECOMMENDED for production)
#define ENABLE_SSL_VALIDATION true

// Enable SHA256 firmware verification (RECOMMENDED)
#define ENABLE_SHA256_VERIFICATION true

// AP Mode credentials (when WiFi fails)
#define AP_SSID "ESP8266-OTA-Config"
#define AP_PASSWORD "12345678"


// ========= OTA UPDATE CONFIGURATION =========
// Current firmware version
#define FIRMWARE_VERSION "1.0.0"

// Auto-check for updates interval (milliseconds)
// 3600000 = 1 hour
#define UPDATE_CHECK_INTERVAL 3600000

// Enable automatic updates (if false, only manual updates)
#define AUTO_UPDATE_ENABLED false

// Maximum firmware size (bytes)
#define MAX_FIRMWARE_SIZE 1048576  // 1MB


// ========= SYSTEM CONFIGURATION =========
// Serial baud rate
#define SERIAL_BAUD 115200

// LED indicators
#define LED_ENABLED true
#define LED_PIN LED_BUILTIN

// LED blink intervals (milliseconds)
#define LED_BLINK_NORMAL 1000
#define LED_BLINK_UPDATE_AVAILABLE 500
#define LED_BLINK_OTA_PROGRESS 200
#define LED_BLINK_WIFI_ERROR 100

// Watchdog timer (milliseconds)
#define WATCHDOG_TIMEOUT 8000


// ========= DHT SENSOR CONFIGURATION (Version 2.0.0) =========
#define DHT_ENABLED false
#define DHT_PIN D2
#define DHT_TYPE DHT11
#define DHT_READ_INTERVAL 2000  // milliseconds


// ========= DEBUG CONFIGURATION =========
// Enable debug output
#define DEBUG_ENABLED true

// Debug levels: 0=None, 1=Error, 2=Warning, 3=Info, 4=Debug
#define DEBUG_LEVEL 3

// Enable memory monitoring
#define MONITOR_MEMORY true
#define MEMORY_WARNING_THRESHOLD 10000  // bytes


// ========= ADVANCED CONFIGURATION =========
// HTTP client timeout (milliseconds)
#define HTTP_TIMEOUT 15000

// Maximum retry attempts for failed operations
#define MAX_RETRY_ATTEMPTS 3

// Delay between retry attempts (milliseconds)
#define RETRY_DELAY 2000

// Enable mDNS hostname
#define MDNS_ENABLED true
#define MDNS_HOSTNAME "esp8266-ota"


// ========= COMPILE-TIME CHECKS =========
#if !defined(WIFI_SSID) || !defined(WIFI_PASSWORD)
  #error "WiFi credentials must be defined!"
#endif

#if !defined(GITHUB_USER) || !defined(GITHUB_REPO)
  #error "GitHub repository information must be defined!"
#endif

#if ENABLE_SSL_VALIDATION && !defined(GITHUB_FINGERPRINT)
  #error "GitHub fingerprint must be defined when SSL validation is enabled!"
#endif


#endif // CONFIG_H
