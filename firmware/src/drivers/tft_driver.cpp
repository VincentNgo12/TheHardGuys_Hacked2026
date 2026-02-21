#include "tft_driver.h"
#include <Arduino.h>
#include <math.h>

// ─────────────────────────────────────────────────────────────────────────────
//  LAYOUT  (320 × 240 landscape)
//
//   y=  0..21   Status bar  ← "SMART BAND" left │ HH:MM:SS center │ compass right
//   y= 23..110  ROW 1:  BPM card (x=2,w=96) │ Clock card (x=100,w=120) │ SpO2 (x=222,w=96)
//   y=113..154  ROW 2:  Temp card (x=2,w=156) │ Humidity card (x=160,w=158)
//   y=157..208  AQI panel (x=2, w=316)  ← badge │ CO2+bar │ TVOC+bar
//   y=211..238  Bottom bar (x=2, w=316) ← AX/AY bars │ fall status
// ─────────────────────────────────────────────────────────────────────────────

// ── Threshold colour selectors ───────────────────────────────────────────────

uint16_t TFTDriver::_hrColor(uint8_t hr) {
    if (hr == 0)              return UI_TEXT_MED;
    if (hr < 45 || hr > 120)  return UI_VAL_ALERT;
    if (hr < 55 || hr > 100)  return UI_VAL_WARN;
    return UI_VAL_GOOD;
}
uint16_t TFTDriver::_spo2Color(uint8_t s) {
    if (s == 0)  return UI_TEXT_MED;
    if (s < 90)  return UI_VAL_ALERT;
    if (s < 95)  return UI_VAL_WARN;
    return UI_VAL_GOOD;
}
uint16_t TFTDriver::_tempColor(float t) {
    if (t > 35.0f || t < 10.0f) return UI_VAL_ALERT;
    if (t > 28.0f || t < 15.0f) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}
uint16_t TFTDriver::_humidColor(float h) {
    if (h < 20.0f || h > 80.0f) return UI_VAL_ALERT;
    if (h < 30.0f || h > 65.0f) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}
uint16_t TFTDriver::_co2Color(uint16_t c) {
    if (c > 1500) return UI_VAL_ALERT;
    if (c >  600) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}
uint16_t TFTDriver::_tvocColor(uint16_t t) {
    if (t > 500) return UI_VAL_ALERT;
    if (t > 150) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}
uint16_t TFTDriver::_aqiColor(uint8_t aqi) {
    switch (aqi) {
        case 1:  return AQI_COL_1;
        case 2:  return AQI_COL_2;
        case 3:  return AQI_COL_3;
        case 4:  return AQI_COL_4;
        default: return AQI_COL_5;
    }
}

// ── Draw primitives ──────────────────────────────────────────────────────────

// White rounded card with a thin coloured border.
void TFTDriver::_drawCard(int x, int y, int w, int h, uint16_t border) {
    tft.fillRoundRect(x, y, w, h, 6, UI_CARD);
    tft.drawRoundRect(x, y, w, h, 6, border);
}

// Heart icon centred at (cx, cy). sz is the half-span of the whole shape.
void TFTDriver::_drawHeart(int cx, int cy, int sz, uint16_t color) {
    int r = (sz + 1) / 2;
    tft.fillCircle(cx - r, cy, r, color);
    tft.fillCircle(cx + r, cy, r, color);
    tft.fillTriangle(cx - sz + 1, cy + 1,
                     cx + sz - 1, cy + 1,
                     cx,          cy + sz, color);
}

// Mini compass rose at (cx, cy) radius r, needle at heading degrees.
// Fully redraws itself so it can be called each tick if needed.
void TFTDriver::_drawMiniCompass(int cx, int cy, int r, int heading) {
    tft.fillCircle(cx, cy, r, UI_CARD);
    tft.drawCircle(cx, cy, r, UI_BORDER);
    tft.setTextColor(UI_TEXT_DARK); tft.setTextSize(1);
    tft.setCursor(cx - 3, cy - r + 2); tft.print("N");
    float rad = heading * PI / 180.0f;
    tft.drawLine(cx, cy,
                 cx + (int)((r - 3) * sin(rad)),
                 cy - (int)((r - 3) * cos(rad)),
                 UI_ACCENT);               // north tip – rose
    tft.drawLine(cx, cy,
                 cx - (int)((r - 4) * sin(rad)),
                 cy + (int)((r - 4) * cos(rad)),
                 UI_TEXT_MED);             // south tail – grey
    tft.fillCircle(cx, cy, 2, UI_TEXT_DARK);
}

