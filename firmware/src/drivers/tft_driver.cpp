// firmware/src/tft_driver.cpp
// ─────────────────────────────────────────────────────────────────
//  "Sakura Band" TFT Driver Implementation
//  ILI9341 320×240  –  ESP32 / Arduino
//  Adafruit_GFX  +  Adafruit_ILI9341
// ─────────────────────────────────────────────────────────────────

#include "tft_driver.h"

// ═════════════════════════════════════════════════════════════════
//  LIFECYCLE
// ═════════════════════════════════════════════════════════════════

bool TFTDriver::begin() {
    tft.begin();
    tft.setRotation(1);   // landscape, USB at left
    tft.fillScreen(C_BG);
    return true;
}

void TFTDriver::clearScreen(uint16_t color) {
    tft.fillScreen(color);
}

void TFTDriver::drawSplash() {
    tft.fillScreen(C_BG);
    // Central blossom
    _drawBlossom(SCREEN_W / 2, SCREEN_H / 2 - 16, 28, C_ACCENT2, C_ACCENT);
    // Title
    drawText(SCREEN_W / 2, SCREEN_H / 2 + 20, "Sakura Band", 2, C_ACCENT, true);
    drawText(SCREEN_W / 2, SCREEN_H / 2 + 40, "initialising...", 1, C_SUBTEXT, true);
}

// ═════════════════════════════════════════════════════════════════
//  HOME SCREEN  (Screen 0)
// ═════════════════════════════════════════════════════════════════

void TFTDriver::drawHomeScreen_BG() {
    tft.fillScreen(C_BG);
    _drawSakuraBranch(false);

    // ── Nav buttons (static) ──────────────────────────────────
    _drawCard(204, 100, 60, 18, 5, C_CARD_A, C_ACCENT2);
    drawText(212, 106, "* Alert", 1, C_ACCENT);

    _drawCard(204, 122, 60, 18, 5, C_CARD_A, C_ACCENT2);
    drawText(216, 128, "Air ~", 1, C_ACCENT);

    // ── ENV summary card shell ────────────────────────────────
    _drawCard(8, 172, 252, 60, 8, C_CARD_A, C_ACCENT2);
    const char *lbl[] = { "eCO2", "Hum.", "TVOC", "Temp" };
    const char *unt[] = { "ppm",  "%",    "ppb",  "*C"   };
    for (int i = 0; i < 4; i++) {
        int sx = 10 + i * 62;
        drawText(sx + 2, 172 + 6,  lbl[i], 1, C_SUBTEXT);
        drawText(sx + 2, 172 + 48, unt[i], 1, C_SUBTEXT);
        if (i < 3)
            tft.drawLine(sx + 61, 176, sx + 61, 228, C_ACCENT3);
    }
}

