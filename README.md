![Version](https://img.shields.io/badge/Version-1.0_ADV-blue)
![Hardware](https://img.shields.io/badge/Hardware-Cardputer-orange)
![Platform](https://img.shields.io/badge/Platform-M5Stack-red)
![License](https://img.shields.io/badge/License-CC_BY--NC_4.0-red)
[![Boosty](https://img.shields.io/badge/Support-Boosty-orange)](https://boosty.to/zeloksa)

# 📡 WiFi Remote Display ADV (V1.0)

**WiFi Remote Display ADV V1.0** is an ultra-low latency screen mirroring payload tool strictly optimized for the **M5Stack Cardputer ADV**. It seamlessly injects a Python-based UDP streaming engine into a target Windows PC via USB HID and receives high-speed, adaptive-quality desktop video over a local Wi-Fi network. 

*(Note: Remote HID control features, such as remote mouse and keyboard inputs, are planned for future updates!)*

> [!IMPORTANT]
> **Source Code Status:** This project is Open Source. You are welcome to inspect, modify, and contribute to the code on GitHub.
> **Distribution:** Source code and binary (`.bin`) via the **Releases** tab and M5Burner.

> [!NOTE]
> **Transparent Injection & Safety:** During the payload delivery, the Cardputer will visibly type the Python script directly into an open PowerShell window. Do not panic! The code is fully Open Source, and the injection **will pause at the very end**. The script will NOT execute automatically—you have full control to read and verify the code on your screen before manually pressing `ENTER` on the PC keyboard to start the stream.

---

## ⚡ V1.0 Technical Highlights

* **Zero-Install Payload Injection:** Uses the Cardputer as a USB HID keyboard to automatically open PowerShell, download required dependencies (`mss`, `opencv`), and write a raw Python streaming script directly into the target PC.
* **True Adaptive Bitrate (ABR):** Mathematically monitors frame render times and UDP packet drops. If lag is detected, it dynamically downgrades JPEG compression quality on the PC side to maintain FPS, automatically recovering quality when the network stabilizes.
* **Zero-Flicker Direct UI:** Employs a highly optimized screen-drawing algorithm that bypasses heavy RAM buffers. Menu cursors rewrite only changed pixels, saving 64KB of Heap memory and eliminating screen flicker entirely.
* **Hardware Anti-Deadlock:** Implements deep radio module power cycling (`WiFi.mode(WIFI_OFF)`) to bypass the notorious ESP32 hardware bugs that cause infinite freezing during Wi-Fi scans.
* **NVS Wi-Fi Memory:** Securely stores your network credentials in the ESP32's non-volatile storage (NVS). Fully autonomous and requires no SD card.

---

## ⚠️ Important Requirements (Must Read)

Before running the payload, the target PC **MUST** meet the following criteria, or the injection will fail:

1. **Python MUST be Installed:** The target Windows PC (10/11) must have Python 3.x installed and explicitly added to the system `PATH`. The injected script relies on the Python engine to capture and compress the screen.
2. **SAME Wi-Fi Network:** The Cardputer and the target PC **must** be connected to the exact same local Wi-Fi network for the UDP packets to route successfully.
3. **English Keyboard Layout:** The target PC's keyboard layout **MUST** be switched to English before pressing `[ G ]` on the Cardputer. If set to another language, the PowerShell injection will type gibberish and fail.

---

## 🛠 Installation
### Method 1: M5Burner (Recommended)
1. Open **M5Burner**.
2. Search for `WiFi Remote Display ADV` or `Zeloksa`.
3. Select version **V1.0**.
4. Burn to your M5Stack Cardputer.

### Method 2: Manual Flashing
1. Download the source code from GitHub.
2. Ensure you have the `M5Cardputer`, `Preferences`, and `USBHIDKeyboard` libraries installed in your Arduino IDE.
3. Compile and flash to your device.

---

## 🕹 Controls

**System Ready Menu:**
* **[ G ]**: Start Onboarding & Payload Injection.
* **[ DEL ]**: Hard Reset Wi-Fi (Erases saved NVS credentials and reboots).

**During Injection (Onboarding):**
* **[ ESC / \` ]** or **[ DEL ]**: Instantly abort the USB injection and safely release all PC keys.

**During Streaming:**
* **[ = ] / [ - ]**: Zoom In / Zoom Out.
* **[ ; ] / [ . ]**: Pan Camera Up / Down.
* **[ , ] / [ / ]**: Pan Camera Left / Right.
* **[ 0 ] / [ 9 ]**: Increase / Decrease Cardputer Speaker Volume (Beep indicators).
* **[ ESC / \` ]**: Stop stream, kill PC script, and return to Main Menu.

---

## 📖 Operational Guide

### 1. Network Setup
Upon boot, the Cardputer performs a deep radio scan. Select your Wi-Fi network (W/S to navigate, ENTER to select) and input the password. This is saved permanently. If you move to a new location, press `[ DEL ]` in the main menu to clear the memory.

### 2. The Onboarding HUD
Press `[ G ]` to begin the 5-step safety check:
* **Step 1:** Warns you to physically connect the USB.
* **Step 2:** Confirms Python is in `PATH`.
* **Step 3:** Verifies OS compatibility.
* **Step 4:** Forces you to verify the PC is in the English layout.
* **Step 5:** Displays the hotkeys for panning and zooming.

### 3. Payload Delivery & Streaming
Once confirmed, the Cardputer locks its interface and rapidly types out the UDP streaming server script into the target PC's PowerShell. A progress bar tracks the injection. **The injection stops right before execution.** You can review the code on the PC monitor, and when you are ready, press `ENTER` on the PC keyboard to launch the stream. The Cardputer will then automatically catch the `0xAA` JPEG packet headers and begin rendering the live desktop.

---

## ☕ Support the Project
Support the development of advanced diagnostic tools and payloads for the Cardputer ecosystem:
* **[https://boosty.to/zeloksa](https://boosty.to/zeloksa)**

---
*Developed by Engineer Zeloksa. Strictly optimized for Cardputer ADV.*
