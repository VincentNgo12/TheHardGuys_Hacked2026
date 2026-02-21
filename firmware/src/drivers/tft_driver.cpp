// src/drivers/tft_driver.cpp
#include "tft_driver.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <math.h>

// ─────────────────────────────────────────────────────────
bool TFTDriver::begin() {
    tft.begin();
    tft.setRotation(1); // Landscape
    tft.fillScreen(COL_BG);
    return true;
}

void TFTDriver::clearScreen(uint16_t color) {
    tft.fillScreen(color);
}

// ─────────────────────────────────────────────────────────
//  SPLASH
// ─────────────────────────────────────────────────────────
void TFTDriver::drawSplash() {
    tft.fillScreen(COL_BG);
    tft.setTextColor(COL_ACCENT);
    tft.setTextSize(3);
    tft.setCursor(60, 80);
    tft.print("SMART BAND");

    tft.setTextColor(COL_SUBTEXT);
    tft.setTextSize(1);
    tft.setCursor(88, 120);
    tft.print("ESP32 Feather V2  v1.0");
    tft.setCursor(112, 135);
    tft.print("Initializing...");

    for (int i = 0; i <= 200; i += 4) {
        tft.fillRect(60, 160, i, 6, COL_ACCENT);
        delay(15);
    }
}

// ─────────────────────────────────────────────────────────
//  HOME SCREEN
// ─────────────────────────────────────────────────────────
void TFTDriver::drawHomeScreen_BG() {
    tft.fillScreen(COL_BG);

    // Status bar
    drawStatusBar();

    tft.setTextColor(COL_SUBTEXT);
    tft.setTextSize(1);
    tft.setCursor(8, 92);
    tft.print("SMART BAND  v1.0");

    tft.fillRect(8, 105, 140, 2, COL_ACCENT);

    // Sidebar buttons
    drawAppButton(262,  30, 50, 50, COL_CARD, "~T",  "ENV");
    drawAppButton(262,  90, 50, 50, COL_CARD, "CO2", "AIR");
    drawAppButton(262, 150, 50, 50, COL_CARD, "HR",  "HEART");
}

void TFTDriver::updateHomeScreen(const DisplayData &d, bool heartBeatTick) {
    // Time placeholder (RTC not implemented — shows uptime)
    uint32_t s   = millis() / 1000;
    uint8_t  hh  = (s / 3600) % 24;
    uint8_t  mm  = (s / 60) % 60;
    uint8_t  ss  = s % 60;

    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hh, mm);
    tft.fillRect(8, 26, 120, 32, COL_BG); // Clear time area
    tft.setTextColor(COL_TEXT);
    tft.setTextSize(4);
    tft.setCursor(8, 26);
    tft.print(timeBuf);

    char secBuf[3];
    snprintf(secBuf, sizeof(secBuf), "%02d", ss);
    tft.fillRect(8, 74, 30, 16, COL_BG); // Clear seconds area
    tft.setTextColor(COL_ACCENT);
    tft.setTextSize(2);
    tft.setCursor(8, 74);
    tft.print(secBuf);

    // Widgets
    drawHeartWidget(d.heartRate, heartBeatTick);
    drawEnvCard(d.eCO2, d.eTVOC, d.temperature, d.humidity, d.spo2);

    // Compass (heading from accel as placeholder)
    drawCompassWidget(d.accelX * 10.0f);
}

// ─────────────────────────────────────────────────────────
//  ENV SCREEN
// ─────────────────────────────────────────────────────────
void TFTDriver::drawEnvScreen_BG() {
    tft.fillScreen(COL_BG);

    // Header
    tft.fillRect(0, 0, SCREEN_W, 22, COL_CARD);
    tft.setTextColor(COL_ACCENT);
    tft.setTextSize(1);
    tft.setCursor(4, 7);
    tft.print("< HOME");
    tft.setTextColor(COL_TEXT);
    tft.setCursor(100, 7);
    tft.print("ENVIRONMENT  &  AIR QUALITY");

    tft.setTextColor(COL_SUBTEXT); tft.setTextSize(1);
    tft.setCursor(233, 200); tft.print("Send 'H'");
    tft.setCursor(233, 210); tft.print("to return");
}

