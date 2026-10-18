
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_arduino_version.h>

// ===== WIFI =====
const char* WIFI_SSID = "***************";
const char* WIFI_PASSWORD = "***************";

// ===== SERVER API =====
const char* SERVER_URL = "http://192.168.1.67:1010/api/stats";

// ===== SPRZET =====
TFT_eSPI tft = TFT_eSPI();

#define TFT_BL_PIN 21

#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define TOUCH_CLK  25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
Preferences prefs;

// ===== USTAWIENIA I KOLORY =====
int theme = 0;
int brightness = 200;
int refreshSeconds = 30;
int currentPage = -1;

bool settingsSaved = true;
unsigned long lastRefresh = 0;

uint16_t bgColor = TFT_BLACK;
uint16_t fgColor = TFT_WHITE;
uint16_t accentColor = TFT_CYAN;
uint16_t cardColor = 0x2126;
uint16_t mutedColor = TFT_LIGHTGREY;
uint16_t borderColor = TFT_DARKGREY;

// ===== MENU =====
const char* names[8] = {
  "SERVER", "LAMP",
  "SENSORS", "NETWORK",
  "STORAGE", "ROOM",
  "TOOLS", "SETTINGS"
};

const char* descriptions[8] = {
  "Home server control",
  "Light control",
  "Temperature sensors",
  "Network status",
  "Disk information",
  "Room information",
  "Useful tools",
  "Panel settings"
};


uint16_t menuColors[8];

struct ThemePalette {
  uint16_t bg;
  uint16_t fg;
  uint16_t accent;
  uint16_t card;
  uint16_t muted;
  uint16_t border;
  uint16_t tiles[8];
};

const ThemePalette palettes[8] = {
  // DARK CYAN
  {0x1082, TFT_WHITE, 0x07FF, 0x2126, 0xBDF7, 0x4A69,
   {0x13B4,0x13AF,0x1B6F,0x1B5A,0x2B6D,0x1A8D,0x23B0,0x294A}},

  // LIGHT
  {0xFFFF, 0x1082, 0x245F, 0xDEFB, 0x7BEF, 0xBDF7,
   {0xBDF7,0xD6BA,0xC6DF,0xBDD7,0xE71C,0xCE79,0xD69A,0xAD7F}},

  // OCEAN
  {0x000D, TFT_WHITE, 0x07FF, 0x0193, 0x7DDF, 0x04BF,
   {0x0193,0x03EF,0x0255,0x045A,0x0356,0x04B5,0x02D5,0x03B0}},

  // FOREST
  {0x1104, TFT_WHITE, 0x47F3, 0x2248, 0xAEF5, 0x3DCA,
   {0x2D46,0x3DCA,0x1D68,0x35A8,0x2E09,0x45EB,0x2608,0x366B}},

  // SUNSET
  {0x20F3, TFT_WHITE, 0xFD20, 0x39A6, 0xD6BA, 0x8A69,
   {0xA2C6,0xFC60,0xC2E8,0xE34A,0xF3AA,0xB28A,0xDAA8,0xFAE0}},

  // VIOLET
  {0x18CA, TFT_WHITE, 0xC7BF, 0x312F, 0xC618, 0x6B5A,
   {0x49F3,0x7277,0x79F4,0x9A16,0x51D3,0x8A16,0x69F5,0xA2B7}},

  // MATRIX
  {0x0000, 0xDFFF, 0x07E0, 0x10A3, 0x87F0, 0x2D67,
   {0x0320,0x03E0,0x0540,0x05E0,0x0A60,0x0BE0,0x14A0,0x17E0}},

  // ROSE
  {0x20D3, TFT_WHITE, 0xFB7D, 0x4928, 0xE6B6, 0x8A30,
   {0xA8CA,0xD2B0,0xE2F3,0xF313,0xC291,0xEAF5,0xB30D,0xFA75}}
};