// Horizontal progress bar.  frac in [0..1].  Track colour = UI_BG.
void TFTDriver::_drawHBar(int x, int y, int w, int h,
                           float frac, uint16_t color) {
    tft.fillRoundRect(x, y, w, h, 2, UI_BG);
    int fill = constrain((int)(frac * w), 0, w);
    if (fill > 0) tft.fillRoundRect(x, y, fill, h, 2, color);
}

// ── Legacy env-screen panel helpers ─────────────────────────────────────────

void TFTDriver::_drawPanelShell(int x, int y, int w, int h,
                                 uint16_t accent, const char *label) {
    tft.fillRoundRect(x, y, w, h, 4, UI_CARD);
    tft.drawRoundRect(x, y, w, h, 4, accent);
    tft.fillRect(x + 1, y + 1, w - 2, 3, accent);
    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    int tw = strlen(label) * 6;
    tft.setCursor(x + (w - tw) / 2, y + 6);
    tft.print(label);
}

void TFTDriver::_updatePanelValue(int x, int y, int w,
                                   const char *valStr, const char *unit,
                                   uint16_t color, bool alert) {
    tft.fillRect(x + 1, y + 16, w - 2, 34, UI_CARD);
    tft.setTextColor(alert ? UI_VAL_ALERT : color);
    tft.setTextSize(3);
    int tw = strlen(valStr) * 18;
    tft.setCursor(x + (w - tw) / 2, y + 18);
    tft.print(valStr);
    tft.fillRect(x + 1, y + 50, w - 2, 10, UI_CARD);
    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tw = strlen(unit) * 6;
    tft.setCursor(x + (w - tw) / 2, y + 52);
    tft.print(unit);
}

// ── Legacy drawWidget (kept so old callsites still compile) ──────────────────

void TFTDriver::drawWidget(const char *label, int x, int y, int w, int h,
                            int value, const char *unit, uint16_t color) {
    _drawPanelShell(x, y, w, h, color, label);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", value);
    _updatePanelValue(x, y, w, buf, unit, color);
}

// ─────────────────────────────────────────────────────────────────────────────
//  INIT
// ─────────────────────────────────────────────────────────────────────────────

bool TFTDriver::begin() {
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(UI_BG);
    _prevHeading = -1;
    return true;
}

void TFTDriver::clearScreen(uint16_t color) { tft.fillScreen(color); }

// ─────────────────────────────────────────────────────────────────────────────
//  SPLASH
// ─────────────────────────────────────────────────────────────────────────────