void TFTDriver::updateEnvScreen(const DisplayData &d) {
    // Gauges
    drawGaugeArc(58,  105, 50, -10, 50,   d.temperature,   COL_AMBER,  "TEMP", "C");
    drawGaugeArc(175, 105, 50,   0, 100,  d.humidity,      COL_ACCENT, "HUM",  "%");

    uint16_t co2col  = (d.eCO2  < 800) ? COL_GREEN : (d.eCO2  < 1500) ? COL_AMBER : COL_RED;
    uint16_t tvoccol = (d.eTVOC < 220) ? COL_GREEN : (d.eTVOC < 660)  ? COL_AMBER : COL_RED;

    drawGaugeArc(58,  200, 34, 400,   2000, (float)d.eCO2,  co2col,  "CO2",  "ppm");
    drawGaugeArc(175, 200, 34,   0,   1000, (float)d.eTVOC, tvoccol, "TVOC", "ppb");

    // Value overlays
    char buf[12];
    tft.fillRect(34, 97, 50, 16, COL_BG); // Clear temp value
    tft.setTextColor(COL_TEXT); tft.setTextSize(2);
    snprintf(buf, sizeof(buf), "%.1f", d.temperature);
    tft.setCursor(34, 97); tft.print(buf);

    tft.fillRect(152, 97, 40, 16, COL_BG); // Clear humidity value
    snprintf(buf, sizeof(buf), "%.0f", d.humidity);
    tft.setCursor(152, 97); tft.print(buf);

    tft.fillRect(38, 197, 40, 8, COL_BG); // Clear co2 value
    tft.setTextColor(co2col);  tft.setTextSize(1);
    snprintf(buf, sizeof(buf), "%d", d.eCO2);
    tft.setCursor(38, 197); tft.print(buf);

    tft.fillRect(154, 197, 40, 8, COL_BG); // Clear tvoc value
    tft.setTextColor(tvoccol);
    snprintf(buf, sizeof(buf), "%d", d.eTVOC);
    tft.setCursor(154, 197); tft.print(buf);

    // AQI badge
    const char *aqiLabel = (d.eCO2 < 800) ? "GOOD" : (d.eCO2 < 1500) ? "MOD." : "POOR";
    uint16_t aqiColor    = (d.eCO2 < 800) ? COL_GREEN : (d.eCO2 < 1500) ? COL_AMBER : COL_RED;
    tft.fillRect(230, 80, 60, 30, COL_BG); // Clear AQI area
    tft.setTextColor(COL_SUBTEXT); tft.setTextSize(1);
    tft.setCursor(240, 80); tft.print("AQI");
    tft.setTextColor(aqiColor); tft.setTextSize(2);
    tft.setCursor(233, 95); tft.print(aqiLabel);
}

// ─────────────────────────────────────────────────────────
//  FALL ALERT OVERLAY
// ─────────────────────────────────────────────────────────
void TFTDriver::drawFallAlert(uint32_t fallAlertStart) {
    tft.fillRoundRect(40, 60, 240, 120, 12, 0x8000);
    tft.drawRoundRect(40, 60, 240, 120, 12, COL_RED);
    tft.drawRoundRect(41, 61, 238, 118, 12, COL_AMBER);

    tft.setTextColor(COL_RED);   tft.setTextSize(3);
    tft.setCursor(148, 70);      tft.print("!");

    tft.setTextColor(COL_RED);   tft.setTextSize(2);
    tft.setCursor(100, 100);     tft.print("FALL DETECTED");

    tft.setTextColor(COL_TEXT);  tft.setTextSize(1);
    tft.setCursor(68, 130);      tft.print("Sudden motion event detected.");
    tft.setCursor(80, 142);      tft.print("Are you okay? Alert in 5s.");

    float progress = (float)(millis() - fallAlertStart) / 5000.0f;
    tft.fillRect(60, 162, 200, 8, COL_CARD);
    tft.fillRect(60, 162, (int16_t)(200 * progress), 8, COL_AMBER);
}

// ─────────────────────────────────────────────────────────
//  STATUS BAR
// ─────────────────────────────────────────────────────────
void TFTDriver::drawStatusBar() {
    tft.fillRect(0, 0, SCREEN_W, 18, COL_CARD);
    drawText(4, 4, "SMART BAND", 1, COL_ACCENT);
    // Battery placeholder
    tft.drawRect(290, 4, 24, 10, COL_SUBTEXT);
    tft.fillRect(314, 6, 3, 6, COL_SUBTEXT);
    tft.fillRect(292, 6, 18, 6, COL_GREEN);
}

