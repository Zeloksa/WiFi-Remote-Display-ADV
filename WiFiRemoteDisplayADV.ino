#include <M5Cardputer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <Preferences.h> 

const int udpPort = 1234;
String ssid = "";
String password = "";
int speaker_volume = 15; 

WiFiUDP udp;
USBHIDKeyboard Keyboard;
Preferences preferences; 

enum SystemState { BOOT, SETUP, READY, STREAMING };
SystemState currentState = BOOT;

IPAddress pc_ip;
uint16_t pc_port = 0;
unsigned long last_packet_time = 0;
unsigned long last_key_time = 0;

int low_fps_streak = 0;
int high_fps_streak = 0;
unsigned long last_quality_change = 0; 
int current_quality = 50; 

#define MAX_JPG_SIZE 100000  
#define MAX_CHUNKS 120       
#define CHUNK_SIZE 1400      

uint8_t jpg_buffer[MAX_JPG_SIZE];
bool chunks_received[MAX_CHUNKS];
uint32_t current_frame_id = 0;
uint8_t current_total_chunks = 0;
uint8_t chunks_count = 0;
uint32_t total_jpg_size = 0;

unsigned long last_chunk_time = 0; 
unsigned long last_frame_time = 0; 
unsigned long last_good_frame_time = 0; 

M5Canvas osd(&M5Cardputer.Display);
String osd_text = "";
uint32_t osd_color = YELLOW;
unsigned long osd_timer = 0;
float current_zoom = 1.0;

void playSound(int freq, int duration) {
    if (speaker_volume > 0) M5Cardputer.Speaker.tone(freq, duration);
}

void showOSD(String text, uint32_t color = YELLOW) {
    osd_text = text;
    osd_color = color;
    osd_timer = millis();
}

void drawWatermark() {
    M5Cardputer.Display.setTextDatum(bottom_right);
    M5Cardputer.Display.setTextColor(LIGHTGREY);
    M5Cardputer.Display.setTextFont(2); 
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString("Eng:Zeloksa", 235, 130);
    M5Cardputer.Display.setTextFont(1); 
    M5Cardputer.Display.setTextSize(1.5);
    M5Cardputer.Display.setTextDatum(top_left); 
}

void drawMessage(String title, String msg1, String msg2 = "", uint32_t color = BLUE) {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, 240, 25, color);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString(title, 120, 12);
    
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.setCursor(0, 45);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawString(msg1, 120, 45);
    if (msg2 != "") {
        M5Cardputer.Display.drawString(msg2, 120, 65);
    }
    drawWatermark();
}

void showReadyScreen() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, 240, 25, 0x05E0); 
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString("SYSTEM READY", 120, 12);
    
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.setTextColor(LIGHTGREY);
    M5Cardputer.Display.drawString("Network Connected", 120, 32);

    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawString("Open Source on GitHub", 120, 52);

    M5Cardputer.Display.setTextDatum(middle_right);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawString("Press ", 95, 85);
    
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.drawString("[ G ]", 120, 85);
    
    M5Cardputer.Display.setTextDatum(middle_left);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawString(" to Begin", 145, 85);

    M5Cardputer.Display.setTextDatum(bottom_center);
    M5Cardputer.Display.setTextColor(ORANGE);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString("[ DEL ] - Reset WiFi", 120, 115);
    M5Cardputer.Display.setTextSize(1.5); 
    
    M5Cardputer.Display.setTextDatum(top_left);
    drawWatermark();
}

String inputText(String prompt) {
    while(M5Cardputer.Keyboard.keysState().enter) { M5Cardputer.update(); delay(10); }

    String input_buffer = "";
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setCursor(5, 10);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.println("ENTER " + prompt + ":");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("(Press ENTER to confirm)");
    drawWatermark();
    
    int startY = M5Cardputer.Display.getCursorY() + 10;
    M5Cardputer.Display.setCursor(5, startY);

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            bool changed = false;

            if (status.del && input_buffer.length() > 0) {
                input_buffer.remove(input_buffer.length() - 1);
                changed = true;
            } else {
                for (auto key_char : status.word) {
                    input_buffer += key_char;
                    changed = true;
                }
            }

            if (status.enter) {
                while(M5Cardputer.Keyboard.keysState().enter) { M5Cardputer.update(); delay(10); }
                return input_buffer;
            }

            if (changed) {
                M5Cardputer.Display.fillRect(0, startY, 240, 50, BLACK);
                M5Cardputer.Display.setCursor(5, startY);
                M5Cardputer.Display.print(input_buffer);
            }
            delay(150); 
        }
        delay(10);
    }
}