void TFTDriver::updateHomeScreen(const DisplayData &d, bool heartBeatTick) {
    // ── Status bar ────────────────────────────────────────────
    _drawStatusBar(true, d.battPct);

    // ── Clock ─────────────────────────────────────────────────
    // Pass bgColor so each character cell self-erases the previous digit.
    // This is the only reliable fix for digits "sticking" when they change.
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", 11, 11);
    drawText(8, 24, buf, 5, C_TEXT, false, C_BG);

    snprintf(buf, sizeof(buf), "%02d", d.sec);
    drawText(10, 76, buf, 2, C_ACCENT, false, C_BG);

    drawText(10, 94, d.dateStr, 1, C_SUBTEXT, false, C_BG);

    // Rose accent line
    tft.fillRect(10, 106, 120, 1, C_BG);
    for (int i = 0; i < 120; i++)
        tft.drawPixel(10 + i, 106,
            i < 80  ? C_ACCENT  :
            i < 100 ? C_ACCENT2 : C_ACCENT3);

    // ── Compass ───────────────────────────────────────────────
    _drawCompass(232, 58, 26, d.bearing);

    // ── Heart-rate card ───────────────────────────────────────
    uint16_t hrCol = heartBeatTick ? C_ROSE : _hrColor(d.heartRate);
    _drawCard(8, 114, 148, 50, 10, C_CARD_A, C_ACCENT2);

    // Heart shape
    int hx = 8, hy = 114;
    _drawHeart(hx + 18, hy + 22, 7, hrCol);

    // BPM number
    snprintf(buf, sizeof(buf), "%d", d.heartRate);
    tft.fillRect(hx + 34, hy + 12, 60, 28, C_CARD_A);
    drawText(hx + 34, hy + 12, buf, 3, hrCol);
    drawText(hx + 34, hy + 40, "bpm", 1, C_SUBTEXT);

    // Mini ECG trace
    const int16_t ep[][2] = {
        {0,0},{6,0},{8,4},{10,-8},{12,8},{14,0},
        {18,0},{20,-2},{22,2},{24,0},{38,0}
    };
    int ex = hx + 98, ey = hy + 26;
    for (int i = 0; i < 10; i++)
        tft.drawLine(ex + ep[i][0],   ey + ep[i][1]   * 2,
                     ex + ep[i+1][0], ey + ep[i+1][1] * 2,
                     hrCol);

    // ── ENV summary values ────────────────────────────────────
    char vals[4][10];
    snprintf(vals[0], 10, "%d",   d.eCO2);
    snprintf(vals[1], 10, "%d",   (int)d.humidity);
    snprintf(vals[2], 10, "%d",   d.eTVOC);
    snprintf(vals[3], 10, "%.1f", d.temperature);

    uint16_t vcols[4] = {
        _co2Color(d.eCO2),
        _humidColor(d.humidity),
        _tvocColor(d.eTVOC),
        _tempColor(d.temperature)
    };

    // Re-stamp every row of each ENV column on every update.
    // Two-arg drawText (fg + bg) makes each glyph cell self-erasing,
    // so label, value, and unit can never overwrite each other.
    const char *lbl[] = { "eCO2", "Hum.", "TVOC", "Temp" };
    const char *unt[] = { "ppm",  "%",    "ppb",  "*C"   };
    for (int i = 0; i < 4; i++) {
        int sx = 10 + i * 62;
        drawText(sx + 2, 172 +  6, lbl[i],  1, C_SUBTEXT, false, C_CARD_A); // label
        drawText(sx + 2, 172 + 18, vals[i], 2, vcols[i],  false, C_CARD_A); // value (size-2 = 16 px tall)
        drawText(sx + 2, 172 + 38, unt[i],  1, C_SUBTEXT, false, C_CARD_A); // unit  (clear of value row)
    }
}

// ═════════════════════════════════════════════════════════════════
//  ENV / AIR QUALITY SCREEN  (Screen 1)
// ═════════════════════════════════════════════════════════════════

void TFTDriver::drawEnvScreen_BG() {
    tft.fillScreen(C_BG);
    _drawSakuraBranch(true);

    // Row label
    drawText(110, 6, "AIR QUALITY", 1, C_SUBTEXT);

    // Static gauge labels – top row
    drawText(18, 26, "TEMP", 1, C_SUBTEXT);
    drawText(108, 26, "HUM",  1, C_SUBTEXT);

    // AQI box shell
    _drawCard(198, 30, 100, 78, 8, C_CARD_A, C_ACCENT2);
    drawText(208, 40, "AIR QUALITY", 1, C_SUBTEXT);
    drawText(226, 88, "index", 1, C_SUBTEXT);

    // Dashed divider
    for (int i = 8; i < SCREEN_W - 8; i += 6)
        tft.drawFastHLine(i, 120, 3, C_ACCENT3);

    // Static gauge labels – bottom row
    const char *blbl[] = { "eCO2", "TVOC", "SpO2" };
    for (int i = 0; i < 3; i++)
        drawText(18 + i * 100, 128, blbl[i], 1, C_SUBTEXT);
}