// ─────────────────────────────────────────────────────────
//  HEART WIDGET
// ─────────────────────────────────────────────────────────
void TFTDriver::drawHeartWidget(uint8_t bpm, bool beat) {
    int16_t  x = 8, y = 115;
    uint16_t w = 150, h = 50;

    tft.fillRoundRect(x, y, w, h, 6, COL_CARD);
    tft.drawRoundRect(x, y, w, h, 6, beat ? COL_HEARTPULSE : COL_ACCENT2);

    drawHeartIcon(x + 12, y + 8, beat ? COL_HEARTPULSE : COL_SUBTEXT);

    tft.setTextColor(beat ? COL_HEARTPULSE : COL_TEXT);
    tft.setTextSize(3);
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", bpm);
    tft.setCursor(x + 40, y + 8);
    tft.print(buf);

    tft.setTextColor(COL_SUBTEXT); tft.setTextSize(1);
    tft.setCursor(x + 40, y + 36);
    tft.print("bpm  Heart Rate");

    // ECG line
    uint16_t lx = x + 110, ly = y + 28;
    tft.drawLine(lx,      ly,      lx + 6,  ly,      COL_ACCENT);
    tft.drawLine(lx + 6,  ly,      lx + 8,  ly - 12, COL_ACCENT);
    tft.drawLine(lx + 8,  ly - 12, lx + 10, ly + 8,  COL_ACCENT);
    tft.drawLine(lx + 10, ly + 8,  lx + 12, ly,      COL_ACCENT);
    tft.drawLine(lx + 12, ly,      lx + 20, ly,      COL_ACCENT);
}

// ─────────────────────────────────────────────────────────
//  HEART ICON
// ─────────────────────────────────────────────────────────
void TFTDriver::drawHeartIcon(int16_t x, int16_t y, uint16_t color) {
    tft.fillCircle(x + 5,  y + 5, 5, color);
    tft.fillCircle(x + 11, y + 5, 5, color);
    for (int i = 0; i <= 9; i++) {
        tft.drawLine(x + i,      y + 8, x + 8, y + 20 - i, color);
        tft.drawLine(x + 16 - i, y + 8, x + 8, y + 20 - i, color);
    }
}

// ─────────────────────────────────────────────────────────
//  ENV SUMMARY CARD
// ─────────────────────────────────────────────────────────
void TFTDriver::drawEnvCard(uint16_t co2, uint16_t tvoc,
                             float temp, float rh, uint8_t spo2) {
    int16_t  x = 8, y = 170;
    uint16_t w = 248, h = 64;
    tft.fillRoundRect(x, y, w, h, 6, COL_CARD);

    char buf[8];

    // CO2
    uint16_t co2col = (co2 < 800) ? COL_GREEN : (co2 < 1500) ? COL_AMBER : COL_RED;
    tft.setTextColor(co2col); tft.setTextSize(1);
    tft.setCursor(x + 6, y + 6);  tft.print("eCO2");
    tft.setTextSize(2); tft.setCursor(x + 6, y + 18);
    snprintf(buf, sizeof(buf), "%d", co2); tft.print(buf);
    tft.setTextSize(1); tft.print(" ppm");

    // TVOC
    uint16_t tvoccol = (tvoc < 220) ? COL_GREEN : (tvoc < 660) ? COL_AMBER : COL_RED;
    tft.setTextColor(tvoccol);
    tft.setCursor(x + 6, y + 42); tft.print("TVOC");
    tft.setTextSize(2); tft.setCursor(x + 6, y + 50);
    snprintf(buf, sizeof(buf), "%d", tvoc); tft.print(buf);
    tft.setTextSize(1); tft.print(" ppb");

    tft.drawFastVLine(x + 85, y + 4, h - 8, COL_ACCENT2);

    // Temp
    tft.setTextColor(COL_TEXT); tft.setTextSize(1);
    tft.setCursor(x + 92, y + 6); tft.print("TEMP");
    tft.setTextSize(2); tft.setCursor(x + 92, y + 18);
    snprintf(buf, sizeof(buf), "%.1f", temp); tft.print(buf);
    tft.setTextSize(1); tft.print(" C");

    // Humidity
    tft.setTextColor(COL_ACCENT);
    tft.setCursor(x + 92, y + 42); tft.print("HUM");
    tft.setTextSize(2); tft.setCursor(x + 92, y + 50);
    snprintf(buf, sizeof(buf), "%.0f", rh); tft.print(buf);
    tft.setTextSize(1); tft.print(" %");

    tft.drawFastVLine(x + 160, y + 4, h - 8, COL_ACCENT2);

    // SpO2
    tft.setTextColor(COL_HEARTPULSE);
    tft.setCursor(x + 167, y + 6); tft.print("SpO2");
    tft.setTextSize(2); tft.setCursor(x + 167, y + 18);
    snprintf(buf, sizeof(buf), "%d%%", spo2); tft.print(buf);
}

