// firmware/include/tft_driver.h
#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// ── SPI Pins ──────────────────────────────────────────────
#define TFT_MOSI 19
#define TFT_MISO 21
#define TFT_SCK   5
#define TFT_CS   25
#define TFT_DC   32
#define TFT_RST  12

// ── Screen ────────────────────────────────────────────────
#define SCREEN_W  320
#define SCREEN_H  240

// ── Soft Rose / Blush Palette (RGB565) ───────────────────
//   UI_BG       ~RGB(248,212,232)  blush-pink background
//   UI_STATUSBAR ~RGB(248,196,216) deeper rose header strip
//   UI_CARD      white card faces
//   UI_BORDER    soft pink card border
//   UI_ACCENT    vivid rose (clock, heartbeat pulse)
//   UI_ACCENT_SOFT muted rose (resting heart icon)
#define UI_BG           0xFF5D
#define UI_STATUSBAR    0xFF1B
#define UI_CARD         0xFFFF
#define UI_BORDER       0xFCB6
#define UI_ACCENT       0xF813
#define UI_ACCENT_SOFT  0xFCF3

#define UI_TEXT_DARK    0x2965   // dark blue-grey  – values / titles
#define UI_TEXT_MED     0x8C71   // warm mid-grey   – labels
#define UI_TEXT_LIGHT   0xC618   // light grey      – units

// Sensor value colours (threshold-driven, used on white cards)
#define UI_VAL_GOOD     0x0640   // teal-green
#define UI_VAL_WARN     0xFD20   // amber
#define UI_VAL_ALERT    0xF800   // red
#define UI_VAL_INFO     0x065F   // sky-blue (SpO2, humidity, accel bars)

// AQI level colours
#define AQI_COL_1       0x07E0   // green
#define AQI_COL_2       0xAFE5   // yellow-green
#define AQI_COL_3       0xFFE0   // yellow
#define AQI_COL_4       0xFD20   // orange
#define AQI_COL_5       0xF800   // red

// Legacy GFX colour aliases (kept for any existing callsites)
#define GxTFT_BLACK     0x0000
#define GxTFT_WHITE     0xFFFF
#define GxTFT_RED       0xF800
#define GxTFT_GREEN     0x07E0
#define GxTFT_BLUE      0x001F
#define GxTFT_CYAN      0x07FF
#define GxTFT_MAGENTA   0xF81F
#define GxTFT_YELLOW    0xFFE0
#define GxTFT_ORANGE    0xFC00

// Mini compass position (top-right corner of status bar)
#define COMPASS_CX  307
#define COMPASS_CY   10
#define COMPASS_R    9

enum Screen { SCREEN_HOME, SCREEN_ENV };

struct DisplayData {
    // AHT21
    float    temperature;
    float    humidity;
    // ENS160
    uint16_t eCO2;
    uint16_t eTVOC;
    uint8_t  aqi;
    // MAX30102
    uint8_t  heartRate;
    uint8_t  spo2;
    bool     heartBeat;
    // MPU6500
    float    accelX, accelY, accelZ;
    bool     fallDetected;
};

class TFTDriver {
public:
    bool begin();
    void drawSplash();
    void drawHomeScreen_BG();                                   // static layer – call once
    void updateHomeScreen(const DisplayData &d, bool heartBeatTick);
    void drawEnvScreen_BG();
    void updateEnvScreen(const DisplayData &d);
    void drawFallAlert(uint32_t fallAlertStart);
    void drawWidget(const char *label, int x, int y, int w, int h,
                    int value, const char *unit, uint16_t color);
    void drawText(int16_t x, int16_t y, const char *text,
                  uint8_t size, uint16_t color, bool centered = false);
    void clearScreen(uint16_t color = GxTFT_BLACK);

    int _prevHeading = -1;   // for compass needle erase

private:
    Adafruit_ILI9341 tft{TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO};

    // Drawing primitives
    void     _drawCard(int x, int y, int w, int h, uint16_t border);
    void     _drawHeart(int cx, int cy, int sz, uint16_t color);
    void     _drawMiniCompass(int cx, int cy, int r, int heading);
    void     _drawHBar(int x, int y, int w, int h, float frac, uint16_t color);

    // Env-screen helpers (legacy panel style)
    void     _drawPanelShell(int x, int y, int w, int h,
                              uint16_t accent, const char *label);
    void     _updatePanelValue(int x, int y, int w,
                                const char *valStr, const char *unit,
                                uint16_t color, bool alert = false);

    // Threshold colour selectors
    uint16_t _hrColor(uint8_t hr);
    uint16_t _spo2Color(uint8_t s);
    uint16_t _tempColor(float t);
    uint16_t _humidColor(float h);
    uint16_t _co2Color(uint16_t c);
    uint16_t _tvocColor(uint16_t t);
    uint16_t _aqiColor(uint8_t aqi);
}