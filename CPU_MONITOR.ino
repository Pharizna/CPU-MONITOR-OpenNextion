/* CPU_MONITOR v23 — restaurado
   - Alarmas solo por color
   - MQTT: texto gris cálido (0xAAAAAA) + OK/ERROR
   - Actualización MQTT cada 5s
   - Reconnect MQTT cada 5s
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/lgfx_fonts.hpp>

#include "secrets.h"
#include "logo_flat.h"

WiFiClient espClient;
PubSubClient client(espClient);

// ======================================================
// DISPLAY (ST7789 + LovyanGFX)
// ======================================================
#define LCD_SCLK_PIN GPIO_NUM_5
#define LCD_MOSI_PIN GPIO_NUM_1
#define LCD_MISO_PIN GPIO_NUM_2
#define LCD_RST_PIN  GPIO_NUM_NC
#define LCD_DC_PIN   GPIO_NUM_3
#define LCD_CS_PIN   LCD_MISO_PIN
#define LCD_BL_PIN   GPIO_NUM_6

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;

public:
  LGFX(void) {
    {
      auto c = _bus.config();
      c.spi_host   = SPI2_HOST;
      c.spi_mode   = 0;
      c.freq_write = 40000000;
      c.freq_read  = 16000000;
      c.pin_sclk   = LCD_SCLK_PIN;
      c.pin_mosi   = LCD_MOSI_PIN;
      c.pin_miso   = LCD_MISO_PIN;
      c.pin_dc     = LCD_DC_PIN;
      _bus.config(c);
      _panel.setBus(&_bus);
    }
    {
      auto c = _panel.config();
      c.pin_cs   = LCD_CS_PIN;
      c.pin_rst  = LCD_RST_PIN;
      c.panel_width  = 320;
      c.panel_height = 480;
      c.offset_rotation = 4;
      c.readable  = true;
      c.rgb_order  = 0;
      c.dlen_16bit = false;
      _panel.config(c);
    }
    {
      auto c = _light.config();
      c.pin_bl = LCD_BL_PIN;
      c.freq   = 44100;
      _light.config(c);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};

LGFX display;

// ======================================================
// VARIABLES
// ======================================================
float loadValue = -1;
float tempValue = -1;
int   freqValue = -1;

float prevLoad = -999;
float prevTemp = -999;
int   prevFreq = -999;

float lastLoadDraw = -999;
float lastTempDraw = -999;
float lastFreqDraw = -999;

unsigned long lastBlink = 0;
bool blinkState = false;

// MQTT timers
unsigned long lastMQTTCheck = 0;
unsigned long lastMQTTReconnect = 0;

// COLORES
uint32_t colorOK;
uint32_t colorWarn;
uint32_t colorAlarm2;
uint32_t colorAlarm;
uint32_t colorNoData;

// ======================================================
// ESTADOS
// ======================================================
int getStateVar(const char* label, float value) {

  if (value < 0) return 0;

  if (strcmp(label, "LOAD") == 0) {
    if (value  > 99) value = 99;
    if (value <= 10) return 1;
    if (value <= 20) return 2;
    if (value <= 50) return 3;
    return 4;
  }

  if (strcmp(label, "TEMP") == 0) {
    if (value <= 65) return 1;
    if (value <= 75) return 2;
    if (value <= 85) return 3;
    return 4;
  }

  if (strcmp(label, "FREQ") == 0) {
    if (value <= 1300) return 1;
    if (value <= 2000) return 2;
    if (value <= 3000) return 3;
    return 4;
  }

  return 1;
}

// ======================================================
// BOLD SUAVE
// ======================================================
void drawBoldString(const char* txt, int x, int y, uint32_t color, uint32_t bg) {
    display.setTextColor(color, bg);
    display.drawString(txt, x+1, y);
    display.drawString(txt, x,   y+1);
    display.drawString(txt, x, y);
}

// ======================================================
// ICONOS
// ======================================================
float getAnimFactor(int state) {
  switch (state) {
    case 1: return 0.5f;
    case 2: return 1.0f;
    case 3: return 2.0f;
    default: return 0.0f;
  }
}

void drawIconCPU(int x, int y, uint32_t color, int state) {
  float f = getAnimFactor(state);
  int s = 32;

  display.drawRoundRect(x, y, s, s, 6, color);

  int pulse = 0;
  if (state > 0) {
    float phase = (millis() % (int)(1200 / f)) / (1200.0f / f);
    pulse = (int)(sin(phase * 6.28318f) * (1.5f * f));
    pulse = constrain(pulse, -2, 2);
  }

  display.drawRoundRect(x+8-pulse/2, y+8-pulse/2,
                        s-16+pulse, s-16+pulse,
                        4, color);
}

void drawIconTemp(int x, int y, uint32_t color, int state) {
  float f = getAnimFactor(state);

  display.drawRoundRect(x+8, y, 16, 28, 8, color);
  display.drawCircle(x+16, y+32, 10, color);

  int offset = 0;
  if (state > 0) {
    float phase = (millis() % (int)(1400 / f)) / (1400.0f / f);
    offset = (int)(sin(phase * 6.28318f) * (3 * f));
    offset = constrain(offset, -3, 3);
  }

  display.drawLine(x+16, y+4+offset, x+16, y+26+offset, color);
}

void drawIconFreq(int x, int y, uint32_t color, int state) {
  float f = getAnimFactor(state);
  int h = 28;

  int dx = 0;
  if (state > 0) {
    float phase = (millis() % (int)(1000 / f)) / (1000.0f / f);
    dx = (int)(sin(phase * 6.28318f) * (1.5f * f));
    dx = constrain(dx, -3, 3);
  }

  display.drawLine(x+dx,     y+h/2, x+8+dx,  y+6,    color);
  display.drawLine(x+8+dx,   y+6,   x+16+dx, y+h-6,  color);
  display.drawLine(x+16+dx,  y+h-6, x+24+dx, y+10,   color);
  display.drawLine(x+24+dx,  y+10,  x+32+dx, y+h/2,  color);
}

// ======================================================
// PANEL ESTÁTICO — CORREGIDO
// ======================================================
void drawPanelStatic(const char* label, float value,
                     int x, int y, int w, int h) {

  uint32_t panelBG = display.color565(35,35,35);

  display.fillRect(x+55, y+8, w-65, h-16, panelBG);

  int state = getStateVar(label, value);

  uint32_t txtColor =
      (state == 4) ? colorAlarm :
      (state == 3) ? colorAlarm2 :
      (state == 2) ? colorWarn  :
      (state == 1) ? 0xFFFFFF   :
                     colorNoData;

  char buf[16];

  // --- FORMATO CORRECTO ---
  if (value < 0) {
      snprintf(buf, sizeof(buf), "---");
  }
  else if (strcmp(label, "FREQ") == 0) {
      snprintf(buf, sizeof(buf), "%d", (int)value);
  }
  else if (strcmp(label, "LOAD") == 0 && value > 100) {
      snprintf(buf, sizeof(buf), "%d", (int)value);   // ENTERO
  }
  else {
      snprintf(buf, sizeof(buf), "%.1f", value);      // DECIMAL
  }

  display.setFont(&fonts::Font4);

  if (state > 1) {
      drawBoldString(buf, x+60, y+18, txtColor, panelBG);
  } else {
      display.setTextColor(txtColor, panelBG);
      display.drawString(buf, x+60, y+18);
  }
}
// ======================================================
// PANEL ICONO — color sincronizado
// ======================================================
void drawPanelIcon(const char* label, float value,
                   int x, int y,
                   void (*drawIcon)(int,int,uint32_t,int)) {

  int state = getStateVar(label, value);

  uint32_t color =
      (state == 4) ? colorAlarm :
      (state == 3) ? colorAlarm2 :
      (state == 2) ? colorWarn  :
      (state == 1) ? colorOK    :
                     colorNoData;

  uint32_t panelBG = display.color565(35,35,35);

  display.fillRect(x+5, y+5, 44, 40, panelBG);

  drawIcon(x+12, y+12, color, state);
}

// ======================================================
// UI ESTÁTICA
// ======================================================
void drawStaticUI() {

  display.fillScreen(TFT_BLACK);

  display.setFont(&fonts::FreeSansBold18pt7b);
  display.setTextColor(0xFFFFFF, TFT_BLACK);
  display.drawString("CPU MONITOR", 110, 15);

  display.setFont(&fonts::Font2);
  display.setTextColor(display.color565(200, 80, 80), TFT_BLACK);
  display.drawString("nestdisk/cpu/stats", 20, 70);

  display.drawRect(10, 105, 460, 110, 0x555555);

  display.setFont(&fonts::FreeSansBold12pt7b);
  display.setTextColor(0xAAAAAA, TFT_BLACK);

  display.drawString("Load (%)",   20,  125);
  display.drawString("Temp (C)",   175, 125);
  display.drawString("Freq (MHz)", 325, 125);

  uint32_t panelBG = display.color565(35,35,35);

  display.fillRoundRect(21, 151, 118, 58, 8, panelBG);
  display.drawRoundRect(21, 151, 118, 58, 8, display.color565(70,70,70));

  display.fillRoundRect(176, 151, 118, 58, 8, panelBG);
  display.drawRoundRect(176, 151, 118, 58, 8, display.color565(70,70,70));

  display.fillRoundRect(326, 151, 130, 58, 8, panelBG);
  display.drawRoundRect(326, 151, 130, 58, 8, display.color565(70,70,70));

  int logo_w = 240;
  int logo_h = 30;
  int x = (display.width()  - logo_w) / 2;
  int y = display.height() - logo_h - 50;

  display.drawPng(logo_flat_png, logo_flat_png_len, x, y);
}

// ======================================================
// HEARTBEAT
// ======================================================
void drawHeartbeat() {
  unsigned long now = millis();
  if (now - lastBlink > 500) {
    lastBlink = now;
    blinkState = !blinkState;
    display.fillRect(10, 10, 8, 8,
      blinkState ? display.color565(255, 0, 0) : TFT_BLACK);
  }
}

// ======================================================
// MQTT STATUS — detección + dibujo cada 5 segundos
// ======================================================
void updateMQTTStatus() {

  unsigned long now = millis();
  if (now - lastMQTTCheck < 5000) return;
  lastMQTTCheck = now;

  bool mqttOK = client.connected();

  display.fillRect(360, 60, 150, 30, TFT_BLACK);

  uint32_t colorTitle = 0xAAAAAA;
  display.setFont(&fonts::Font2);
  display.setTextColor(colorTitle, TFT_BLACK);
  display.drawString("MQTT:", 360, 70);

  display.setTextColor(mqttOK ? colorOK : colorAlarm, TFT_BLACK);
  display.drawString(mqttOK ? "OK" : "ERROR", 420, 70);
}

// ======================================================
// UPDATE VALUES — con mqttHasData
// ======================================================
void updateValues() {

  int freqRounded = (freqValue < 0) ? -1 : (freqValue / 10) * 10;

  // LOAD
  if (loadValue != prevLoad) {
    drawPanelStatic("LOAD", loadValue, 21, 151, 118, 58);
    prevLoad = loadValue;
  }

  if (fabs(loadValue - lastLoadDraw) >= 5.0f) {
    drawPanelIcon("LOAD", loadValue, 21, 151, drawIconCPU);
    lastLoadDraw = loadValue;
  }

  // TEMP
  if (tempValue != prevTemp) {
    drawPanelStatic("TEMP", tempValue, 176, 151, 118, 58);
    prevTemp = tempValue;
  }

  if (tempValue != lastTempDraw) {
    drawPanelIcon("TEMP", tempValue, 176, 151, drawIconTemp);
    lastTempDraw = tempValue;
  }

  // FREQ
  if (freqRounded != prevFreq) {
    drawPanelStatic("FREQ", freqRounded, 326, 151, 130, 58);
    prevFreq = freqRounded;
  }

  if (fabs(freqRounded - lastFreqDraw) >= 5.0f) {
    drawPanelIcon("FREQ", freqRounded, 326, 151, drawIconFreq);
    lastFreqDraw = freqRounded;
  }
}

// ======================================================
// MQTT CALLBACK
// ======================================================
void callback(char* topic, byte* payload, unsigned int length) {
  String json;
  for (unsigned int i = 0; i < length; i++) json += (char)payload[i];

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, json)) return;

  const char* sLoad  = doc["load"];
  const char* sTemp  = doc["cpu_temp"];
  const char* sFreq  = doc["freq"];

  loadValue = sLoad ? atof(sLoad) : -1;
  tempValue = sTemp ? atof(sTemp) : -1;
  freqValue = sFreq ? atoi(sFreq) : -1;
}

// ======================================================
// MQTT RECONNECT
// ======================================================
void reconnectMQTT() {

  unsigned long now = millis();
  if (now - lastMQTTReconnect < 5000) return;
  lastMQTTReconnect = now;

  if (client.connected()) return;

  if (client.connect("esp32s3-client", MQTT_USER, MQTT_PASS)) {
    client.subscribe("nestdisk/cpu/stats");
  }
}

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  display.init();
  display.setRotation(1);
  display.setBrightness(200);
  display.setColorDepth(16);

  colorOK     = 0x7FFF00;
  colorWarn   = 0xFFFF00;
  colorAlarm2 = 0xFFA500;
  colorAlarm  = 0xFF0000;
  colorNoData = 0x00FFFF;

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(200);

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  delay(500);
  drawStaticUI();
  updateMQTTStatus();
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  static unsigned long lastFrame = 0;
  unsigned long now = millis();
  if (now - lastFrame < 33) {
    reconnectMQTT();
    client.loop();
    return;
  }
  lastFrame = now;

  reconnectMQTT();
  client.loop();

  drawHeartbeat();
  updateMQTTStatus();
  updateValues();
}
