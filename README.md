![Version](https://img.shields.io/badge/Version-1.2_ADV-blue)
![Hardware](https://img.shields.io/badge/Hardware-Cardputer-orange)
![Platform](https://img.shields.io/badge/Platform-M5Stack-red)
![License](https://img.shields.io/badge/License-CC_BY--NC_4.0-red)
[![Boosty](https://img.shields.io/badge/Support-Boosty-orange)](https://boosty.to/zeloksa)
[![M5Burner](https://img.shields.io/badge/Web_Flasher-burner.m5stack.com-brightgreen)](https://burner.m5stack.com)

# 📡 WiFi Remote Display ADV (V1.2)

**WiFi Remote Display ADV V1.2** is a low-latency wireless secondary display and screen mirroring tool built for the **M5Stack Cardputer / Cardputer ADV**. It deploys a lightweight Python UDP capture script to a Windows 10/11 PC over USB HID and streams live desktop video to the Cardputer over 2.4GHz Wi-Fi.

> [!IMPORTANT]
> **Source Code Status:** Source-Available for Non-Commercial use. Feel free to inspect, modify, and contribute to the repository.
> **Distribution:** Source code and precompiled `.bin` firmware are available in the **Releases** tab and via the official M5Stack Web Burner at **[burner.m5stack.com](https://burner.m5stack.com)**.

> [!NOTE]
> **Transparent Injection & Manual Start:** During setup, the Cardputer types the Python script directly into a visible PowerShell window. **The script does not execute automatically.** Typing stops at the final line so you can inspect the code on your monitor before pressing <kbd>ENTER</kbd> on your PC keyboard to start streaming.

---

## ⚡ Technical Overview

- **Zero-Install USB HID Deployment:** Acts as a standard USB keyboard to open PowerShell, install required packages (`mss`, `opencv-python`, `numpy`), and write `stream.py` with support for both **US** and **UK** English layouts.
- **Unsharp Mask & Area Downscaling:** Combines `cv2.INTER_AREA` decimation with a fast OpenCV Unsharp Mask filter (`OFF` / `NORM` / `HIGH`) so small desktop text remains readable on the 240x135 screen.
- **Smart Cursor & Smooth Follow Mode:** Captures hardware cursor coordinates via Win32 API (`ctypes`), renders a high-contrast pointer that auto-hides after 1.5s of inactivity, and smoothly tracks the cursor when zoomed in.
- **Locked 15-20 FPS Auto-Quality:** Automatically adjusts JPEG compression between `20%` and `85%` to keep framerate stable between 15 and 20 FPS without overloading the ESP32-S3 UDP buffer.
- **Hardware-Clipped OSD:** Uses display clipping (`setClipRect`) and a dedicated sprite buffer so top-bar notifications never flicker or get overwritten by incoming video frames.
- **Non-Blocking Wi-Fi & NVS Storage:** Runs asynchronous background scanning (`WiFi.scanNetworks(true)`), disables modem sleep (`WiFi.setSleep(false)`) for low UDP jitter, and saves credentials in ESP32 NVS flash.

---

## ⚠️ Requirements

Before starting the onboarding wizard, make sure the target PC meets these 3 conditions:

1. **Python 3 Installed in PATH:** Windows 10/11 must have Python 3.x installed with the **"Add Python to PATH"** option enabled.
2. **Same 2.4GHz Wi-Fi Network:** Both the Cardputer and the PC must be on the same local network (the ESP32-S3 radio operates on 2.4GHz; the PC can be on 2.4GHz, 5GHz, or Ethernet as long as client isolation is disabled on the router).
3. **English Keyboard Layout Active:** Switch your PC input language to **English (US or UK)** before starting injection.

---

## 🛠 Installation

### Method 1: M5Stack Web Burner (Recommended)
M5Burner is now available directly in your browser without installing desktop software:
1. Connect your M5Stack Cardputer to your PC via USB-C.
2. Open **[burner.m5stack.com](https://burner.m5stack.com)** in Chrome, Edge, or Opera (WebSerial support required).
3. Search for **`WiFi Remote Display ADV`** or **`Zeloksa`**.
4. Select **V1.2**, connect your COM port, and click **Burn**.

### Method 2: Manual Compilation (Arduino IDE)
1. Download `WiFiRemoteDisplayADV.ino` from this repository.
2. Install the **ESP32 Arduino Core (3.x)** and the `M5Cardputer` library in Arduino IDE.
3. Select board **M5Cardputer**, set USB Mode to **USB-OTG (TinyUSB)**, and upload.

---

## 🕹 Controls

### Network Setup & Menus
| Key | Action |
| :--- | :--- |
| <kbd>W</kbd> / <kbd>S</kbd> or <kbd>;</kbd> / <kbd>.</kbd> | Scroll through scanned Wi-Fi networks |
| <kbd>ENTER</kbd> | Select network / Confirm password / Next step |
| <kbd>ESC</kbd> (<kbd>`</kbd>) | Cancel password input / Abort USB injection |
| <kbd>G</kbd> | Launch 5-Step Onboarding & Payload Injection |
| <kbd>DEL</kbd> | Wipe saved Wi-Fi credentials from NVS and reboot |

### Live Stream Controls
| Key | Action |
| :--- | :--- |
| <kbd>=</kbd> / <kbd>-</kbd> | Zoom In / Zoom Out (up to 20x) |
| <kbd>SPACE</kbd> | Instant Zoom Reset (`1x` full screen) |
| <kbd>;</kbd> <kbd>.</kbd> <kbd>,</kbd> <kbd>/</kbd> | Pan camera Up, Down, Left, Right |
| <kbd>F</kbd> | Toggle **Follow Mouse** mode (auto-tracks cursor when zoomed) |
| <kbd>1</kbd> | Cycle Sharpness filter (`NORM` -> `HIGH` -> `OFF`) |
| <kbd>P</kbd> | Pause / Freeze current frame on screen |
| <kbd>B</kbd> | Cycle display backlight brightness (`25%` / `50%` / `75%` / `100%`) |
| <kbd>M</kbd> | Switch active PC monitor (multi-display setups) |
| <kbd>I</kbd> | Show live stream telemetry (`FPS`, `Quality %`, `Wi-Fi RSSI dBm`) |
| <kbd>0</kbd> / <kbd>9</kbd> | Increase / Decrease speaker beep volume |
| <kbd>ESC</kbd> (<kbd>`</kbd>) / <kbd>DEL</kbd> | Stop stream, terminate PC script, and return to Ready screen |

> [!TIP]
> **First-Zoom Prompt:** The first time you press <kbd>=</kbd> to zoom in during a session, an on-screen prompt will ask if you want to enable automatic cursor tracking (<kbd>ENTER</kbd> for Yes, <kbd>ESC</kbd> for No). You can toggle this anytime with <kbd>F</kbd>.

---

## 📖 Quick Start Guide

1. **Connect to Wi-Fi:** Turn on the Cardputer, select your 2.4GHz network from the live list, and type your password.
2. **Run Onboarding:** Press <kbd>G</kbd> on the `SYSTEM READY` screen and follow the 5 on-screen prompts (including selecting US  <kbd>S</kbd> or UK <kbd>K</kbd> keyboard layout in Step 4).
3. **Start the Stream:** Wait for the progress bar to finish typing the script into PowerShell, then press <kbd>ENTER</kbd> on your **PC keyboard** to start streaming.

---

## 🆕 Changelog: What's New in V1.2

- **Auto-Hiding Cursor & Smooth Follow (<kbd>F</kbd>):** Renders the PC mouse pointer on the Cardputer screen, hides it automatically after 1.5 seconds of inactivity, and smoothly pans the zoomed camera to follow the cursor.
- **First-Zoom Tracking Prompt:** Added an on-screen prompt when zooming in for the first time to quickly enable or skip automatic cursor tracking.
- **Text Sharpening Filter (<kbd>1</kbd>):** Added a 3-mode sharpness toggle (`NORM` / `HIGH` / `OFF`) so small desktop fonts and icons stay crisp on the 240x135 display.
- **Multi-Monitor Support (<kbd>M</kbd>):** Switch between all connected monitors on the host PC directly from the Cardputer keyboard.
- **Frame Pause (<kbd>P</kbd>), Brightness (<kbd>B</kbd>) & Quick Reset (<kbd>SPACE</kbd>):** Freeze the live image to read text comfortably, cycle screen backlight brightness on the fly, and reset zoom back to `1x` with a single key.
- **Live Telemetry (<kbd>I</kbd>) & Solid OSD:** Check real-time `FPS`, `Quality %`, and `Wi-Fi RSSI` on demand via the top notification bar without screen flicker.

---

## ☕ Support the Project
If you find this project useful, you can support future Cardputer tools and updates here:
- **[Boosty — Engineer Zeloksa](https://boosty.to/zeloksa)**

---
*Developed by Engineer Zeloksa. Built for M5Stack Cardputer & Cardputer ADV.*