void setThemeColors() {
  theme = constrain(theme, 0, 7);

  const ThemePalette &p = palettes[theme];

  bgColor = p.bg;
  fgColor = p.fg;
  accentColor = p.accent;
  cardColor = p.card;
  mutedColor = p.muted;
  borderColor = p.border;

  for (int i = 0; i < 8; i++) {
    menuColors[i] = p.tiles[i];
  }
}

// ===== PODSWIETLENIE =====
void setBrightness(int value) {
  brightness = constrain(value, 30, 255);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(TFT_BL_PIN, brightness);
#else
  ledcWrite(0, brightness);
#endif
}

void initBrightness() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(TFT_BL_PIN, 5000, 8);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL_PIN, 0);
#endif

  setBrightness(brightness);
}

// ===== DOTYK =====
int touchX(TS_Point p) {
  return 239 - constrain(map(p.x, 618, 3335, 0, 239), 0, 239);
}

int touchY(TS_Point p) {
  return 319 - constrain(map(p.y, 815, 3540, 0, 319), 0, 319);
}

// ===== ZAPIS / ODCZYT USTAWIEN =====
void loadSettings() {
  prefs.begin("homepanel", true);

  theme = prefs.getInt("theme", 0);
  brightness = prefs.getInt("bright", 200);
  refreshSeconds = prefs.getInt("refresh", 30);

  prefs.end();

  theme = constrain(theme, 0, 7);
  brightness = constrain(brightness, 30, 255);

  if (refreshSeconds != 0 &&
      refreshSeconds != 10 &&
      refreshSeconds != 30 &&
      refreshSeconds != 60) {
    refreshSeconds = 30;
  }

  setThemeColors();
}

void saveSettings() {
  prefs.begin("homepanel", false);

  prefs.putInt("theme", theme);
  prefs.putInt("bright", brightness);
  prefs.putInt("refresh", refreshSeconds);

  prefs.end();

  settingsSaved = true;
}

// ===== PRZYCISKI =====
void drawSmallButton(int x, int y, int w, int h,
                     const char* label, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, 6, color);
  tft.drawRoundRect(x, y, w, h, 6, accentColor);
  tft.setTextColor(TFT_WHITE, color);
  tft.setTextSize(2);

  int textWidth = strlen(label) * 12;
  tft.setCursor(x + (w - textWidth) / 2, y + (h - 16) / 2);
  tft.print(label);
}

void drawBottomButtons(bool showRefresh) {
  uint16_t leftColor = showRefresh ? 0x245B : 0x2929;

  tft.fillRoundRect(8, 270, 108, 38, 8, leftColor);
  tft.drawRoundRect(8, 270, 108, 38, 8,
                    showRefresh ? TFT_CYAN : TFT_DARKGREY);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, leftColor);
  tft.setCursor(15, 281);

  if (showRefresh) {
    tft.print("REFRESH");
  } else {
    tft.print("---");
  }

  tft.fillRoundRect(124, 270, 108, 38, 8, cardColor);
  tft.drawRoundRect(124, 270, 108, 38, 8, borderColor);

  tft.setTextColor(fgColor, cardColor);
  tft.setCursor(143, 281);
  tft.print("< BACK");
}

// ===== MENU GLOWNE =====
void drawButton(int i) {
  int col = i % 2;
  int row = i / 2;

  int x = col * 120 + 4;
  int y = 48 + row * 66;

  tft.fillRoundRect(x, y, 112, 60, 8, menuColors[i]);
  tft.drawRoundRect(x, y, 112, 60, 8, accentColor);

  tft.setTextColor(TFT_WHITE, menuColors[i]);
  tft.setTextSize(1);
  tft.setCursor(x + 8, y + 8);
  tft.printf("%02d", i + 1);

  tft.setTextSize(2);
  tft.setCursor(x + 8, y + 27);
  tft.print(names[i]);
}