void TFTDriver::updateEnvScreen(const DisplayData &d) {
    _drawStatusBar(false, d.battPct);

    // ── Top row gauges ────────────────────────────────────────
    // TEMP
    float tFrac = constrain((d.temperature + 10.f) / 60.f, 0.f, 1.f);
    _drawGaugeArc(50, 90, 34, 7, 215, 325, C_ACCENT3, C_ACCENT, tFrac);
    char tbuf[10]; snprintf(tbuf, sizeof(tbuf), "%.1f", d.temperature);
    tft.fillRect(30, 78, 42, 18, C_BG);
    drawText(38, 82, tbuf, 1, C_TEXT);
    drawText(44, 93, "*C",   1, C_SUBTEXT);

    // HUM
    _drawGaugeArc(135, 90, 34, 7, 215, 325,
                  C_ACCENT3, C_MINT, constrain(d.humidity / 100.f, 0.f, 1.f));
    char hbuf[8]; snprintf(hbuf, sizeof(hbuf), "%d", (int)d.humidity);
    tft.fillRect(115, 78, 42, 18, C_BG);
    drawText(127, 82, hbuf, 1, C_TEXT);
    drawText(132, 93, "%",   1, C_SUBTEXT);

    // AQI box value
    uint16_t aqiCol = _aqiColor(d.aqi);
    const char *aqiTxt = (d.eCO2 >= 1500) ? "Poor"  :
                         (d.eCO2 >= 800)  ? "Mod."  : "Good";
    tft.fillRect(200, 52, 96, 30, C_CARD_A);
    drawText(210, 56, aqiTxt, 2, aqiCol);

    // ── Bottom row gauges ─────────────────────────────────────
    // eCO2
    float co2Frac  = constrain((d.eCO2 - 400.f) / 1600.f, 0.f, 1.f);
    float tvocFrac = constrain(d.eTVOC / 500.f, 0.f, 1.f);
    float spo2Frac = constrain((d.spo2 - 90.f) / 10.f, 0.f, 1.f);

    uint16_t co2Col  = _co2Color(d.eCO2);
    uint16_t tvocCol = _tvocColor(d.eTVOC);
    uint16_t spo2Col = _spo2Color(d.spo2);

    struct { float frac; uint16_t col; char val[10]; } gauges[3];
    snprintf(gauges[0].val, 10, "%d",   d.eCO2);  gauges[0].frac = co2Frac;  gauges[0].col = co2Col;
    snprintf(gauges[1].val, 10, "%d",   d.eTVOC); gauges[1].frac = tvocFrac; gauges[1].col = tvocCol;
    snprintf(gauges[2].val, 10, "%d%%", d.spo2);  gauges[2].frac = spo2Frac; gauges[2].col = spo2Col;

    for (int i = 0; i < 3; i++) {
        int gx = 18 + i * 100;
        _drawGaugeArc(gx + 30, 192, 26, 6, 215, 325,
                      C_ACCENT3, gauges[i].col, gauges[i].frac);
        tft.fillRect(gx + 8, 178, 50, 20, C_BG);
        drawText(gx + 14, 185, gauges[i].val, 1, gauges[i].col);
    }
}

// ═════════════════════════════════════════════════════════════════
//  FALL ALERT OVERLAY
// ═════════════════════════════════════════════════════════════════

void TFTDriver::drawFallAlert(uint32_t fallAlertStart) {
    float prog = constrain((millis() - fallAlertStart) / 5000.f, 0.f, 1.f);

    // Darken every other row
    for (int y = 0; y < SCREEN_H; y += 2)
        tft.drawFastHLine(0, y, SCREEN_W, C_DARK);

    // Alert card
    _drawCard(40, 48, 240, 134, 12, 0x1082, C_ROSE);
    tft.drawRoundRect(41, 49, 238, 132, 11, 0xF041);

    // Warning triangle
    tft.fillTriangle(160, 68, 132, 116, 188, 116, C_AMBER);
    tft.fillTriangle(160, 76, 137, 112, 183, 112, C_DARK);
    drawText(155, 90, "!", 2, C_AMBER);

    // Title & body
    drawText(60,  124, "FALL DETECTED",               2, 0xFF81);
    drawText(50,  149, "Sudden motion event detected.", 1, 0xCE79);
    drawText(62,  161, "Are you okay? Alert in 5s.",   1, 0xCE79);

    // Countdown progress bar
    tft.drawRoundRect(55, 172, 210, 8, 3, C_ROSE);
    int bw = (int)(206 * prog);
    if (bw > 0) tft.fillRect(57, 174, bw, 4, C_ROSE);
}