void drawWiFiList(int network_count, int selected, int topIndex) {
    M5Cardputer.Display.fillRect(0, 35, 240, 100, BLACK); 
    for (int i = 0; i < 5 && (topIndex + i) < network_count; ++i) {
        int idx = topIndex + i;
        int y = 38 + (i * 18);
        if (idx == selected) M5Cardputer.Display.fillRect(0, y - 2, 240, 18, DARKGREEN);
        M5Cardputer.Display.setCursor(5, y);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.print(WiFi.SSID(idx));
        M5Cardputer.Display.print(" ("); M5Cardputer.Display.print(WiFi.RSSI(idx)); M5Cardputer.Display.print("dBm)");
    }
}

void updateWiFiCursor(int network_count, int old_sel, int new_sel, int topIndex) {
    if (old_sel >= topIndex && old_sel < topIndex + 5) {
        int y = 38 + ((old_sel - topIndex) * 18);
        M5Cardputer.Display.fillRect(0, y - 2, 240, 18, BLACK);
        M5Cardputer.Display.setCursor(5, y);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.print(WiFi.SSID(old_sel));
        M5Cardputer.Display.print(" ("); M5Cardputer.Display.print(WiFi.RSSI(old_sel)); M5Cardputer.Display.print("dBm)");
    }
    if (new_sel >= topIndex && new_sel < topIndex + 5) {
        int y = 38 + ((new_sel - topIndex) * 18);
        M5Cardputer.Display.fillRect(0, y - 2, 240, 18, DARKGREEN);
        M5Cardputer.Display.setCursor(5, y);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.print(WiFi.SSID(new_sel));
        M5Cardputer.Display.print(" ("); M5Cardputer.Display.print(WiFi.RSSI(new_sel)); M5Cardputer.Display.print("dBm)");
    }
}

String scanAndSelectWiFi() {
    drawMessage("SCANNING...", "Searching for networks...", "Please wait (up to 5s)", BLUE);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(200);
    WiFi.mode(WIFI_OFF); 
    delay(200);
    WiFi.mode(WIFI_STA); 
    delay(200);
    
    int network_count = WiFi.scanNetworks();
    if (network_count == 0 || network_count == -1) {
        drawMessage("NO NETWORKS", "Nothing found or error.", "Press ENTER to rescan", RED);
        while(true) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isPressed() && M5Cardputer.Keyboard.keysState().enter) return "";
            delay(50);
        }
    }

    int selected = 0;
    int topIndex = 0;
    
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, 240, 20, BLUE);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString("SELECT WIFI (W/S, ENTER)", 120, 10);
    
    M5Cardputer.Display.setTextColor(ORANGE);
    M5Cardputer.Display.drawString("Use SAME WiFi as PC!", 120, 27);
    M5Cardputer.Display.setTextDatum(top_left);

    drawWiFiList(network_count, selected, topIndex); 

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            bool moveUp = false, moveDown = false;
            
            for (auto key_char : status.word) {
                if (key_char == 'w' || key_char == 'W' || key_char == ';') moveUp = true;
                if (key_char == 's' || key_char == 'S' || key_char == '.') moveDown = true;
            }
            
            if (moveUp || moveDown) {
                int old_selected = selected;
                if (moveUp) { selected--; if (selected < 0) selected = network_count - 1; }
                if (moveDown) { selected++; if (selected >= network_count) selected = 0; }
                
                if (selected < topIndex || selected >= topIndex + 5) {
                    if (selected < topIndex) topIndex = selected;
                    if (selected >= topIndex + 5) topIndex = selected - 4;
                    drawWiFiList(network_count, selected, topIndex);
                } else {
                    updateWiFiCursor(network_count, old_selected, selected, topIndex);
                }
            }
            
            if (status.enter) { 
                while(M5Cardputer.Keyboard.keysState().enter) { M5Cardputer.update(); delay(10); }
                return WiFi.SSID(selected); 
            }
            delay(120); 
        }
        delay(10);
    }
}

