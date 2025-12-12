# 🚀 ESP8266 Secure OTA via GitHub

**Simple, secure Over-The-Air firmware updates for ESP8266 using GitHub Releases + Arduino IDE.**

## ✨ Features

- ✅ **GitHub-hosted OTA** - Firmware served from GitHub Releases
- ✅ **SHA256 verification** - Every firmware verified before flashing
- ✅ **HTTPS downloads** - Secure firmware delivery
- ✅ **Automatic versioning** - CI/CD updates manifest on every release
- ✅ **Web interface** - Check and install updates via browser
- ✅ **Auto-check on boot** - Device checks for updates automatically

---

## 📋 Requirements

### Hardware
- **ESP8266** (NodeMCU v2 recommended)
- **4MB Flash** minimum
- **USB Cable** for initial upload

### Software
- **Arduino IDE** 1.8.19+ or 2.x
- **ESP8266 Board Package** 3.0.0+
- **ArduinoJson Library** 6.21.0+

---

## 🚀 Quick Start

### 1️⃣ Arduino IDE Setup

1. **Install ESP8266 Board Support**
   - Open Arduino IDE
   - Go to `File` → `Preferences`
   - Add to "Additional Board Manager URLs":
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Go to `Tools` → `Board` → `Boards Manager`
   - Search for "esp8266" and install **esp8266 by ESP8266 Community**

2. **Install Required Library**
   - Go to `Tools` → `Manage Libraries`
   - Search for **ArduinoJson** by Benoit Blanchon
   - Install version **6.21.0** or newer

### 2️⃣ Configure and Upload

1. **Clone or download this repository**
   ```bash
   git clone https://github.com/khaoullaaa/OTA_Secure_ESP8266-vs2.git
   ```

2. **Open the sketch**
   - Open `OTA_Secure/OTA_Secure.ino` in Arduino IDE

3. **Configure WiFi and GitHub**
   
   Edit these lines at the top of `OTA_Secure.ino`:
   ```cpp
   const char* WIFI_SSID     = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   
   const char* GITHUB_USER = "khaoullaaa";  // Your GitHub username
   const char* GITHUB_REPO = "OTA_Secure_ESP8266-vs2";  // Your repo name
   ```

4. **Select Board and Port**
   - `Tools` → `Board` → `ESP8266 Boards` → **NodeMCU 1.0 (ESP-12E Module)**
   - `Tools` → `Port` → Select your COM port (e.g., COM5)
   - `Tools` → `Upload Speed` → **921600** (or **115200** if upload fails)
   - `Tools` → `Flash Size` → **4MB (FS:3MB OTA:~512KB)**

5. **Upload the sketch**
   - Click the Upload button (→)
   - Wait for compilation and upload to complete

6. **Check Serial Monitor**
   - Open `Tools` → `Serial Monitor`
   - Set baud rate to **115200**
   - You'll see the device IP address and status

---

## 📁 Project Structure

```
OTA_Secure_ESP8266-vs2/
├── OTA_Secure/
│   └── OTA_Secure.ino          # Main sketch (Arduino IDE compatible)
├── .github/
│   └── workflows/
│       └── ota_workflow.yml    # CI/CD pipeline (Arduino CLI)
├── manifest.json               # Updated automatically by CI
├── AUDIT_REPORT.md             # Security audit findings
└── README.md                   # This file
```

---

## 🔄 Create Your First OTA Release

The included GitHub Actions workflow will automatically:
- ✅ Compile the firmware using Arduino CLI
- ✅ Calculate SHA256 hash
- ✅ Create a GitHub Release with `firmware.bin`
- ✅ Update `manifest.json` in your repo

**To trigger a release:**

```bash
git tag v1.0.0
git push origin v1.0.0
```

**GitHub Actions will:**
1. Compile your sketch with the version number
2. Generate SHA256 hash
3. Create a release at `https://github.com/khaoullaaa/OTA_Secure_ESP8266-vs2/releases`
4. Update `manifest.json` in the main branch

**Your ESP8266 will:**
- Automatically check for updates on boot
- Display "Update available" in the web interface
- Allow one-click installation

---

## 🌐 Web Interface

Once connected to WiFi, access the web interface:

```
http://<device-ip>/
```

**You'll see:**
- Current firmware version
- Device IP address
- Update status
- Three buttons:
  - **Check Updates** - Manually check for new firmware
  - **Install Update** - Download and flash (if available)
  - **Reboot** - Restart the device

---

## 🔧 How It Works

1. **Device boots** → connects to WiFi
2. **Fetches manifest.json** from your GitHub repo:
   ```
   https://raw.githubusercontent.com/{user}/{repo}/main/manifest.json
   ```