// ═════════════════════════════════════════════════════════════════
//  GENERIC WIDGET  (legacy compatibility)
// ═════════════════════════════════════════════════════════════════

void TFTDriver::drawWidget(const char *label, int x, int y, int w, int h,
                           int value, const char *unit, uint16_t color) {
    _drawCard(x, y, w, h, 6, C_CARD_A, C_ACCENT2);
    drawText(x + 4, y + 4,  label, 1, C_SUBTEXT);
    char buf[12]; snprintf(buf, sizeof(buf), "%d", value);
    drawText(x + 4, y + 16, buf,   2, color);
    drawText(x + 4, y + h - 12, unit, 1, C_SUBTEXT);
}

// ═════════════════════════════════════════════════════════════════
//  TEXT HELPER
// ═════════════════════════════════════════════════════════════════

void TFTDriver::drawText(int16_t x, int16_t y, const char *text,
                         uint8_t size, uint16_t color, bool centered,
                         uint16_t bgColor) {
    // Two-arg setTextColor fills each character cell's background,
    // so previous digits are fully erased without a separate fillRect.
    tft.setTextColor(color, bgColor);
    tft.setTextSize(size);
    if (centered) {
        int16_t x1, y1; uint16_t tw, th;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
        x -= tw / 2;
    }
    tft.setCursor(x, y);
    tft.print(text);
    // Restore transparent mode so other callers aren't affected
    tft.setTextColor(color);
}

// ═════════════════════════════════════════════════════════════════
//  PRIVATE – SHARED LAYOUT HELPERS
// ═════════════════════════════════════════════════════════════════

void TFTDriver::_drawStatusBar(bool homeScreen, uint8_t battPct) {
    tft.fillRect(0, 0, SCREEN_W, STATUS_H, C_CARD_A);
    tft.drawLine(0, STATUS_H, SCREEN_W, STATUS_H, C_ACCENT3);

    if (homeScreen) {
        // Chip-ECG icon ─────────────────────────────────────────
        tft.drawRoundRect(4, 4, 10, 10, 1, C_ACCENT);
        for (int p : {6, 9, 12}) {
            tft.drawPixel(p, 3, C_ACCENT); tft.drawPixel(p,  2, C_ACCENT);
            tft.drawPixel(p, 14,C_ACCENT); tft.drawPixel(p, 15, C_ACCENT);
        }
        tft.drawPixel(3,  7, C_ACCENT); tft.drawPixel(2,  7, C_ACCENT);
        tft.drawPixel(3, 11, C_ACCENT); tft.drawPixel(2, 11, C_ACCENT);
        tft.drawPixel(14, 7, C_ACCENT); tft.drawPixel(15, 7, C_ACCENT);
        tft.drawPixel(14,11, C_ACCENT); tft.drawPixel(15,11, C_ACCENT);
        // ECG line through chip
        const int8_t ex[] = {1,4,5, 6, 7, 8,10,11,12,13,17};
        const int8_t ey[] = {9,9,11,5,13, 9, 9, 7,11, 9, 9};
        for (int i = 0; i < 10; i++)
            tft.drawLine(ex[i], ey[i], ex[i+1], ey[i+1], C_ROSE);
    } else {
        tft.setTextColor(C_ACCENT);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.print("< Home");
    }

    // Battery icon ────────────────────────────────────────────────
    int bx = SCREEN_W - 32, by = 4;
    tft.drawRoundRect(bx, by, 22, 10, 2, C_ACCENT2);
    tft.fillRect(bx + 22, by + 3, 3, 4, C_ACCENT2);
    int fw = max(1, (int)(20 * battPct / 100.0f));
    uint16_t fc = battPct > 50 ? C_GREEN :
                  battPct > 20 ? C_AMBER : C_ROSE;
    tft.fillRect(bx + 1, by + 1, fw, 8, fc);
}