void saveWifiConfig() {
    preferences.begin("wifi_cfg", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", password);
    preferences.end();
}

bool loadWifiConfig() {
    preferences.begin("wifi_cfg", true);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("pass", "");
    preferences.end();
    return (ssid.length() > 0);
}

void sendPCCommand(String cmd) {
    if (pc_port != 0) {
        udp.beginPacket(pc_ip, pc_port);
        udp.print(cmd);
        udp.endPacket();
    }
}

bool waitEnterOrEsc(bool showBlink = true) {
    bool flash = true;
    unsigned long last_flash = millis();
    
    while(M5Cardputer.Keyboard.isPressed()) { M5Cardputer.update(); delay(10); }

    while (true) {
        M5Cardputer.update();
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        
        if (status.enter) return true;
        if (M5Cardputer.Keyboard.isKeyPressed('`') || status.del) return false;

        if (showBlink && (millis() - last_flash > 600)) {
            flash = !flash;
            last_flash = millis();
            
            M5Cardputer.Display.fillRect(0, 115, 140, 20, BLACK);
            
            if (flash) {
                M5Cardputer.Display.setTextDatum(bottom_left);
                M5Cardputer.Display.setTextColor(GREEN);
                M5Cardputer.Display.drawString("[ ENTER: NEXT ]", 5, 133); 
            }
            M5Cardputer.Display.setTextDatum(top_left); 
        }
        delay(10);
    }
}

bool safeDelay(int ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.keysState().del) {
            return false; 
        }
        delay(10);
    }
    return true;
}

bool runOnboarding() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, 240, 25, BLUE);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString("STEP 1: USB", 120, 12);
    
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.drawString("Connect USB to PC", 120, 40);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.drawString("DO NOT proceed", 120, 65);
    M5Cardputer.Display.drawString("if unplugged!", 120, 85);
    M5Cardputer.Display.setTextDatum(top_left);
    drawWatermark();
    if (!waitEnterOrEsc(true)) return false;

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, 240, 25, BLUE);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString("STEP 2: PYTHON", 120, 12);
    
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.setTextColor(ORANGE);
    M5Cardputer.Display.drawString("Python MUST be installed", 120, 40);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawString("and added to PATH", 120, 65);
    M5Cardputer.Display.setTextDatum(top_left);
    drawWatermark();
    if (!waitEnterOrEsc(true)) return false;

    drawMessage("STEP 3: SYSTEM", "Works on Win 10 & 11.", "PowerShell will open", ORANGE);
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.drawString("to install modules.", 120, 85);
    M5Cardputer.Display.setTextDatum(top_left);
    if (!waitEnterOrEsc(true)) return false;

    drawMessage("STEP 4: KEYBOARD", "Is PC layout set to", "ENGLISH?", PURPLE);
    M5Cardputer.Display.setTextDatum(bottom_center);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.drawString("YES [ENTER]  |  NO [ESC]", 120, 110);
    M5Cardputer.Display.setTextDatum(top_left);
    if (!waitEnterOrEsc(false)) return false; 

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, 240, 25, 0x0410); 
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString("STEP 5: CONTROLS", 120, 12);
    
    M5Cardputer.Display.setTextDatum(top_left);
    M5Cardputer.Display.setCursor(5, 35);
    M5Cardputer.Display.setTextColor(LIGHTGREY); M5Cardputer.Display.print("ZOOM: ");
    M5Cardputer.Display.setTextColor(YELLOW);    M5Cardputer.Display.print("[=]");
    M5Cardputer.Display.setTextColor(WHITE);     M5Cardputer.Display.print(" In  ");
    M5Cardputer.Display.setTextColor(YELLOW);    M5Cardputer.Display.print("[-]");
    M5Cardputer.Display.setTextColor(WHITE);     M5Cardputer.Display.print(" Out");
    
    M5Cardputer.Display.setCursor(5, 60);
    M5Cardputer.Display.setTextColor(LIGHTGREY); M5Cardputer.Display.print("MOVE: ");
    M5Cardputer.Display.setTextColor(YELLOW);    M5Cardputer.Display.print("[;]");
    M5Cardputer.Display.setTextColor(WHITE);     M5Cardputer.Display.print(" Up  ");
    M5Cardputer.Display.setTextColor(YELLOW);    M5Cardputer.Display.print("[.]");
    M5Cardputer.Display.setTextColor(WHITE);     M5Cardputer.Display.print(" Dwn");

    M5Cardputer.Display.setCursor(5, 85);
    M5Cardputer.Display.setTextColor(LIGHTGREY); M5Cardputer.Display.print("      "); 
    M5Cardputer.Display.setTextColor(YELLOW);    M5Cardputer.Display.print("[,]");
    M5Cardputer.Display.setTextColor(WHITE);     M5Cardputer.Display.print(" Lft ");
    M5Cardputer.Display.setTextColor(YELLOW);    M5Cardputer.Display.print("[/]");
    M5Cardputer.Display.setTextColor(WHITE);     M5Cardputer.Display.print(" Rgt");

    drawWatermark();
    if (!waitEnterOrEsc(true)) return false;

    return true;
}

