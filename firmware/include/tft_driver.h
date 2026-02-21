// include/tft_driver.h
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

// ── Colour Palette (RGB565) ───────────────────────────────
#define GxTFT_BLACK     0x0000
#define GxTFT_WHITE     0xFFFF
#define GxTFT_RED       0xF800
#define GxTFT_GREEN     0x07E0
#define GxTFT_BLUE      0x001F
#define GxTFT_CYAN      0x07FF
#define GxTFT_MAGENTA   0xF81F
#define GxTFT_YELLOW    0xFFE0
#define GxTFT_ORANGE    0xFC00

#define UI_BG           0x0841   // near-black
#define UI_PANEL        0x1082   // dark card
#define UI_BORDER       0x2945   // subtle border
#define UI_GRAY         0x8410   // dim label text
#define UI_DARKGRAY     0x4208
#define UI_WHITE        0xFFFF
#define UI_CYAN         0x07FF
#define UI_RED          0xF800
#define UI_GREEN        0x07E0
#define UI_YELLOW       0xFFE0
#define UI_ORANGE       0xFD20
#define UI_PURPLE       0xC81F
#define UI_CLOCK_FACE   0x1494   // dark blue-grey clock dial
#define UI_CLOCK_RIM    0x2965

#define CLK_CX  160
#define CLK_CY   95
#define CLK_R    70

// ── Screen Dimensions ─────────────────────────────────────
#define SCREEN_W 320
#define SCREEN_H 240

// ── Screen States ─────────────────────────────────────────
enum Screen { SCREEN_HOME, SCREEN_ENV };

// ── Display Data fed in by task_display ──────────────────
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

    // ── Screens ───────────────────────────────────────────
    void drawSplash();
    void drawWidget(const char* label, int x, int y, int w, int h, int value, const char* unit, uint16_t color);
    void updateHomeScreen(const DisplayData &d, bool heartBeatTick);
    void drawEnvScreen_BG();
    void updateEnvScreen (const DisplayData &d);
    void drawFallAlert (uint32_t fallAlertStart);
    // New public method
    void drawHomeScreen_BG();

    // ── Helpers (public so task can use them if needed) ───
    void drawText(int16_t x, int16_t y, const char *text,
                  uint8_t size, uint16_t color, bool centered = false);
    void clearScreen(uint16_t color = GxTFT_BLACK);
    // Private members
    float   _prevSecAngle  = -1;
    float   _prevMinAngle  = -1;
    float   _prevHourAngle = -1;
    int     _prevHeading   = -1;

    // Private helpers
    void _drawHand(float angleDeg, int length, int thickness, uint16_t color);
    void _redrawTicks();
    void _drawPanelShell(int x, int y, int w, int h, uint16_t accent, const char *label);
    void _updatePanelValue(int x, int y, int w, const char *valStr, const char *unit,
                            uint16_t color, bool alert = false);

private:
    Adafruit_ILI9341 tft{TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO};
};