void TFTDriver::_drawBlossom(int cx, int cy, int r,
                              uint16_t petal, uint16_t center) {
    const float angles[5] = { -90.f, -18.f, 54.f, 126.f, 198.f };
    for (int i = 0; i < 5; i++) {
        float rad = angles[i] * M_PI / 180.f;
        tft.fillCircle(cx + (int)(r * 0.6f * cosf(rad)),
                       cy + (int)(r * 0.6f * sinf(rad)),
                       r / 2, petal);
    }
    tft.fillCircle(cx, cy, r / 3, center);
}

void TFTDriver::_drawSakuraBranch(bool faded) {
    uint16_t branchCol = faded ? C_ACCENT3 : C_ACCENT2;
    uint16_t petal1    = 0xFDB6;
    uint16_t petal2    = 0xFCB3;

    // Main sinuous vertical branch
    for (int i = 0; i < 150; i++) {
        int bx = 295 + (int)(8 * sinf(i / 150.f * 2.8f));
        int by = 5 + i;
        if (bx < SCREEN_W && by < SCREEN_H)
            tft.drawPixel(bx, by, branchCol);
    }
    // Side branches
    for (int i = 0; i < 28; i++) {
        int bx = 289 - i, by = 48 + i / 2;
        if (bx >= 0) tft.drawPixel(bx, by, branchCol);
    }
    for (int i = 0; i < 26; i++) {
        int bx = 288 - i, by = 90 + i / 2;
        if (bx >= 0) tft.drawPixel(bx, by, branchCol);
    }

    if (!faded) {
        _drawBlossom(272, 64,  9, petal1, C_ACCENT2);
        _drawBlossom(265, 100, 8, petal2, C_ACCENT2);
        _drawBlossom(276, 128, 7, petal1, C_ACCENT2);
        tft.fillCircle(252, 76,  3, petal2);
        tft.fillCircle(248, 112, 3, petal1);
        tft.fillCircle(260, 148, 3, petal2);
    } else {
        _drawBlossom(272, 64,  8, C_ACCENT3, C_ACCENT3);
        _drawBlossom(265, 100, 7, C_ACCENT3, C_ACCENT3);
    }
}

// ═════════════════════════════════════════════════════════════════
//  PRIVATE – SHAPE PRIMITIVES
// ═════════════════════════════════════════════════════════════════

void TFTDriver::_drawCard(int x, int y, int w, int h,
                           int r, uint16_t fill, uint16_t border) {
    tft.fillRoundRect(x, y, w, h, r, fill);
    tft.drawRoundRect(x, y, w, h, r, border);
}

void TFTDriver::_drawHeart(int cx, int cy, int sz, uint16_t color) {
    // Two circles + triangle for a pixel-art heart
    tft.fillCircle(cx - sz / 2, cy - sz / 4, sz / 2, color);
    tft.fillCircle(cx + sz / 2, cy - sz / 4, sz / 2, color);
    tft.fillTriangle(cx - sz, cy - sz / 4,
                     cx + sz, cy - sz / 4,
                     cx,      cy + sz,     color);
}

void TFTDriver::_drawHBar(int x, int y, int w, int h,
                           float frac, uint16_t color) {
    tft.fillRect(x, y, w, h, C_ACCENT3);
    int fw = (int)(w * constrain(frac, 0.f, 1.f));
    if (fw > 0) tft.fillRect(x, y, fw, h, color);
    tft.drawRect(x, y, w, h, C_ACCENT2);
}

// ═════════════════════════════════════════════════════════════════
//  PRIVATE – GAUGE ARC
// ═════════════════════════════════════════════════════════════════

void TFTDriver::_drawGaugeArc(int cx, int cy, int r, int strokeW,
                               int startDeg, int endDeg,
                               uint16_t trackCol, uint16_t fillCol,
                               float frac) {
    frac = constrain(frac, 0.f, 1.f);
    int fillEnd = startDeg + (int)((endDeg - startDeg) * frac);

    for (int s = 0; s < strokeW; s++) {
        int rr = r - s;
        for (int d = startDeg; d <= endDeg; d += 2) {
            float rad = d * M_PI / 180.f;
            int px = cx + (int)(rr * cosf(rad));
            int py = cy + (int)(rr * sinf(rad));
            if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H)
                tft.drawPixel(px, py, d <= fillEnd ? fillCol : trackCol);
        }
    }
    // Needle-tip dot at fill end
    float tipRad = fillEnd * M_PI / 180.f;
    tft.fillCircle(cx + (int)(r * cosf(tipRad)),
                   cy + (int)(r * sinf(tipRad)),
                   strokeW / 2 + 1, fillCol);
}

