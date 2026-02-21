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
#define COL_BG         0x0B0F
#define COL_CARD       0x1082
#define COL_ACCENT     0x07E8
#define COL_ACCENT2    0x3C9F
#define COL_TEXT       0xEF7D
#define COL_SUBTEXT    0x8410
#define COL_RED        0xF800
#define COL_AMBER      0xFD00
#define COL_GREEN      0x07E0
#define COL_HEARTPULSE 0xF810

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
    void drawHomeScreen_BG();
    void updateHomeScreen(const DisplayData &d, bool heartBeatTick);
    void drawEnvScreen_BG();
    void updateEnvScreen (const DisplayData &d);
    void drawFallAlert (uint32_t fallAlertStart);

    // ── Helpers (public so task can use them if needed) ───
    void drawText(int16_t x, int16_t y, const char *text,
                  uint8_t size, uint16_t color, bool centered = false);
    void clearScreen(uint16_t color = COL_BG);

private:
    Adafruit_ILI9341 tft{TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO};

    // ── Widget draw calls ─────────────────────────────────
    void drawHeartWidget   (uint8_t bpm, bool beat);
    void drawEnvCard       (uint16_t co2, uint16_t tvoc,
                            float temp, float rh, uint8_t spo2);
    void drawAppButton     (int16_t x, int16_t y, uint16_t w, uint16_t h,
                            uint16_t bg, const char *icon, const char *label);
    void drawCompassWidget (float heading);
    void drawGaugeArc      (int16_t cx, int16_t cy, int16_t r,
                            float minVal, float maxVal, float val,
                            uint16_t color, const char *label, const char *unit);
    void drawHeartIcon     (int16_t x, int16_t y, uint16_t color);
    void drawStatusBar     ();
};