void drawMenu() {
  tft.fillScreen(bgColor);

  tft.setTextColor(accentColor, bgColor);
  tft.setTextSize(2);
  tft.setCursor(8, 6);
  tft.print("HOME PANEL");

  tft.setTextColor(fgColor, bgColor);
  tft.setTextSize(1);
  tft.setCursor(8, 29);
  tft.print("ESP32 CONTROL CENTER");

  for (int i = 0; i < 8; i++) {
    drawButton(i);
  }
}

// ===== WIFI =====
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Laczenie z Wi-Fi");
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi OK, IP ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Nie udalo sie polaczyc z Wi-Fi");
  }
}

// ===== STATYSTYKI SERWERA =====
bool serverOnline = false;
String serverHostname = "docker-apps";
String serverUptime = "--";
int ramPercent = 0;
int diskPercent = 0;
int dockerRunning = 0;
int dockerTotal = 0;
float load1 = 0.0;

bool fetchServerStats() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 5000) {
      delay(250);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    serverOnline = false;
    return false;
  }

  HTTPClient http;
  http.setConnectTimeout(3000);
  http.setTimeout(4000);

  if (!http.begin(SERVER_URL)) {
    serverOnline = false;
    return false;
  }

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("API HTTP error: %d\n", httpCode);
    http.end();
    serverOnline = false;
    return false;
  }

  JsonDocument doc;
  DeserializationError error =
      deserializeJson(doc, http.getString());

  http.end();

  if (error) {
    Serial.println("Blad odczytu JSON");
    serverOnline = false;
    return false;
  }

  serverHostname = doc["hostname"] | "server";
  serverUptime = doc["uptime"] | "unknown";
  ramPercent = doc["ram_percent"] | 0;
  diskPercent = doc["disk_percent"] | 0;
  dockerRunning = doc["docker_running"] | 0;
  dockerTotal = doc["docker_total"] | 0;
  load1 = doc["load_1m"] | 0.0;

  serverOnline = true;
  lastRefresh = millis();

  Serial.println("Statystyki serwera OK");
  return true;
}

// ===== KARTY STATYSTYK =====
void drawStatCard(int x, int y, int w, int h,
                  const char* label, String value,
                  uint16_t accent) {
  tft.fillRoundRect(x, y, w, h, 7, cardColor);
  tft.drawRoundRect(x, y, w, h, 7, borderColor);

  tft.setTextSize(1);
  tft.setTextColor(mutedColor, cardColor);
  tft.setCursor(x + 8, y + 7);
  tft.print(label);

  tft.setTextSize(2);
  tft.setTextColor(accent, cardColor);
  tft.setCursor(x + 8, y + 24);
  tft.print(value);
}

// ===== STRONA SERVER =====
void drawServerPage() {
  tft.fillScreen(bgColor);

  tft.setTextColor(accentColor, bgColor);
  tft.setTextSize(2);
  tft.setCursor(10, 15);
  tft.print("SERVER");

  uint16_t statusColor = serverOnline ? TFT_GREEN : TFT_RED;

  tft.fillCircle(18, 45, 4, statusColor);
  tft.setTextColor(statusColor, bgColor);
  tft.setTextSize(1);
  tft.setCursor(29, 40);
  tft.print(serverOnline ? "ONLINE" : "OFFLINE");

  tft.setTextColor(fgColor, bgColor);
  tft.setCursor(105, 40);
  tft.print(serverHostname);

  tft.drawFastHLine(10, 56, 220, borderColor);

  if (serverOnline) {
    drawStatCard(8, 65, 108, 55,
                 "RAM USED", String(ramPercent) + "%",
                 accentColor);

    drawStatCard(124, 65, 108, 55,
                 "DISK USED", String(diskPercent) + "%",
                 TFT_YELLOW);

    drawStatCard(8, 128, 108, 55,
                 "DOCKER",
                 String(dockerRunning) + "/" + String(dockerTotal),
                 TFT_GREEN);

    drawStatCard(124, 128, 108, 55,
                 "LOAD 1 MIN", String(load1, 2),
                 TFT_MAGENTA);

    tft.setTextColor(mutedColor, bgColor);
    tft.setTextSize(1);
    tft.setCursor(10, 200);
    tft.print("UPTIME:");

    tft.setTextColor(fgColor, bgColor);
    tft.setCursor(10, 215);
    tft.print(serverUptime.substring(0, 30));
  } else {
    tft.setTextColor(TFT_RED, bgColor);
    tft.setTextSize(2);
    tft.setCursor(15, 100);
    tft.print("API UNAVAILABLE");

    tft.setTextColor(mutedColor, bgColor);
    tft.setTextSize(1);
    tft.setCursor(15, 130);
    tft.print("Check Wi-Fi and API");

    tft.setCursor(15, 145);
    tft.print("192.168.1.67:1010");
  }

  drawBottomButtons(true);
}

