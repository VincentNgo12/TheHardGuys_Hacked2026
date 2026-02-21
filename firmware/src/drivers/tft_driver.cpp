// src/drivers/tft_driver.cpp
#include "tft_driver.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <math.h>

// ─────────────────────────────────────────────────────────────────────────────
//  INTERNAL HELPERS
// ─────────────────────────────────────────────────────────────────────────────

// Draw (or erase) a clock hand using a filled polygon approach for thickness
void TFTDriver::_drawHand(float angleDeg, int length, int thickness, uint16_t color)
{
    float rad = angleDeg * PI / 180.0f;
    int x2 = CLK_CX + length * sin(rad);
    int y2 = CLK_CY - length * cos(rad);
    for (int t = -thickness / 2; t <= thickness / 2; t++) {
        float perp = rad + PI / 2.0f;
        int ox = (int)(t * cos(perp));
        int oy = (int)(t * sin(perp));
        tft.drawLine(CLK_CX + ox, CLK_CY + oy, x2 + ox, y2 + oy, color);
    }
}

// Redraw tick marks after erasing a hand swept over them
void TFTDriver::_redrawTicks()
{
    for (int i = 0; i < 60; i++) {
        float rad  = i * 6.0f * PI / 180.0f;
        bool  isHr = (i % 5 == 0);
        int   inner = isHr ? (CLK_R - 11) : (CLK_R - 6);
        int x1 = CLK_CX + (CLK_R - 2) * sin(rad);
        int y1 = CLK_CY - (CLK_R - 2) * cos(rad);
        int x2 = CLK_CX + inner        * sin(rad);
        int y2 = CLK_CY - inner        * cos(rad);
        uint16_t col = isHr ? UI_CYAN : UI_BORDER;
        tft.drawLine(x1, y1, x2, y2, col);
        if (isHr) tft.drawLine(x1 + 1, y1, x2 + 1, y2, col);
    }
}

// Draw the side sensor panel shell (called once from BG draw)
void TFTDriver::_drawPanelShell(int x, int y, int w, int h,
                                 uint16_t accent, const char *label)
{
    tft.fillRoundRect(x, y, w, h, 4, UI_PANEL);
    tft.drawRoundRect(x, y, w, h, 4, accent);
    // 3-px accent bar across top
    tft.fillRect(x + 1, y + 1, w - 2, 3, accent);
    // label
    tft.setTextColor(UI_GRAY);
    tft.setTextSize(1);
    int tw = strlen(label) * 6;
    tft.setCursor(x + (w - tw) / 2, y + 6);
    tft.print(label);
}

// Update only the value inside a panel (clears the value area first)
void TFTDriver::_updatePanelValue(int x, int y, int w,
                                   const char *valStr, const char *unit,
                                   uint16_t color, bool alert)
{
    // value area: y+18 to y+50 (text size 3 = 24px tall)
    tft.fillRect(x + 1, y + 16, w - 2, 34, UI_PANEL);
    tft.setTextColor(alert ? UI_RED : color);
    tft.setTextSize(3);
    int tw = strlen(valStr) * 18;
    tft.setCursor(x + (w - tw) / 2, y + 18);
    tft.print(valStr);
    // unit
    tft.fillRect(x + 1, y + 50, w - 2, 10, UI_PANEL);
    tft.setTextColor(UI_GRAY);
    tft.setTextSize(1);
    tw = strlen(unit) * 6;
    tft.setCursor(x + (w - tw) / 2, y + 52);
    tft.print(unit);
}

// ─────────────────────────────────────────────────────────────────────────────
//  INIT
// ─────────────────────────────────────────────────────────────────────────────
bool TFTDriver::begin()
{
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(UI_BG);
    _prevSecAngle  = -1;
    _prevMinAngle  = -1;
    _prevHourAngle = -1;
    _prevHeading   = -1;
    return true;
}

void TFTDriver::clearScreen(uint16_t color) { tft.fillScreen(color); }

