# 🚀 ESP8266 OTA Update System with GitHub Integration

Secure Over-The-Air (OTA) firmware update system for ESP8266 microcontrollers with automated GitHub CI/CD pipeline.

## ✨ Features

### 🔐 Security
- ✅ **SSL/TLS Certificate Validation** - Fingerprint verification for GitHub connections
- ✅ **SHA256 Integrity Check** - Firmware verification before flashing
- ✅ **Secure HTTPS** - All communications encrypted
- ✅ **Auto-reconnect WiFi** - Network resilience
- ✅ **Watchdog Protection** - Prevents system lockups

### 📡 OTA Updates
- ✅ **GitHub Releases** - Automatic firmware distribution
- ✅ **Version Management** - Semantic versioning support
- ✅ **Progress Tracking** - Real-time update progress
- ✅ **Rollback Protection** - Version validation before update
- ✅ **Automatic Checks** - Hourly update verification

### 🌐 Web Interface
- ✅ **Modern Responsive UI** - Mobile-friendly dashboard
- ✅ **Real-time Status** - Live system information
- ✅ **Manual Updates** - User-initiated firmware updates
- ✅ **System Info** - Detailed device diagnostics
- ✅ **LED Indicators** - Visual status feedback

### 🛠️ Development
- ✅ **GitHub Actions CI/CD** - Automated build and release
- ✅ **Local Test Server** - Python-based OTA server
- ✅ **Multiple Versions** - Progressive firmware examples
- ✅ **PlatformIO Support** - Modern build system

---

## 📋 Requirements

### Hardware
- **ESP8266** (NodeMCU v2 recommended)
- **DHT11 Sensor** (for Version 2.0.0)
- **4MB Flash** minimum
- **USB Cable** for initial programming

### Software
- **Arduino IDE** or **PlatformIO**
- **Python 3.8+** (for local server)
- **GitHub Account** (for releases)

---

## 🚀 Quick Start

### 1️⃣ Initial Setup

1. **Clone the repository**
   ```bash
   git clone https://github.com/khaoullaaa/ESP8266_OTA.git
   cd ESP8266_OTA
   ```

2. **Configure WiFi Credentials**
   
   Edit `ESP8266_Firmware/OTA_Secure/OTA_Secure.ino`:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

3. **Configure GitHub Repository**
   ```cpp
   const char* githubUser = "YOUR_GITHUB_USERNAME";
   const char* repoName = "YOUR_REPO_NAME";
   ```

4. **Upload Initial Firmware**
   - Open `OTA_Secure.ino` in Arduino IDE
   - Select Board: NodeMCU 1.0 (ESP-12E Module)
   - Upload to ESP8266

### 2️⃣ GitHub Setup

1. **Enable GitHub Actions**
   - Copy `GitHub_Actions/ota_workflow.yml` to `.github/workflows/`
   - Commit and push to your repository

2. **Create a Release**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

3. **GitHub Actions will automatically:**
   - ✅ Compile the firmware
   - ✅ Generate SHA256 hash
   - ✅ Create release with firmware
   - ✅ Update manifest.json

### 3️⃣ OTA Update

1. **Device checks for updates** (automatic every hour)
2. **Or manually trigger** via web interface
3. **Firmware downloads and flashes** automatically
4. **Device reboots** with new version

---

## 📁 Project Structure

```
ESP8266_OTA/
├── ESP8266_Firmware/
│   ├── OTA_Secure/
│   │   └── OTA_Secure.ino        # Main OTA firmware
│   ├── Version1_LED/
│   │   └── Version1_LED.ino      # v1.0.0 - LED control
│   ├── Version2_Sensor/
│   │   └── Version2_Sensor.ino   # v2.0.0 - DHT11 sensor
│   ├── platformio.ini             # PlatformIO config
│   └── manifest.json              # Firmware metadata
├── GitHub_Actions/
│   └── ota_workflow.yml           # CI/CD pipeline
├── local_ota_server.py            # Local test server
├── generate_sha256.ps1            # Hash utility
└── README.md
```

---

## 🔧 Configuration

### Static IP Address (Optional)

```cpp
IPAddress local_IP(192, 168, 1, 84);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
```

### Update GitHub Fingerprint

Get current fingerprint:
```bash
openssl s_client -connect raw.githubusercontent.com:443 < /dev/null 2>/dev/null | openssl x509 -fingerprint -noout -in /dev/stdin -sha1
```