// ═════════════════════════════════════════════════════════════════
//  PRIVATE – COMPASS
// ═════════════════════════════════════════════════════════════════

void TFTDriver::_drawCompass(int cx, int cy, int r, float bearing) {
    // Background & ring
    tft.fillCircle(cx, cy, r, 0xFFF9);
    tft.drawCircle(cx, cy, r,     C_ACCENT2);
    tft.drawCircle(cx, cy, r + 1, C_ACCENT3);

    // Tick marks
    for (int d = 0; d < 360; d += 20)
        tft.drawPixel(cx + (int)((r - 3) * cosf(d * M_PI / 180.f)),
                      cy + (int)((r - 3) * sinf(d * M_PI / 180.f)),
                      C_ACCENT3);

    // Erase old needle if heading changed
    if (_prevHeading >= 0 && _prevHeading != (int)bearing) {
        float oldRad = _prevHeading * M_PI / 180.f;
        int onx = cx + (int)(22 * sinf(oldRad));
        int ony = cy - (int)(22 * cosf(oldRad));
        int osx = cx - (int)(22 * sinf(oldRad));
        int osy = cy + (int)(22 * cosf(oldRad));
        tft.drawLine(cx, cy, onx, ony, 0xFFF9);
        tft.drawLine(cx, cy, osx, osy, 0xFFF9);
    }

    // Draw new needle
    float rad = bearing * M_PI / 180.f;
    int nx = cx + (int)(22 * sinf(rad)),  ny = cy - (int)(22 * cosf(rad));
    int sx = cx - (int)(22 * sinf(rad)),  sy = cy + (int)(22 * cosf(rad));
    tft.drawLine(cx, cy, nx, ny, C_ROSE);
    tft.drawLine(cx, cy, sx, sy, C_SUBTEXT);
    tft.fillCircle(cx, cy, 3, C_WHITE);

    tft.setTextColor(C_ROSE);
    tft.setTextSize(1);
    tft.setCursor(nx - 2, ny - 8);
    tft.print("N");

    _prevHeading = (int)bearing;
}

// ═════════════════════════════════════════════════════════════════
//  PRIVATE – THRESHOLD COLOUR SELECTORS
// ═════════════════════════════════════════════════════════════════

uint16_t TFTDriver::_hrColor(uint8_t hr) {
    if (hr < 50 || hr > 110) return UI_VAL_ALERT;
    if (hr < 60 || hr > 100) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}

uint16_t TFTDriver::_spo2Color(uint8_t s) {
    if (s < 90) return UI_VAL_ALERT;
    if (s < 95) return UI_VAL_WARN;
    return UI_VAL_INFO;
}

uint16_t TFTDriver::_tempColor(float t) {
    if (t < 10.f || t > 35.f) return UI_VAL_ALERT;
    if (t < 18.f || t > 30.f) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}

uint16_t TFTDriver::_humidColor(float h) {
    if (h < 20.f || h > 85.f) return UI_VAL_ALERT;
    if (h < 30.f || h > 70.f) return UI_VAL_WARN;
    return UI_VAL_INFO;
}

uint16_t TFTDriver::_co2Color(uint16_t c) {
    if (c > 1500) return UI_VAL_ALERT;
    if (c >  800) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}

uint16_t TFTDriver::_tvocColor(uint16_t t) {
    if (t > 300) return UI_VAL_ALERT;
    if (t > 100) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}

uint16_t TFTDriver::_aqiColor(uint8_t aqi) {
    if (aqi >= 4) return UI_VAL_ALERT;
    if (aqi == 3) return UI_VAL_WARN;
    return UI_VAL_GOOD;
}