3. **Compares versions** - uses semantic versioning (e.g., 1.0.0 vs 1.1.0)
4. **If update available** - shows in web interface
5. **User clicks "Install Update"** (or automatic on boot)
6. **Downloads firmware** from GitHub Release URL
7. **Calculates SHA256** while downloading (streaming verification)
8. **Verifies hash** - if mismatch, aborts and keeps current firmware
9. **Flashes firmware** - writes to flash memory
10. **Reboots** - device restarts with new version

---

## 🔐 Security

- **SHA256 verification** - Firmware integrity checked before flashing
- **HTTPS** - All downloads over encrypted connection  
- **Streaming verification** - Hash calculated during download (memory efficient)
- **No hardcoded certificates** - Uses `setInsecure()` but validates via SHA256
- **Rollback protection** - Only installs newer semantic versions

> **Note:** This uses `client.setInsecure()` which skips TLS certificate verification. Security relies on SHA256 hash verification. For maximum security, implement certificate pinning or CA validation.

---

## 🛠️ Development Workflow

### Making Changes

1. Edit `OTA_Secure/OTA_Secure.ino` in Arduino IDE
2. Test upload via USB
3. Commit and push changes to GitHub

### Releasing New Version

1. **Tag a new version:**
   ```bash
   git tag v1.1.0
   git push origin v1.1.0
   ```

2. **Wait for GitHub Actions** (check the "Actions" tab)

3. **Your devices automatically see the update!**

---

## 📊 Manifest Format

The `manifest.json` is auto-generated by CI:

```json
{
  "version": "v1.0.0",
  "firmware_url": "https://github.com/khaoullaaa/OTA_Secure_ESP8266-vs2/releases/download/v1.0.0/firmware.bin",
  "sha256": "abc123def456..."
}
```

**Fields:**
- `version` - Git tag (e.g., v1.0.0)
- `firmware_url` - Direct download link to firmware.bin
- `sha256` - SHA256 hash of the firmware binary

---

## 🐛 Troubleshooting

### ❌ Upload fails with "Permission Denied"
- **Close Serial Monitor** in Arduino IDE
- Close any other programs using the COM port
- Unplug and replug the USB cable
- Try a different USB port

### ❌ Device doesn't connect to WiFi
- Check SSID and password are correct
- Ensure **2.4GHz WiFi** (ESP8266 doesn't support 5GHz)
- Check Serial Monitor for connection errors
- Move device closer to router

### ❌ OTA update fails
- Verify `manifest.json` exists in your GitHub repo
- Check GitHub Release has `firmware.bin` file
- Verify SHA256 in manifest matches the release
- Check Serial Monitor for detailed error messages
- Ensure stable WiFi connection during download

### ❌ "Not enough space" error
- Firmware binary too large for flash memory
- Select correct Flash Size in `Tools` menu
- Remove unnecessary code/libraries to reduce size

### ❌ Compilation errors
- Ensure **ArduinoJson** library is installed
- Ensure ESP8266 board package is version 3.0.0+
- Check that board is set to **NodeMCU 1.0 (ESP-12E)**

---

## 📝 Version Comparison

The firmware uses **semantic versioning** (MAJOR.MINOR.PATCH):

```
v1.2.3
 │ │ └─ Patch: Bug fixes
 │ └─── Minor: New features (backward compatible)
 └───── Major: Breaking changes
```

**Examples:**
- `1.0.0` → `1.0.1` ✅ Update (patch)
- `1.0.0` → `1.1.0` ✅ Update (minor)
- `1.0.0` → `2.0.0` ✅ Update (major)
- `1.1.0` → `1.0.0` ❌ No update (older)

---

## 💡 Tips

- **Serial Monitor:** Always check Serial Monitor (115200 baud) for debug info
- **IP Address:** Note the device IP from Serial Monitor or check your router
- **WiFi:** Use strong WiFi signal for reliable OTA updates
- **Testing:** Test new firmware with USB upload before creating OTA release
- **Versions:** Use semantic versioning for clear update progression

---

## 📄 License

This project is open source and available for educational purposes.

---

## 🙏 Credits

Built with:
- **ESP8266 Arduino Core** - ESP8266 WiFi support
- **ArduinoJson** by Benoit Blanchon - JSON parsing
- **Arduino CLI** - CI/CD builds
- **BearSSL** - SHA256 verification (included in ESP8266 core)

---

## 👤 Author

**Khaoula**
- GitHub: [@khaoullaaa](https://github.com/khaoullaaa)
- Repository: [OTA_Secure_ESP8266-vs2](https://github.com/khaoullaaa/OTA_Secure_ESP8266-vs2)

---

**⭐ If this helped you, please star the repo!**