Update in code:
```cpp
const char* githubFingerprint = "C6 06 5C F7 ...";
```

---

## 🧪 Local Testing

### Start Local OTA Server

```bash
cd ESP8266_OTA
python local_ota_server.py
```

The server will:
- ✅ Auto-detect your local IP
- ✅ Generate self-signed certificate
- ✅ Calculate firmware SHA256
- ✅ Serve manifest and firmware

### Update Firmware for Local Testing

```cpp
// Temporarily disable SSL verification
client.setInsecure();

// Update manifest URL
const char* manifestURL = "https://YOUR_LOCAL_IP:8443/manifest.json";
```

---

## 📊 Web Interface

Access the web dashboard:
```
http://192.168.1.84/
```

### Available Endpoints

| Endpoint | Description |
|----------|-------------|
| `/` | Main dashboard |
| `/check` | Check for updates |
| `/update` | Trigger OTA update |
| `/reboot` | Restart device |
| `/info` | System information |

---

## 🔒 Security Best Practices

### ✅ Production Checklist

- [ ] Enable SSL certificate validation
- [ ] Use strong WiFi passwords
- [ ] Update GitHub fingerprint regularly
- [ ] Verify SHA256 before flashing
- [ ] Use HTTPS for all connections
- [ ] Implement rollback mechanism
- [ ] Monitor update failures
- [ ] Use signed firmware releases

### ⚠️ Security Warnings

```cpp
// ❌ NEVER use in production
client.setInsecure();

// ✅ Always verify certificates
client.setFingerprint(githubFingerprint);
```

---

## 🐛 Troubleshooting

### WiFi Connection Failed
- Check SSID and password
- Verify WiFi signal strength
- Device enters AP mode: `ESP8266-OTA-Config` (password: `12345678`)

### OTA Update Failed
- Check GitHub fingerprint
- Verify firmware size < available space
- Ensure stable WiFi connection
- Check serial monitor for error codes

### Certificate Validation Failed
- Update GitHub fingerprint
- Temporarily use `client.setInsecure()` for testing
- Verify system time is correct

### LED Blink Patterns

| Pattern | Status |
|---------|--------|
| Slow (1s) | Normal operation |
| Medium (500ms) | Update available |
| Fast (200ms) | OTA in progress |
| Very Fast (100ms) | WiFi disconnected |

---

## 📦 Manifest Schema

```json
{
  "version": "2.0.0",
  "firmware_url": "https://github.com/user/repo/releases/download/v2.0.0/firmware.bin",
  "sha256": "64_char_hex_hash",
  "build_date": "2025-12-12",
  "description": "Version description",
  "min_version": "1.0.0",
  "changelog": [
    "Feature 1",
    "Feature 2"
  ]
}
```

---

## 🚀 Version History

### v2.0.0 - DHT11 Sensor
- ✅ Temperature and humidity monitoring
- ✅ Web dashboard with auto-refresh
- ✅ Sensor error detection
- ✅ Improved stability

### v1.0.0 - LED Control
- ✅ Basic WiFi connectivity
- ✅ LED blink demonstration
- ✅ Serial monitoring
- ✅ Baseline for OTA updates

---

## 📝 Development

### Build with PlatformIO

```bash
cd ESP8266_Firmware/OTA_Secure
pio run
pio run --target upload
pio device monitor
```

### Generate SHA256

**Windows PowerShell:**
```powershell
.\generate_sha256.ps1 -firmwarePath "firmware.bin"
```

**Linux/Mac:**
```bash
sha256sum firmware.bin
```

---

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open Pull Request

---

## 📄 License

This project is open source and available under the MIT License.

---

## 👤 Author

**Khaoula**
- GitHub: [@khaoullaaa](https://github.com/khaoullaaa)
- Repository: [ESP8266_OTA](https://github.com/khaoullaaa/ESP8266_OTA)

---

## 🙏 Acknowledgments

- ESP8266 Community
- Arduino Core for ESP8266
- GitHub Actions
- ArduinoJson Library

---

## 📞 Support

For issues and questions:
1. Check the [Troubleshooting](#-troubleshooting) section
2. Review closed issues on GitHub
3. Open a new issue with detailed information

---

**⭐ If this project helped you, please give it a star!**