bool injectPayload() {
    drawMessage("INJECTING", "DO NOT TOUCH KEYBOARD!", "Opening PowerShell...", RED);
    playSound(2000, 200);

    Keyboard.press(KEY_LEFT_GUI); Keyboard.press('r'); delay(300); Keyboard.releaseAll();
    if (!safeDelay(800)) goto abort_injection;
    
    Keyboard.print("powershell\n"); 
    if (!safeDelay(2000)) goto abort_injection;
    
    drawMessage("DOWNLOADING", "Installing modules...", "Wait a moment.", RED);
    Keyboard.print("pip install mss opencv-python numpy\n"); 
    if (!safeDelay(1500)) goto abort_injection;
    
    {
        String py = String("Set-Content -Path stream.py -Value @\"\n"
                    "import mss, cv2, socket, numpy as np, struct, math, time, sys, os\n"
                    "UDP_IP = '") + WiFi.localIP().toString() + String("'\n"
                    R"=====(UDP_PORT = 1234
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 2097152)
sock.setblocking(False)
sct = mss.mss()
mon = sct.monitors[1]
f_id = 0
z, q, px, py_pos = 1.0, 50, 0.5, 0.5
w, h = mon['width'], mon['height']
while True:
    try:
        while True:
            d, addr = sock.recvfrom(64)
            c = d.decode()
            if c == 'STOP': os._exit(0)
            elif c == 'Z+': z = max(0.05, z - 0.05)
            elif c == 'Z-': z = min(1.0, z + 0.05)
            elif c == 'Q+': q = min(100, q + 10)
            elif c == 'Q-': q = max(10, q - 10)
            elif c == 'U': py_pos = max(0.0, py_pos - z/2)
            elif c == 'D': py_pos = min(1.0, py_pos + z/2)
            elif c == 'L': px = max(0.0, px - z/2)
            elif c == 'R': px = min(1.0, px + z/2)
    except: pass
    img = np.array(sct.grab(mon))
    if z < 1.0:
        cw, ch = int(w * z), int(h * z)
        max_x, max_y = w - cw, h - ch
        cx, cy = int(px * max_x), int(py_pos * max_y)
        img = img[cy : cy + ch, cx : cx + cw]
    img = cv2.resize(img, (240, 135), interpolation=cv2.INTER_LINEAR)
    real_q = int(q * 0.85) if q > 10 else 10
    _, buf = cv2.imencode('.jpg', img, [cv2.IMWRITE_JPEG_QUALITY, real_q, cv2.IMWRITE_JPEG_OPTIMIZE, 1])
    data = buf.tobytes()
    chunks = math.ceil(len(data) / 1400)
    f_id = (f_id + 1) & 0xFFFFFFFF
    for i in range(chunks):
        h_bytes = struct.pack('<B I B B', 0xAA, f_id, chunks, i)
        sock.sendto(h_bytes + data[i*1400:(i+1)*1400], (UDP_IP, UDP_PORT))
    delay_time = 0.035 if q >= 80 else (0.025 if q >= 60 else 0.015)
    time.sleep(delay_time)
"@
python stream.py; exit)=====");

        drawMessage("WRITING SCRIPT", "Please wait...", "[ ESC ] to Abort", RED);
        M5Cardputer.Display.drawRect(20, 85, 200, 20, WHITE);
        
        int payload_length = py.length();
        for(int i = 0; i < payload_length; i++) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.keysState().del) {
                goto abort_injection;
            }

            Keyboard.print(py[i]);
            if (i % 10 == 0) {
                int px_width = (i * 200) / payload_length;
                M5Cardputer.Display.fillRect(20, 85, px_width, 20, GREEN);
            }
            delay(5);
        }
    }

    playSound(3000, 300);
    return true;

