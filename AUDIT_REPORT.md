# OTA_Secure_ESP8266 – Deep Audit (Dec 12, 2025)

## What I changed (cleanup + correctness)

- Removed committed WiFi credentials from:
  - `ESP8266_Firmware/OTA_Secure/OTA_Secure.ino`
  - `ESP8266_Firmware/Version1_LED/Version1_LED.ino`
  - `ESP8266_Firmware/Version2_Sensor/Version2_Sensor.ino`
- Deleted generated/unwanted files:
  - `__pycache__/` (bytecode cache)
  - Removed the redundant `GitHub_Actions/` folder (workflow now lives where GitHub expects it)
- Added real GitHub Actions workflow at `/.github/workflows/ota_workflow.yml`
- Fixed `ESP8266_Firmware/platformio.ini` (removed misleading ArduinoOTA/espota upload settings)
- Hardened `generate_sha256.ps1` (correct manifest path, preserve JSON depth, write UTF‑8 without BOM)
- Hardened `local_ota_server.py` (no auto-`pip install`, no dummy firmware, streaming SHA256 + SSLContext)
- Added `requirements.txt` for the local server

## Critical issues found (and why they matter)

### 1) OTA was effectively broken (cert pinning + GitHub download redirects)
- Firmware pinned a fingerprint for `raw.githubusercontent.com`, but the **firmware download URL** points to GitHub Releases, which commonly redirects to `objects.githubusercontent.com`.
- Result: TLS pinning would fail, so updates would fail or behave inconsistently.

Fix applied:
- `OTA_Secure.ino` now downloads the firmware with redirect-follow + TLS insecure, but enforces **strict SHA256 verification while streaming**. This makes updates work reliably while still preventing malicious firmware installation.

Remaining risk:
- A network attacker can still *block* updates (DoS) even if they can’t install a modified firmware.

### 2) “SHA256 verification” existed in docs, not in code
- The firmware previously printed “SHA256 will be verified” but never computed or checked it.

Fix applied:
- Implemented streaming SHA256 in `performRealOTA()` and aborts if mismatch.

### 3) Versioning was not persistent across reboot (update loop risk)
- `currentVersion` was a RAM string hardcoded to `"1.0.0"` at startup.
- Even after a successful update, reboot would reset the value, making the device think it’s outdated forever.

Fix applied:
- `currentVersion` is now driven by a build-time macro `FW_VERSION`.
- CI now compiles with `-DFW_VERSION="${tag}"`.

### 4) Version comparison was incorrect
- Previous logic: `latestVersion != currentVersion`.
- This allows downgrades and breaks on tags like `v1.2.3` vs `1.2.3`.

Fix applied:
- Added semantic version parsing and comparisons:
  - only update when `latest > current`
  - respects `min_version`

### 5) GitHub Actions workflow placement and logic problems
- GitHub only runs workflows from `.github/workflows/`.
- The old workflow lived under `GitHub_Actions/` and wouldn’t run unless copied manually.

Fix applied:
- Created `/.github/workflows/ota_workflow.yml`.
- Workflow now:
  - compiles firmware
  - produces `firmware.bin`
  - computes SHA256
  - creates release assets (firmware + manifest + sha256.txt)
  - updates `ESP8266_Firmware/manifest.json` on `main` so devices fetching the raw manifest see the latest release.

## Security findings (still recommended)

- **Secrets in git history**: even though current files are cleaned, if this repo was ever pushed publicly, the old commits still contain WiFi credentials. Rotate credentials and consider `git filter-repo` to purge history.
- **Unauthenticated web endpoints**: `/update`, `/reboot` can be triggered by anyone on the LAN. Consider adding at least a shared token, Basic Auth, or disabling these endpoints in production.
- **TLS fingerprint pinning is fragile**: certificate fingerprints rotate. For a stronger approach, use CA trust anchors / cert store instead of hard-coded fingerprints.
- **Weak AP password** (`12345678`) when fallback AP mode starts: change it or generate per-device.

## “Unwanted / useless” items to keep removed

- `__pycache__/` (generated)
- `GitHub_Actions/` (redundant once `.github/workflows` exists)
- Avoid committing `.venv/` (now ignored in `.gitignore`)

## Quick sanity checklist (manual)

- Create a tag like `v1.0.1` and push it.
- Verify the GitHub Action:
  - creates a Release
  - uploads `firmware.bin`, `manifest.json`, `sha256.txt`
  - commits an updated `ESP8266_Firmware/manifest.json` to `main`
- Flash the device once, then verify:
  - it reports `FW_VERSION` correctly
  - it detects the new manifest
  - it downloads firmware and passes SHA256 check