// ─────────────────────────────────────────────────────────────────────────────
//  SPLASH
// ─────────────────────────────────────────────────────────────────────────────
void TFTDriver::drawSplash()
{
    tft.fillScreen(UI_BG);

    // Outer glow ring
    tft.drawCircle(160, 100, 55, UI_BORDER);
    tft.drawCircle(160, 100, 56, UI_CYAN);

    tft.setTextColor(UI_WHITE);
    tft.setTextSize(3);
    int tw = 10 * 18;                        // "SMART BAND" = 10 chars * size-3 width
    tft.setCursor((320 - tw) / 2, 75);
    tft.print("SMART BAND");

    tft.setTextColor(UI_CYAN);
    tft.setTextSize(1);
    tft.setCursor(82, 112);
    tft.print("ESP32 Feather V2  v1.0");
    tft.setCursor(112, 124);
    tft.print("Initializing...");

    // Progress bar
    tft.drawRoundRect(60, 148, 200, 10, 4, UI_BORDER);
    for (int i = 0; i <= 196; i += 4) {
        tft.fillRoundRect(62, 150, i, 6, 3, UI_CYAN);
        delay(15);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  HOME SCREEN – STATIC BACKGROUND  (call once, then use updateHomeScreen)
// ─────────────────────────────────────────────────────────────────────────────
void TFTDriver::drawHomeScreen_BG()
{
    tft.fillScreen(UI_BG);

    // ── Clock face ────────────────────────────────────────────────────────────
    tft.fillCircle(CLK_CX, CLK_CY, CLK_R + 5, UI_CYAN);        // accent rim
    tft.fillCircle(CLK_CX, CLK_CY, CLK_R + 3, UI_CLOCK_RIM);
    tft.fillCircle(CLK_CX, CLK_CY, CLK_R,     UI_CLOCK_FACE);
    _redrawTicks();

    // Hour numbers
    tft.setTextColor(UI_WHITE);
    tft.setTextSize(1);
    const char *hrLbl[] = {"12","1","2","3","4","5","6","7","8","9","10","11"};
    for (int i = 0; i < 12; i++) {
        float rad = i * 30.0f * PI / 180.0f;
        int nx = CLK_CX + (CLK_R - 20) * sin(rad);
        int ny = CLK_CY - (CLK_R - 20) * cos(rad);
        int lw = strlen(hrLbl[i]) * 6;
        tft.setCursor(nx - lw / 2, ny - 4);
        tft.print(hrLbl[i]);
    }

    // ── Left panels: Heart Rate (top) + SpO2 (bottom) ─────────────────────────
    _drawPanelShell(2,   2, 78, 88, UI_RED,  "HEART RATE");
    _drawPanelShell(2,  96, 78, 88, UI_CYAN, "SpO2");

    // Simple heart icon in HR panel
    tft.fillCircle(25, 24, 5, UI_RED);
    tft.fillCircle(33, 24, 5, UI_RED);
    tft.fillTriangle(19, 27, 39, 27, 29, 37, UI_RED);

    // ── Right panels: Temp (top) + Humidity (bottom) ──────────────────────────
    _drawPanelShell(240,  2, 78, 88, UI_ORANGE, "TEMPERATURE");
    _drawPanelShell(240, 96, 78, 88, UI_CYAN,   "HUMIDITY");

    // Thermometer icon in temp panel
    tft.fillRoundRect(276, 12, 6, 20, 3, UI_ORANGE);
    tft.fillCircle(279, 34, 6, UI_ORANGE);

    // Droplet icon in humidity panel
    tft.fillTriangle(279, 102, 271, 116, 287, 116, UI_CYAN);
    tft.fillCircle(279, 117, 6, UI_CYAN);

    // ── Bottom bar ────────────────────────────────────────────────────────────
    tft.fillRoundRect(0, 193, 320, 46, 4, UI_PANEL);
    tft.drawRoundRect(0, 193, 320, 46, 4, UI_BORDER);

    // Compass rose
    tft.fillCircle(22, 216, 18, UI_DARKGRAY);
    tft.drawCircle(22, 216, 18, UI_GRAY);
    tft.setTextColor(UI_WHITE); tft.setTextSize(1);
    tft.setCursor(19, 200); tft.print("N");
    tft.setCursor(19, 227); tft.print("S");
    tft.setCursor( 5, 213); tft.print("W");
    tft.setCursor(32, 213); tft.print("E");

    // Accel bar outlines  (AX / AY / AZ)
    tft.setTextColor(UI_GRAY); tft.setTextSize(1);
    tft.setCursor(46, 197); tft.print("AX");
    tft.setCursor(46, 207); tft.print("AY");
    tft.setCursor(46, 217); tft.print("AZ");
    for (int i = 0; i < 3; i++)
        tft.drawRoundRect(62, 196 + i * 10, 90, 7, 2, UI_BORDER);

    // CO2 + TVOC labels
    tft.setTextColor(UI_GRAY); tft.setTextSize(1);
    tft.setCursor(160, 197); tft.print("eCO2");
    tft.setCursor(160, 218); tft.print("eTVOC");

    // Heartbeat dot label
    tft.setTextColor(UI_GRAY); tft.setTextSize(1);
    tft.setCursor(244, 197); tft.print("PULSE");
}

// ─────────────────────────────────────────────────────────────────────────────
//  HOME SCREEN – DYNAMIC UPDATE
// ─────────────────────────────────────────────────────────────────────────────
void TFTDriver::updateHomeScreen(const DisplayData &d, bool heartBeatTick)
{
    // ── Uptime clock ──────────────────────────────────────────────────────────
    uint32_t s  = millis() / 1000;
    uint8_t  hh = (s / 3600) % 12;   // 12-hour for analog face
    uint8_t  mm = (s / 60)   % 60;
    uint8_t  ss =  s          % 60;

    // ── Analog hands ─────────────────────────────────────────────────────────
    float secAngle  = ss * 6.0f;
    float minAngle  = mm * 6.0f  + ss * 0.1f;
    float hourAngle = hh * 30.0f + mm * 0.5f;

    if (_prevSecAngle >= 0) {
        // Erase previous hands by redrawing in clock-face colour
        _drawHand(_prevSecAngle,  CLK_R - 8,  1, UI_CLOCK_FACE);
        _drawHand(_prevMinAngle,  CLK_R - 18, 3, UI_CLOCK_FACE);
        _drawHand(_prevHourAngle, CLK_R - 28, 5, UI_CLOCK_FACE);
        _redrawTicks();
    }
    _drawHand(hourAngle, CLK_R - 28, 5, UI_WHITE);
    _drawHand(minAngle,  CLK_R - 18, 3, UI_WHITE);
    _drawHand(secAngle,  CLK_R - 8,  1, UI_RED);
    tft.fillCircle(CLK_CX, CLK_CY, 4, UI_CYAN);   // centre pip

    _prevSecAngle  = secAngle;
    _prevMinAngle  = minAngle;
    _prevHourAngle = hourAngle;

    // Digital time below clock
    tft.fillRect(CLK_CX - 46, 176, 92, 10, UI_BG);
    char timeBuf[9];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", hh, mm, ss);
    tft.setTextColor(UI_CYAN); tft.setTextSize(1);
    tft.setCursor(CLK_CX - 46, 177);
    tft.print(timeBuf);

    // ── Fall detected: flash the clock rim red ────────────────────────────────
    uint16_t rimColor = d.fallDetected ? UI_RED : UI_CYAN;
    tft.drawCircle(CLK_CX, CLK_CY, CLK_R + 4, rimColor);
    tft.drawCircle(CLK_CX, CLK_CY, CLK_R + 5, rimColor);

    // ── Left panels ───────────────────────────────────────────────────────────
    char buf[12];

    // Heart Rate
    snprintf(buf, sizeof(buf), "%d", d.heartRate);
    _updatePanelValue(2,  2, 78, buf, "bpm", UI_RED,
                      d.heartRate > 110 || d.heartRate < 45);

    // SpO2
    snprintf(buf, sizeof(buf), "%d", d.spo2);
    _updatePanelValue(2, 96, 78, buf, "%  SpO2", UI_CYAN, d.spo2 < 95);

    // ── Right panels ──────────────────────────────────────────────────────────

    // Temperature
    snprintf(buf, sizeof(buf), "%.1f", d.temperature);
    _updatePanelValue(240,  2, 78, buf, "\xF7""C", UI_ORANGE, d.temperature > 37.5f);

    // Humidity
    snprintf(buf, sizeof(buf), "%.0f", d.humidity);
    _updatePanelValue(240, 96, 78, buf, "% RH", UI_CYAN,
                      d.humidity < 30 || d.humidity > 70);

    // ── Bottom bar ────────────────────────────────────────────────────────────

    // Compass needle
    if (_prevHeading >= 0) {
        float pr = _prevHeading * PI / 180.0f;
        tft.drawLine(22, 216, 22 + (int)(14 * sin(pr)), 216 - (int)(14 * cos(pr)), UI_DARKGRAY);
        tft.drawLine(22, 216, 22 - (int)( 8 * sin(pr)), 216 + (int)( 8 * cos(pr)), UI_DARKGRAY);
    }
    // NOTE: replace 0 with your actual heading when compass is wired up
    int heading = 0;
    float hr2 = heading * PI / 180.0f;
    tft.drawLine(22, 216, 22 + (int)(14 * sin(hr2)), 216 - (int)(14 * cos(hr2)), UI_RED);
    tft.drawLine(22, 216, 22 - (int)( 8 * sin(hr2)), 216 + (int)( 8 * cos(hr2)), UI_WHITE);
    tft.fillCircle(22, 216, 2, UI_WHITE);
    _prevHeading = heading;

    // Accel bars — map ±2g to 0-90px
    auto accelBar = [&](float val, int barY, uint16_t col) {
        tft.fillRoundRect(63, barY, 88, 5, 2, UI_DARKGRAY);
        int filled = constrain((int)((val + 2.0f) / 4.0f * 88), 0, 88);
        tft.fillRoundRect(63, barY, filled, 5, 2, col);
    };
    accelBar(d.accelX, 197, UI_RED);
    accelBar(d.accelY, 207, UI_GREEN);
    accelBar(d.accelZ, 217, UI_CYAN);

    // CO2 value
    tft.fillRect(160, 205, 78, 13, UI_PANEL);
    uint16_t co2Col = d.eCO2 < 600 ? UI_GREEN : d.eCO2 < 1000 ? UI_YELLOW : UI_RED;
    snprintf(buf, sizeof(buf), "%d ppm", d.eCO2);
    tft.setTextColor(co2Col); tft.setTextSize(1);
    tft.setCursor(160, 206);
    tft.print(buf);

    // TVOC value
    tft.fillRect(160, 225, 78, 13, UI_PANEL);
    uint16_t tvocCol = d.eTVOC < 150 ? UI_GREEN : d.eTVOC < 500 ? UI_YELLOW : UI_RED;
    snprintf(buf, sizeof(buf), "%d ppb", d.eTVOC);
    tft.setTextColor(tvocCol); tft.setTextSize(1);
    tft.setCursor(160, 226);
    tft.print(buf);

    // Heartbeat pulse dot
    tft.fillRect(244, 205, 74, 28, UI_PANEL);
    uint16_t dotCol = heartBeatTick ? UI_RED : 0x3000;
    tft.fillCircle(281, 215, 10, dotCol);
    // BPM below dot
    snprintf(buf, sizeof(buf), "%d", d.heartRate);
    tft.setTextColor(UI_GRAY); tft.setTextSize(1);
    int bw = strlen(buf) * 6;
    tft.setCursor(281 - bw / 2, 229);
    tft.print(buf);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ENV SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void TFTDriver::drawEnvScreen_BG()
{
    tft.fillScreen(UI_BG);

    // Header
    tft.setTextColor(UI_CYAN);   tft.setTextSize(1);
    tft.setCursor(4, 7);         tft.print("< HOME");
    tft.setTextColor(UI_WHITE);
    tft.setCursor(80, 7);        tft.print("ENVIRONMENT  &  AIR QUALITY");
    tft.drawFastHLine(0, 18, 320, UI_BORDER);

    // Four panel shells
    _drawPanelShell(  4, 24, 148, 100, UI_ORANGE, "TEMPERATURE");
    _drawPanelShell(  4, 130, 148, 100, UI_CYAN,  "HUMIDITY");
    _drawPanelShell(166, 24, 150, 100, UI_GREEN,  "eCO2");
    _drawPanelShell(166, 130, 150, 100, UI_YELLOW, "eTVOC");
}

void TFTDriver::updateEnvScreen(const DisplayData &d)
{
    char buf[12];

    snprintf(buf, sizeof(buf), "%.1f", d.temperature);
    _updatePanelValue(  4,  24, 148, buf, "\xF7""C",  UI_ORANGE, d.temperature > 37.5f);

    snprintf(buf, sizeof(buf), "%.0f", d.humidity);
    _updatePanelValue(  4, 130, 148, buf, "% RH",    UI_CYAN,   d.humidity < 30 || d.humidity > 70);

    uint16_t co2Col = d.eCO2 < 600 ? UI_GREEN : d.eCO2 < 1000 ? UI_YELLOW : UI_RED;
    snprintf(buf, sizeof(buf), "%d", d.eCO2);
    _updatePanelValue(166,  24, 150, buf, "ppm",     co2Col,    d.eCO2 > 1000);

    uint16_t tvocCol = d.eTVOC < 150 ? UI_GREEN : d.eTVOC < 500 ? UI_YELLOW : UI_RED;
    snprintf(buf, sizeof(buf), "%d", d.eTVOC);
    _updatePanelValue(166, 130, 150, buf, "ppb",     tvocCol,   d.eTVOC > 500);
}

// ─────────────────────────────────────────────────────────────────────────────
//  FALL ALERT OVERLAY
// ─────────────────────────────────────────────────────────────────────────────
void TFTDriver::drawFallAlert(uint32_t fallAlertStart)
{
    tft.fillRoundRect(40, 60, 240, 120, 12, UI_BG);
    tft.drawRoundRect(40, 60, 240, 120, 12, UI_RED);
    tft.drawRoundRect(41, 61, 238, 118, 12, UI_ORANGE);

    tft.setTextColor(UI_RED);   tft.setTextSize(3);
    tft.setCursor(148, 70);     tft.print("!");

    tft.setTextColor(UI_RED);   tft.setTextSize(2);
    tft.setCursor(82, 100);     tft.print("FALL DETECTED");

    tft.setTextColor(UI_WHITE); tft.setTextSize(1);
    tft.setCursor(68, 130);     tft.print("Sudden motion event detected.");
    tft.setCursor(80, 142);     tft.print("Are you okay? Alert in 5s.");

    float progress = constrain((float)(millis() - fallAlertStart) / 5000.0f, 0.0f, 1.0f);
    tft.fillRect(60, 162, 200, 8, UI_BG);
    tft.fillRect(60, 162, (int)(200 * progress), 8, UI_ORANGE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  TEXT HELPER
// ─────────────────────────────────────────────────────────────────────────────
void TFTDriver::drawText(int16_t x, int16_t y, const char *text,
                          uint8_t size, uint16_t color, bool centered)
{
    tft.setTextColor(color);
    tft.setTextSize(size);
    if (centered)
        tft.setCursor(x - (strlen(text) * 6 * size) / 2, y);
    else
        tft.setCursor(x, y);
    tft.print(text);
}