abort_injection:
    Keyboard.releaseAll();
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.fillRect(0, 0, 240, 25, RED);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.drawString("CODE PAUSED", 120, 12);
    
    M5Cardputer.Display.setTextDatum(top_center);
    M5Cardputer.Display.drawString("Injection stopped.", 120, 45);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.drawString("Press [ENTER] to restart", 120, 80);
    M5Cardputer.Display.setTextDatum(top_left);
    drawWatermark();
    
    while(M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.keysState().del) { M5Cardputer.update(); delay(10); }
    while(true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.keysState().enter) break;
        delay(10);
    }
    while(M5Cardputer.Keyboard.keysState().enter) { M5Cardputer.update(); delay(10); }
    return false;
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    setCpuFrequencyMhz(240);
    
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(128); 
    M5Cardputer.Display.setTextSize(1.5);
    
    M5Cardputer.Speaker.setVolume(speaker_volume);
    osd.createSprite(240, 20); 

    Keyboard.begin(); 
    USB.begin();

    bool isConnected = false;
    
    WiFi.disconnect(true, true);
    delay(200);
    WiFi.mode(WIFI_OFF);
    delay(200);
    WiFi.mode(WIFI_STA);
    delay(200);

    if (loadWifiConfig()) {
        drawMessage("CONNECTING...", "Saved SSID:", ssid, BLUE); 
        WiFi.begin(ssid.c_str(), password.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) { delay(400); attempts++; }
        if (WiFi.status() == WL_CONNECTED) {
            isConnected = true;
        } else {
            preferences.begin("wifi_cfg", false);
            preferences.clear();
            preferences.end();
        }
    }

    while (!isConnected) {
        ssid = scanAndSelectWiFi();
        if (ssid == "") continue; 
        password = inputText("PASSWORD");
        drawMessage("CONNECTING...", "SSID: " + ssid, "Please wait...", BLUE);
        WiFi.begin(ssid.c_str(), password.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 25) { delay(400); attempts++; }
        if (WiFi.status() == WL_CONNECTED) { 
            isConnected = true; 
            saveWifiConfig(); 
        } else { 
            drawMessage("FAILED", "Wrong password.", "Try again.", RED); 
            delay(2500); 
        }
    }

    currentState = READY;
    showReadyScreen();
    udp.begin(udpPort);
    playSound(2000, 100); delay(100); playSound(3000, 100);
}