void TFTDriver::drawSplash() {
    tft.fillScreen(UI_BG);

    // Soft glowing halo
    tft.fillCircle(160, 95, 53, UI_ACCENT_SOFT);
    tft.fillCircle(160, 95, 50, UI_CARD);
    _drawHeart(160, 80, 14, UI_ACCENT);

    tft.setTextColor(UI_TEXT_DARK); tft.setTextSize(2);
    tft.setCursor(107, 104); tft.print("SMART BAND");

    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tft.setCursor(88, 120); tft.print("ESP32 Feather V2  v1.0");
    tft.setCursor(108, 132); tft.print("Initializing...");

    tft.drawRoundRect(60, 155, 200, 8, 3, UI_BORDER);
    for (int i = 0; i <= 196; i += 4) {
        tft.fillRoundRect(62, 157, i, 4, 2, UI_ACCENT);
        delay(15);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  HOME SCREEN – STATIC BACKGROUND  (call once on screen switch)
// ─────────────────────────────────────────────────────────────────────────────

void TFTDriver::drawHomeScreen_BG() {
    tft.fillScreen(UI_BG);

    // ── Status bar (y=0..21) ─────────────────────────────
    tft.fillRect(0, 0, 320, 22, UI_STATUSBAR);
    tft.setTextColor(UI_TEXT_DARK); tft.setTextSize(1);
    tft.setCursor(6, 7); tft.print("SMART BAND");

    // Compass rose shell – needle drawn dynamically in update
    tft.fillCircle(COMPASS_CX, COMPASS_CY, COMPASS_R, UI_CARD);
    tft.drawCircle(COMPASS_CX, COMPASS_CY, COMPASS_R, UI_BORDER);
    tft.setTextColor(UI_TEXT_DARK); tft.setTextSize(1);
    tft.setCursor(COMPASS_CX - 3, COMPASS_CY - COMPASS_R + 2);
    tft.print("N");

    // ── ROW 1 cards (y=23, h=88) ─────────────────────────
    //  BPM  : x=2,   w=96   centre_x=50
    //  Clock: x=100, w=120  centre_x=160
    //  SpO2 : x=222, w=96   centre_x=270
    _drawCard(  2, 23,  96, 88, UI_ACCENT_SOFT);
    _drawCard(100, 23, 120, 88, UI_BORDER);
    _drawCard(222, 23,  96, 88, UI_VAL_INFO);

    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tft.setCursor(20, 28); tft.print("HEART RATE");   // centred in w=96
    tft.setCursor(143, 28); tft.print("TIME");         // centred in w=120
    tft.setCursor(252, 28); tft.print("SpO2");         // centred in w=96

    // ── ROW 2 cards (y=113, h=42) ────────────────────────
    //  Temp : x=2,   w=156  centre_x=80
    //  Humid: x=160, w=158  centre_x=239
    _drawCard(  2, 113, 156, 42, UI_VAL_WARN);
    _drawCard(160, 113, 158, 42, UI_VAL_INFO);

    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tft.setCursor(47, 118); tft.print("TEMPERATURE");  // "TEMPERATURE" 66px, centred at 80
    tft.setCursor(215, 118); tft.print("HUMIDITY");    // "HUMIDITY" 48px, centred at 239

    // ── AQI panel (y=157, h=52) ──────────────────────────
    //  Three zones separated by vertical lines:
    //    Badge : x=3..87   centre_x=45
    //    CO2   : x=89..203 centre_x=146
    //    TVOC  : x=205..317 centre_x=261
    tft.fillRoundRect(2, 157, 316, 52, 6, UI_CARD);
    tft.drawRoundRect(2, 157, 316, 52, 6, UI_BORDER);
    tft.drawFastVLine( 88, 162, 42, UI_BORDER);
    tft.drawFastVLine(204, 162, 42, UI_BORDER);

    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tft.setCursor(11, 162); tft.print("AIR QUALITY");  // 66px centred at 45
    tft.setCursor(137, 162); tft.print("CO2");          // 18px centred at 146
    tft.setCursor(249, 162); tft.print("TVOC");         // 24px centred at 261

    // ── Bottom bar (y=211, h=27) ─────────────────────────
    tft.fillRoundRect(  2, 211, 316, 27, 4, UI_CARD);
    tft.drawRoundRect(  2, 211, 316, 27, 4, UI_BORDER);

    tft.setTextColor(UI_TEXT_LIGHT); tft.setTextSize(1);
    tft.setCursor(6, 215); tft.print("AX");
    tft.setCursor(6, 225); tft.print("AY");
    // Bar track outlines
    tft.drawRoundRect(22, 214, 110, 5, 2, UI_BORDER);
    tft.drawRoundRect(22, 224, 110, 5, 2, UI_BORDER);
}

// ─────────────────────────────────────────────────────────────────────────────
//  HOME SCREEN – DYNAMIC UPDATE
// ─────────────────────────────────────────────────────────────────────────────

void TFTDriver::updateHomeScreen(const DisplayData &d, bool heartBeatTick) {
    char buf[16];

    // ── Uptime ────────────────────────────────────────────
    uint32_t s  = millis() / 1000;
    uint8_t  hh = (s / 3600) % 24;
    uint8_t  mm = (s / 60)   % 60;
    uint8_t  ss =  s          % 60;

    // Status bar – time (centre of bar, skip compass zone)
    tft.fillRect(88, 4, 130, 13, UI_STATUSBAR);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hh, mm, ss);
    tft.setTextColor(UI_TEXT_DARK); tft.setTextSize(1);
    tft.setCursor(100, 7); tft.print(buf);

    // Compass (redraws its own background, so safe to call each tick)
    _drawMiniCompass(COMPASS_CX, COMPASS_CY, COMPASS_R, 0);
    // ↑ Replace 0 with actual magnetometer heading when hardware is wired up.

    // ─────────────────────────────────────────────────────
    //  ROW 1 — HEART RATE card (x=2, y=23, w=96, h=88)
    //  Dynamic area: y=37..110  (below the static label at y=28)
    // ─────────────────────────────────────────────────────
    tft.fillRect(3, 37, 94, 72, UI_CARD);

    // Heart icon – pulses bright on beat, soft at rest
    _drawHeart(50, 52, 10, heartBeatTick ? UI_ACCENT : UI_ACCENT_SOFT);

    // BPM value
    uint16_t hrCol = _hrColor(d.heartRate);
    snprintf(buf, sizeof(buf), "%d", d.heartRate);
    tft.setTextColor(hrCol); tft.setTextSize(3);
    int tw = strlen(buf) * 18;
    tft.setCursor(50 - tw / 2, 66); tft.print(buf);

    tft.setTextColor(UI_TEXT_LIGHT); tft.setTextSize(1);
    tft.setCursor(44, 92); tft.print("bpm");

    // ─────────────────────────────────────────────────────
    //  ROW 1 — CLOCK card (x=100, y=23, w=120, h=88)
    //  centre_x=160
    // ─────────────────────────────────────────────────────
    tft.fillRect(101, 37, 118, 72, UI_CARD);

    snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
    tft.setTextColor(UI_ACCENT); tft.setTextSize(3);
    tw = strlen(buf) * 18;                            // "12:34" = 90 px
    tft.setCursor(160 - tw / 2, 47); tft.print(buf);

    snprintf(buf, sizeof(buf), ":%02d", ss);
    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(2);
    tw = strlen(buf) * 12;
    tft.setCursor(160 - tw / 2, 78); tft.print(buf);

    // ─────────────────────────────────────────────────────
    //  ROW 1 — SpO2 card (x=222, y=23, w=96, h=88)
    //  centre_x=270
    // ─────────────────────────────────────────────────────
    tft.fillRect(223, 37, 94, 72, UI_CARD);

    uint16_t spo2Col = _spo2Color(d.spo2);
    tft.fillCircle(270, 50, 5, spo2Col);               // saturation indicator dot

    snprintf(buf, sizeof(buf), "%d", d.spo2);
    tft.setTextColor(spo2Col); tft.setTextSize(3);
    tw = strlen(buf) * 18;
    tft.setCursor(270 - tw / 2, 63); tft.print(buf);

    tft.setTextColor(UI_TEXT_LIGHT); tft.setTextSize(1);
    tft.setCursor(263, 91); tft.print("% O2");

    // ─────────────────────────────────────────────────────
    //  ROW 2 — TEMPERATURE  (x=2, y=113, w=156, h=42)
    // ─────────────────────────────────────────────────────
    tft.fillRect(3, 122, 154, 30, UI_CARD);
    uint16_t tmpCol = _tempColor(d.temperature);
    snprintf(buf, sizeof(buf), "%.1f \xF7""C", d.temperature);
    tft.setTextColor(tmpCol); tft.setTextSize(2);
    tft.setCursor(10, 128); tft.print(buf);

    // ─────────────────────────────────────────────────────
    //  ROW 2 — HUMIDITY  (x=160, y=113, w=158, h=42)
    // ─────────────────────────────────────────────────────
    tft.fillRect(161, 122, 156, 30, UI_CARD);
    uint16_t humCol = _humidColor(d.humidity);
    snprintf(buf, sizeof(buf), "%.0f%%  RH", d.humidity);
    tft.setTextColor(humCol); tft.setTextSize(2);
    tft.setCursor(168, 128); tft.print(buf);

    // ─────────────────────────────────────────────────────
    //  AQI PANEL  (x=2, y=157, w=316, h=52)
    //  Static labels sit at y=162..169.
    //  Dynamic content: y=171..205.
    // ─────────────────────────────────────────────────────

    // — Badge zone (x=3..87) —
    tft.fillRect(3, 171, 85, 34, UI_CARD);
    uint16_t aqiCol = _aqiColor(d.aqi);
    tft.fillRoundRect(4, 172, 82, 32, 4, aqiCol);

    tft.setTextColor(UI_CARD); tft.setTextSize(2);
    snprintf(buf, sizeof(buf), "%d", d.aqi);
    tft.setCursor(20, 176); tft.print(buf);

    tft.setTextSize(1);
    const char *aqiStr;
    switch (d.aqi) {
        case 1: aqiStr = "GOOD"; break;
        case 2: aqiStr = "FAIR"; break;
        case 3: aqiStr = "MOD."; break;
        case 4: aqiStr = "POOR"; break;
        default: aqiStr = "HAZD"; break;
    }
    tw = strlen(aqiStr) * 6;
    tft.setCursor(44 - tw / 2, 192); tft.print(aqiStr);

    // — CO2 zone (x=89..203, centre=146) —
    tft.fillRect(89, 171, 114, 34, UI_CARD);
    uint16_t co2Col = _co2Color(d.eCO2);

    snprintf(buf, sizeof(buf), "%d", d.eCO2);
    tft.setTextColor(co2Col); tft.setTextSize(2);
    tw = strlen(buf) * 12;
    tft.setCursor(146 - tw / 2, 173); tft.print(buf);

    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tft.setCursor(138, 190); tft.print("ppm");

    // CO2 progress bar  (0–2000 ppm → 0–106 px)
    _drawHBar(92, 200, 106, 4, constrain(d.eCO2 / 2000.0f, 0.f, 1.f), co2Col);

    // — TVOC zone (x=205..317, centre=261) —
    tft.fillRect(205, 171, 112, 34, UI_CARD);
    uint16_t tvocCol = _tvocColor(d.eTVOC);

    snprintf(buf, sizeof(buf), "%d", d.eTVOC);
    tft.setTextColor(tvocCol); tft.setTextSize(2);
    tw = strlen(buf) * 12;
    tft.setCursor(261 - tw / 2, 173); tft.print(buf);

    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tft.setCursor(252, 190); tft.print("ppb");

    // TVOC progress bar  (0–1000 ppb → 0–108 px)
    _drawHBar(208, 200, 108, 4, constrain(d.eTVOC / 1000.0f, 0.f, 1.f), tvocCol);

    // ─────────────────────────────────────────────────────
    //  BOTTOM BAR  (x=2, y=211, w=316, h=27)
    // ─────────────────────────────────────────────────────
    tft.fillRect(23, 213, 109, 22, UI_CARD);           // clear bar area only

    // Accel bars — map ±4 g → [0, 1]
    float axf = constrain((d.accelX + 4.0f) / 8.0f, 0.f, 1.f);
    float ayf = constrain((d.accelY + 4.0f) / 8.0f, 0.f, 1.f);
    _drawHBar(23, 214, 110, 5, axf, UI_VAL_INFO);
    _drawHBar(23, 224, 110, 5, ayf, UI_VAL_INFO);

    // Fall status badge
    tft.fillRect(170, 213, 146, 22, UI_CARD);
    if (d.fallDetected) {
        tft.fillRoundRect(170, 213, 145, 22, 3, UI_VAL_ALERT);
        tft.setTextColor(UI_CARD); tft.setTextSize(1);
        tft.setCursor(178, 219); tft.print("!! FALL DETECTED !!");
    } else {
        tft.fillRoundRect(170, 213, 145, 22, 3, UI_VAL_GOOD);
        tft.setTextColor(UI_CARD); tft.setTextSize(1);
        tft.setCursor(218, 219); tft.print("ALL CLEAR");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ENV SCREEN  (detail view – called from serial command / touch)
// ─────────────────────────────────────────────────────────────────────────────

void TFTDriver::drawEnvScreen_BG() {
    tft.fillScreen(UI_BG);

    // Header
    tft.fillRect(0, 0, 320, 22, UI_STATUSBAR);
    tft.setTextColor(UI_ACCENT);  tft.setTextSize(1);
    tft.setCursor(4, 7); tft.print("< HOME");
    tft.setTextColor(UI_TEXT_DARK);
    tft.setCursor(72, 7); tft.print("ENVIRONMENT  &  AIR QUALITY");

    // Four panel shells (pink-themed)
    _drawPanelShell(  4,  26, 148, 100, UI_VAL_WARN,  "TEMPERATURE");
    _drawPanelShell(  4, 132, 148, 100, UI_VAL_INFO,  "HUMIDITY");
    _drawPanelShell(166,  26, 150, 100, UI_VAL_GOOD,  "eCO2");
    _drawPanelShell(166, 132, 150, 100, UI_VAL_WARN,  "eTVOC");
}

void TFTDriver::updateEnvScreen(const DisplayData &d) {
    char buf[12];

    snprintf(buf, sizeof(buf), "%.1f", d.temperature);
    _updatePanelValue(  4,  26, 148, buf, "\xF7""C",
                       _tempColor(d.temperature),
                       d.temperature > 37.5f);

    snprintf(buf, sizeof(buf), "%.0f", d.humidity);
    _updatePanelValue(  4, 132, 148, buf, "% RH",
                       _humidColor(d.humidity),
                       d.humidity < 20.f || d.humidity > 80.f);

    snprintf(buf, sizeof(buf), "%d", d.eCO2);
    _updatePanelValue(166,  26, 150, buf, "ppm",
                      _co2Color(d.eCO2), d.eCO2 > 1500);

    snprintf(buf, sizeof(buf), "%d", d.eTVOC);
    _updatePanelValue(166, 132, 150, buf, "ppb",
                      _tvocColor(d.eTVOC), d.eTVOC > 500);
}

// ─────────────────────────────────────────────────────────────────────────────
//  FALL ALERT OVERLAY
// ─────────────────────────────────────────────────────────────────────────────

void TFTDriver::drawFallAlert(uint32_t fallAlertStart) {
    // Semi-opaque modal frame
    tft.fillRoundRect(40, 58, 240, 124, 12, UI_CARD);
    tft.drawRoundRect(40, 58, 240, 124, 12, UI_VAL_ALERT);
    tft.drawRoundRect(41, 59, 238, 122, 12, UI_ACCENT_SOFT);

    tft.setTextColor(UI_VAL_ALERT); tft.setTextSize(3);
    tft.setCursor(148, 68); tft.print("!");

    tft.setTextColor(UI_VAL_ALERT); tft.setTextSize(2);
    tft.setCursor(82, 98); tft.print("FALL DETECTED");

    tft.setTextColor(UI_TEXT_MED); tft.setTextSize(1);
    tft.setCursor(68, 128); tft.print("Sudden motion event detected.");
    tft.setCursor(80, 140); tft.print("Are you okay? Alert in 5s.");

    float p = constrain((float)(millis() - fallAlertStart) / 5000.0f, 0.f, 1.f);
    tft.fillRect(60, 160, 200, 8, UI_BG);
    tft.fillRect(60, 160, (int)(200 * p), 8, UI_ACCENT);
}

// ─────────────────────────────────────────────────────────────────────────────
//  TEXT HELPER
// ─────────────────────────────────────────────────────────────────────────────

void TFTDriver::drawText(int16_t x, int16_t y, const char *text,
                          uint8_t size, uint16_t color, bool centered) {
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.setCursor(centered ? x - (int16_t)(strlen(text) * 6 * size) / 2 : x, y);
    tft.print(text);
}