// ===== STRONA SETTINGS =====
const char* themeNames[8] = {
  "DARK CYAN",
  "LIGHT",
  "OCEAN",
  "FOREST",
  "SUNSET",
  "VIOLET",
  "MATRIX",
  "ROSE"
};

void drawSettingsPage() {
  tft.fillScreen(bgColor);

  tft.setTextColor(accentColor, bgColor);
  tft.setTextSize(2);
  tft.setCursor(10, 12);
  tft.print("SETTINGS");

  tft.drawFastHLine(10, 38, 220, borderColor);

  // Motyw
  tft.setTextColor(fgColor, bgColor);
  tft.setTextSize(2);
  tft.setCursor(10, 49);
  tft.print("THEME");

  tft.fillRoundRect(105, 43, 125, 34, 6, cardColor);
  tft.drawRoundRect(105, 43, 125, 34, 6, accentColor);
  tft.setTextColor(accentColor, cardColor);
  tft.setCursor(115, 52);
  tft.print(themeNames[theme]);
  tft.setCursor(207, 52);
  tft.print(">");

  // Jasnosc
  tft.setTextColor(fgColor, bgColor);
  tft.setCursor(10, 94);
  tft.print("BRIGHTNESS");

  drawSmallButton(10, 119, 42, 34, "-", cardColor);
  drawSmallButton(188, 119, 42, 34, "+", cardColor);

  // Pasek jasnosci
  tft.drawRoundRect(61, 127, 116, 18, 4, borderColor);
  int fillWidth = map(brightness, 30, 255, 0, 108);
  tft.fillRoundRect(65, 131, fillWidth, 10, 3, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(mutedColor, bgColor);
  tft.setCursor(80, 158);
  tft.printf("%d / 255", brightness);

  // Auto refresh
  tft.setTextSize(2);
  tft.setTextColor(fgColor, bgColor);
  tft.setCursor(10, 183);
  tft.print("AUTO REFRESH");

  tft.fillRoundRect(142, 176, 88, 34, 6, cardColor);
  tft.drawRoundRect(142, 176, 88, 34, 6, accentColor);

  tft.setTextColor(accentColor, cardColor);
  tft.setCursor(151, 185);

  if (refreshSeconds == 0) tft.print("OFF");
  else {
    tft.printf("%ds", refreshSeconds);
  }

  tft.setTextSize(1);
  tft.setTextColor(mutedColor, bgColor);
  tft.setCursor(10, 224);
  tft.print(settingsSaved ? "Settings saved" : "Unsaved changes!");

  // Lewy przycisk zapisuje ustawienia
  tft.fillRoundRect(8, 270, 108, 38, 8, 0x245B);
  tft.drawRoundRect(8, 270, 108, 38, 8, TFT_CYAN);
  tft.setTextColor(TFT_WHITE, 0x245B);
  tft.setTextSize(2);
  tft.setCursor(18, 281);
  tft.print("SAVE");

  // Powrot
  tft.fillRoundRect(124, 270, 108, 38, 8, cardColor);
  tft.drawRoundRect(124, 270, 108, 38, 8, borderColor);
  tft.setTextColor(fgColor, cardColor);
  tft.setCursor(143, 281);
  tft.print("< BACK");
}

// ===== POZOSTALE PODSTRONY =====
void drawPage(int page) {
  if (page == 0) {
    fetchServerStats();
    drawServerPage();
    return;
  }

  if (page == 7) {
    drawSettingsPage();
    return;
  }

  tft.fillScreen(bgColor);

  tft.setTextColor(accentColor, bgColor);
  tft.setTextSize(2);
  tft.setCursor(10, 35);
  tft.print(names[page]);

  tft.drawFastHLine(10, 64, 220, borderColor);

  tft.setTextColor(fgColor, bgColor);
  tft.setTextSize(2);
  tft.setCursor(10, 82);
  tft.print(descriptions[page]);

  tft.setTextColor(TFT_GREEN, bgColor);
  tft.setTextSize(1);
  tft.setCursor(10, 190);
  tft.printf("PAGE %d / 8", page + 1);

  tft.setTextColor(mutedColor, bgColor);
  tft.setCursor(10, 230);
  tft.print("Functions coming soon");

  drawBottomButtons(false);
}

// ===== OBSLUGA DOTYKU SETTINGS =====
void handleSettingsTouch(int x, int y) {
  bool changed = false;

  // Lista motywow
  if (y >= 40 && y <= 82 && x >= 100) {
    theme = (theme + 1) % 8;
    setThemeColors();
    settingsSaved = false;
    changed = true;
  }
  // Jasnosc -
  else if (y >= 115 && y <= 158 && x < 58) {
    brightness = max(30, brightness - 25);
    setBrightness(brightness);
    settingsSaved = false;
    changed = true;
  }
  // Jasnosc +
  else if (y >= 115 && y <= 158 && x >= 180) {
    brightness = min(255, brightness + 25);
    setBrightness(brightness);
    settingsSaved = false;
    changed = true;
  }
  // Auto refresh: OFF -> 10 -> 30 -> 60 -> OFF
  else if (y >= 170 && y <= 214 && x >= 135) {
    if (refreshSeconds == 0) refreshSeconds = 10;
    else if (refreshSeconds == 10) refreshSeconds = 30;
    else if (refreshSeconds == 30) refreshSeconds = 60;
    else refreshSeconds = 0;

    settingsSaved = false;
    changed = true;
  }
  // Zapis
  else if (y >= 265 && y <= 315 && x < 120) {
    saveSettings();
    changed = true;
  }
  // Powrot
  else if (y >= 265 && y <= 315 && x >= 120) {
    currentPage = -1;
    drawMenu();
    return;
  }

  if (changed) {
    drawSettingsPage();
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(2);

  loadSettings();
  initBrightness();

  touchSPI.begin(
    TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS
  );
  touch.begin(touchSPI);

  drawMenu();
  connectWiFi();
}

// ===== LOOP =====
void loop() {
  // Automatyczne odswiezanie statystyk serwera
  if (currentPage == 0 && refreshSeconds > 0 &&
      millis() - lastRefresh >=
      (unsigned long)refreshSeconds * 1000UL) {
    fetchServerStats();
    drawServerPage();
  }

  if (!touch.touched()) {
    delay(10);
    return;
  }

  TS_Point p = touch.getPoint();
  int x = touchX(p);
  int y = touchY(p);

  Serial.printf("X=%d Y=%d\n", x, y);

  if (currentPage == -1) {
    if (y >= 48 && y < 312) {
      int col = x / 120;
      int row = (y - 48) / 66;

      if (col >= 0 && col < 2 &&
          row >= 0 && row < 4) {
        currentPage = row * 2 + col;
        drawPage(currentPage);
      }
    }
  } else if (currentPage == 7) {
    handleSettingsTouch(x, y);
  } else {
    if (y >= 265 && y <= 315) {
      if (x >= 120) {
        currentPage = -1;
        drawMenu();
      } else if (currentPage == 0) {
        fetchServerStats();
        drawServerPage();
      }
    }
  }

  while (touch.touched()) {
    delay(10);
  }

  delay(120);
}