void loop() {
    M5Cardputer.update();

    if (currentState == READY) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (status.del) {
            drawMessage("RESETTING...", "Deleting WiFi config...", "Rebooting...", RED);
            preferences.begin("wifi_cfg", false);
            preferences.clear();
            preferences.end();
            
            WiFi.disconnect(true, true);
            delay(500);
            WiFi.mode(WIFI_OFF);
            delay(500);
            
            ESP.restart(); 
        }

        if (M5Cardputer.Keyboard.isKeyPressed('g')) {
            while(udp.parsePacket() > 0) { udp.read(); } 
            
            if (runOnboarding()) {
                if (injectPayload()) {
                    currentState = STREAMING;
                    last_packet_time = millis();
                    last_frame_time = millis(); 
                    last_good_frame_time = millis();
                    last_quality_change = millis();
                    
                    current_quality = 50;
                    current_zoom = 1.0;
                    low_fps_streak = 0;
                    high_fps_streak = 0;
                    
                    M5Cardputer.Display.fillScreen(BLACK);
                    M5Cardputer.Display.fillRect(0, 0, 240, 25, ORANGE);
                    M5Cardputer.Display.setTextColor(BLACK);
                    M5Cardputer.Display.setTextDatum(middle_center);
                    M5Cardputer.Display.drawString("MANUAL START", 120, 12);
                    
                    M5Cardputer.Display.setTextColor(WHITE);
                    M5Cardputer.Display.setTextSize(2);
                    M5Cardputer.Display.drawString("PRESS ENTER", 120, 60);
                    M5Cardputer.Display.setTextSize(1.5);
                    M5Cardputer.Display.setTextColor(YELLOW);
                    M5Cardputer.Display.drawString("ON PC KEYBOARD", 120, 90);
                    M5Cardputer.Display.setTextDatum(top_left);
                    drawWatermark();
                } else {
                    showReadyScreen();
                }
            } else {
                showReadyScreen();
            }
        }
    }

    if (currentState == STREAMING) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (millis() - last_key_time > 100) {
            bool key_pressed = false;

            if (M5Cardputer.Keyboard.isKeyPressed('=')) { sendPCCommand("Z+"); current_zoom = max(0.05f, current_zoom - 0.05f); showOSD("ZOOM: " + String((int)(1.0/current_zoom)) + "x"); key_pressed = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('-')) { sendPCCommand("Z-"); current_zoom = min(1.0f, current_zoom + 0.05f); showOSD(current_zoom >= 1.0f ? "ZOOM OUT" : "ZOOM: " + String((int)(1.0/current_zoom)) + "x"); key_pressed = true; }
            
            if (M5Cardputer.Keyboard.isKeyPressed(';')) { sendPCCommand("U"); showOSD("PAN UP"); key_pressed = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('.')) { sendPCCommand("D"); showOSD("PAN DOWN"); key_pressed = true; }
            if (M5Cardputer.Keyboard.isKeyPressed(',')) { sendPCCommand("L"); showOSD("PAN LEFT"); key_pressed = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('/')) { sendPCCommand("R"); showOSD("PAN RIGHT"); key_pressed = true; }
            
            if (M5Cardputer.Keyboard.isKeyPressed('0')) { 
                speaker_volume = min(255, speaker_volume + 15); 
                M5Cardputer.Speaker.setVolume(speaker_volume); 
                showOSD("SPK VOL: " + String((speaker_volume * 100) / 255) + "%"); 
                key_pressed = true; 
            }
            if (M5Cardputer.Keyboard.isKeyPressed('9')) { 
                speaker_volume = max(0, speaker_volume - 15); 
                M5Cardputer.Speaker.setVolume(speaker_volume); 
                showOSD("SPK VOL: " + String((speaker_volume * 100) / 255) + "%"); 
                key_pressed = true; 
            }

            if (key_pressed) last_key_time = millis();
        }

        if (M5Cardputer.Keyboard.isKeyPressed('`') || status.del) {
            while(M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.keysState().del) {
                M5Cardputer.update(); delay(10);
            }

            drawMessage("EXIT SCRIPT?", "Press ENTER to confirm", "Press ESC (`) to cancel", RED);
            while (true) {
                M5Cardputer.update();
                Keyboard_Class::KeysState confirm_status = M5Cardputer.Keyboard.keysState();
                
                if (confirm_status.enter) {
                    sendPCCommand("STOP");
                    delay(500); 
                    currentState = READY;
                    pc_port = 0; 
                    
                    while(udp.parsePacket() > 0) { udp.read(); }
                    
                    showReadyScreen();
                    
                    while(M5Cardputer.Keyboard.keysState().enter) { M5Cardputer.update(); delay(10); }
                    return; 
                }
                if (M5Cardputer.Keyboard.isKeyPressed('`') || confirm_status.del) {
                    while(M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.keysState().del) {
                        M5Cardputer.update(); delay(10);
                    }
                    break;
                }
                delay(50);
            }
        }

        if (pc_port != 0 && (millis() - last_packet_time > 2000)) {
            currentState = READY;
            pc_port = 0;
            
            while(udp.parsePacket() > 0) { udp.read(); }
            
            M5Cardputer.Display.clear();
            drawMessage("DISCONNECTED", "Stream stopped.", "Press 'G' to inject again", ORANGE);
            playSound(1000, 500);
            return; 
        }

        if (pc_port != 0 && (millis() - last_frame_time > 1500)) {
            if (current_quality > 10 && (millis() - last_quality_change > 2000)) {
                current_quality = max(10, current_quality - 10);
                sendPCCommand("Q-");
                showOSD("CRITICAL FIX: " + String(current_quality) + "%", RED);
                last_quality_change = millis();
                last_frame_time = millis(); 
            }
        }

        if (chunks_count > 0 && (millis() - last_chunk_time > 150)) {
            current_frame_id = 0;
            chunks_count = 0;
            memset(chunks_received, 0, sizeof(chunks_received));
        }

        int udp_packet_size;
        uint8_t udp_rx_buffer[1500];
        
        while ((udp_packet_size = udp.parsePacket()) > 0) {
            last_packet_time = millis();
            last_chunk_time = millis();
            pc_ip = udp.remoteIP();
            pc_port = udp.remotePort();

            udp.read(udp_rx_buffer, udp_packet_size);

            if (udp_rx_buffer[0] == 0xAA) {
                uint32_t f_id;
                memcpy(&f_id, &udp_rx_buffer[1], 4);
                uint8_t total_chunks_in_frame = udp_rx_buffer[5];
                uint8_t current_chunk_idx = udp_rx_buffer[6];

                if (f_id != current_frame_id) {
                    current_frame_id = f_id;
                    current_total_chunks = total_chunks_in_frame;
                    chunks_count = 0;
                    total_jpg_size = 0;
                    memset(chunks_received, 0, sizeof(chunks_received));
                }

                if (current_chunk_idx < MAX_CHUNKS && !chunks_received[current_chunk_idx]) {
                    uint16_t chunk_payload_size = udp_packet_size - 7;
                    uint16_t offset = current_chunk_idx * CHUNK_SIZE;

                    if (offset + chunk_payload_size <= MAX_JPG_SIZE) {
                        memcpy(&jpg_buffer[offset], &udp_rx_buffer[7], chunk_payload_size);
                        chunks_received[current_chunk_idx] = true;
                        chunks_count++;
                        
                        if (offset + chunk_payload_size > total_jpg_size) {
                            total_jpg_size = offset + chunk_payload_size;
                        }

                        if (chunks_count == current_total_chunks) {
                            M5Cardputer.Display.drawJpg(jpg_buffer, total_jpg_size, 0, 0);
                            last_frame_time = millis(); 
                            
                            unsigned long frame_duration = millis() - last_good_frame_time;
                            last_good_frame_time = millis();

                            if (millis() - last_quality_change > 2000) {
                                if (frame_duration > 120) {
                                    low_fps_streak++;
                                    high_fps_streak = 0;
                                } 
                                else if (frame_duration < 45) {
                                    high_fps_streak++;
                                    low_fps_streak = 0;
                                }

                                if (low_fps_streak >= 5) {
                                    if (current_quality > 10) {
                                        current_quality -= 10;
                                        sendPCCommand("Q-");
                                        showOSD("LAG FIX: " + String(current_quality) + "%", RED);
                                        playSound(2000, 50); 
                                        last_quality_change = millis();
                                        low_fps_streak = 0;
                                    }
                                }
                                
                                if (high_fps_streak >= 60) {
                                    if (current_quality < 100) {
                                        current_quality += 5;
                                        sendPCCommand("Q+");
                                        showOSD("SMOOTH: " + String(current_quality) + "%", GREEN);
                                        last_quality_change = millis();
                                        high_fps_streak = 0;
                                    }
                                }
                            }
                            
                            if (millis() - osd_timer < 1500) {
                                osd.fillSprite(BLACK); 
                                osd.setTextColor(osd_color); 
                                osd.setTextDatum(middle_center);
                                osd.drawString(osd_text, 120, 10);
                                osd.pushSprite(0, 0, BLACK); 
                            }
                            break; 
                        }
                    }
                }
            }
        }
    }
}