// ─────────────────────────────────────────────────────────
//  APP BUTTON
// ─────────────────────────────────────────────────────────
void TFTDriver::drawAppButton(int16_t x, int16_t y, uint16_t w, uint16_t h,
                               uint16_t bg, const char *icon, const char *label) {
    tft.fillRoundRect(x, y, w, h, 8, bg);
    tft.drawRoundRect(x, y, w, h, 8, COL_ACCENT2);

    tft.setTextColor(COL_ACCENT); tft.setTextSize(2);
    int16_t tw = strlen(icon) * 12;
    tft.setCursor(x + (w - tw) / 2, y + 8); tft.print(icon);

    tft.setTextColor(COL_SUBTEXT); tft.setTextSize(1);
    int16_t lw = strlen(label) * 6;
    tft.setCursor(x + (w - lw) / 2, y + 34); tft.print(label);
}

// ─────────────────────────────────────────────────────────
//  COMPASS WIDGET
// ─────────────────────────────────────────────────────────
void TFTDriver::drawCompassWidget(float heading) {
    int16_t cx = 195, cy = 75, r = 30;
    tft.drawCircle(cx, cy, r,     COL_ACCENT2);
    tft.drawCircle(cx, cy, r - 2, COL_CARD);

    tft.setTextColor(COL_SUBTEXT); tft.setTextSize(1);
    tft.setCursor(cx - 3, cy - r + 2); tft.print("N");
    tft.setCursor(cx - 3, cy + r - 9); tft.print("S");
    tft.setCursor(cx + r - 8, cy - 4); tft.print("E");
    tft.setCursor(cx - r + 2, cy - 4); tft.print("W");

    float rad = heading * PI / 180.0f;
    int16_t nx = cx + (r - 6) * sin(rad);
    int16_t ny = cy - (r - 6) * cos(rad);
    tft.fillCircle(cx, cy, 3, COL_ACCENT);
    tft.drawLine(cx, cy, nx, ny, COL_ACCENT);
    tft.drawLine(cx, cy,
                 cx - (r - 10) * sin(rad),
                 cy + (r - 10) * cos(rad), COL_RED);
}

// ─────────────────────────────────────────────────────────
//  GAUGE ARC
// ─────────────────────────────────────────────────────────
void TFTDriver::drawGaugeArc(int16_t cx, int16_t cy, int16_t r,
                              float minVal, float maxVal, float val,
                              uint16_t color, const char *label, const char *unit) {
    const float START_DEG = 135.0f;
    const float END_DEG   = 405.0f;
    const float SWEEP     = END_DEG - START_DEG;

    val = constrain(val, minVal, maxVal);
    float fraction  = (val - minVal) / (maxVal - minVal);
    float filledEnd = START_DEG + fraction * SWEEP;

    for (float deg = START_DEG; deg <= END_DEG; deg += 2.0f) {
        float rad = deg * PI / 180.0f;
        tft.fillCircle(cx + (int16_t)(r * cos(rad)),
                       cy + (int16_t)(r * sin(rad)), 2, COL_CARD);
    }
    for (float deg = START_DEG; deg <= filledEnd; deg += 2.0f) {
        float rad = deg * PI / 180.0f;
        tft.fillCircle(cx + (int16_t)(r * cos(rad)),
                       cy + (int16_t)(r * sin(rad)), 2, color);
    }
    float endRad = filledEnd * PI / 180.0f;
    tft.fillCircle(cx + r * cos(endRad), cy + r * sin(endRad), 3, color);

    tft.setTextColor(COL_SUBTEXT); tft.setTextSize(1);
    tft.setCursor(cx - (strlen(label) * 6) / 2, cy + 10); tft.print(label);
    tft.setCursor(cx - strlen(unit) * 3,         cy + 20); tft.print(unit);
}

// ─────────────────────────────────────────────────────────
//  TEXT HELPER
// ─────────────────────────────────────────────────────────
void TFTDriver::drawText(int16_t x, int16_t y, const char *text,
                          uint8_t size, uint16_t color, bool centered) {
    tft.setTextColor(color);
    tft.setTextSize(size);
    if (centered) {
        tft.setCursor(x - (strlen(text) * 6 * size) / 2, y);
    } else {
        tft.setCursor(x, y);
    }
    tft.print(text);
}