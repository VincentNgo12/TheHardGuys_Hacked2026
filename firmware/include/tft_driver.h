// firmware/include/tft_driver.h
// ─────────────────────────────────────────────────────────────────
//  "Sakura Band" TFT Driver  –  ILI9341 320×240  (ESP32 / Arduino)
//  Palette + API updated to match the Sakura GUI design.
// ─────────────────────────────────────────────────────────────────
#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <math.h>

// ── SPI Pins ──────────────────────────────────────────────────────
#define TFT_MOSI 19
#define TFT_MISO 21
#define TFT_SCK   5
#define TFT_CS   25
#define TFT_DC   32
#define TFT_RST  12

// ── Screen dimensions ─────────────────────────────────────────────
#define SCREEN_W   320
#define SCREEN_H   240
#define STATUS_H    18   // height of the top status bar

// ── Sakura Colour Palette (RGB565) ────────────────────────────────
//
//  C_BG        #fce8e8  blush background
//  C_CARD      white card face
//  C_CARD_A    frosted / semi-white card
//  C_ACCENT    #d4687a  deep rose  – primary highlight, compass N,
//                                    heart icon, AQI critical
//  C_ACCENT2   #e8a0a8  soft rose  – card borders, secondary text
//  C_ACCENT3   #f7c5cc  petal light – dividers, tick marks,
//                                     gauge track, faded branch
//  C_TEXT      #4a2030  dark plum   – primary value text
//  C_SUBTEXT   #b07080  dusty pink  – labels / units
//  C_ROSE      #e84060  vivid rose  – heart shape, fall alert,
//                                     SpO₂ gauge, progress bar
//  C_BLUSH     #f0a0b0  blush highlight (reserved)
//  C_MINT      #7dd4c0  mint  – humidity gauge fill
//  C_AMBER     #e8a050  amber – warning (TVOC warn, fall triangle)
//  C_GREEN     #80c080  green – good AQI / battery ok
//  C_DARK      #18c300  very dark – overlay rows, behind-overlay bg
//  C_WHITE / C_BLACK   convenience aliases

#define C_BG        0xFEF5
#define C_CARD      0xFFFF
#define C_CARD_A    0xFFFB
#define C_ACCENT    0xD30F
#define C_ACCENT2   0xE894
#define C_ACCENT3   0xFDB6
#define C_TEXT      0x4908
#define C_SUBTEXT   0xB3B1
#define C_ROSE      0xE204
#define C_BLUSH     0xF054
#define C_MINT      0x7E98
#define C_AMBER     0xE284
#define C_GREEN     0x8300
#define C_DARK      0x18C3
#define C_WHITE     0xFFFF
#define C_BLACK     0x0000

// Sensor threshold colours – used by the helper selectors below
//   VAL_GOOD  → C_GREEN   (normal / safe range)
//   VAL_WARN  → C_AMBER   (caution)
//   VAL_ALERT → C_ROSE    (action required)
//   VAL_INFO  → C_MINT    (informational, e.g. SpO₂ / humidity)
#define UI_VAL_GOOD   C_GREEN
#define UI_VAL_WARN   C_AMBER
#define UI_VAL_ALERT  C_ROSE
#define UI_VAL_INFO   C_MINT

// Legacy GFX colour aliases (kept for any existing call-sites)
#define GxTFT_BLACK   0x0000
#define GxTFT_WHITE   0xFFFF
#define GxTFT_RED     0xF800
#define GxTFT_GREEN   0x07E0
#define GxTFT_BLUE    0x001F
#define GxTFT_CYAN    0x07FF
#define GxTFT_MAGENTA 0xF81F
#define GxTFT_YELLOW  0xFFE0
#define GxTFT_ORANGE  0xFC00

// ── Screen identifiers ────────────────────────────────────────────
enum Screen { SCREEN_HOME, SCREEN_ENV };

// ── Sensor data bundle ────────────────────────────────────────────
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
    // Navigation / system
    float    bearing;   // compass heading, degrees
    uint8_t  battPct;   // 0–100
    // Clock  (set externally before calling update)
    uint8_t  hour, min, sec;
    char     dateStr[20];
};

// ═════════════════════════════════════════════════════════════════
//  TFTDriver
// ═════════════════════════════════════════════════════════════════
class TFTDriver {
public:
    // ── Lifecycle ──────────────────────────────────────────────
    bool begin();
    void drawSplash();
    void clearScreen(uint16_t color = C_BG);

    // ── Home Screen (Screen 0) ─────────────────────────────────
    //  drawHomeScreen_BG()  –  static layer, call once on screen switch
    //  updateHomeScreen()   –  dynamic layer, call each render tick
    void drawHomeScreen_BG();
    void updateHomeScreen(const DisplayData &d, bool heartBeatTick);

    // ── Air / Env Screen (Screen 1) ───────────────────────────
    void drawEnvScreen_BG();
    void updateEnvScreen(const DisplayData &d);

    // ── Fall Alert Overlay ────────────────────────────────────
    //  Draws the darkened overlay + alert card on top of whatever
    //  is currently on screen.
    //  fallAlertStart – millis() timestamp when fall was detected.
    void drawFallAlert(uint32_t fallAlertStart);

    // ── Generic primitives (exposed for Platform IO tasks) ────
    void drawWidget(const char *label, int x, int y, int w, int h,
                    int value, const char *unit, uint16_t color);
    void drawText(int16_t x, int16_t y, const char *text,
                  uint8_t size, uint16_t color, bool centered = false,
                  uint16_t bgColor = C_BG);   // bgColor makes glyphs self-erasing

    // Public state tracked across frames
    int  _prevHeading = -1;   // used to erase old compass needle

private:
    Adafruit_ILI9341 tft{TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO};

    // ── Shared layout helpers ──────────────────────────────────
    void _drawStatusBar(bool homeScreen, uint8_t battPct);
    void _drawSakuraBranch(bool faded);
    void _drawBlossom(int cx, int cy, int r, uint16_t petal, uint16_t center);

    // ── Card / shape primitives ───────────────────────────────
    void _drawCard(int x, int y, int w, int h,
                   int r, uint16_t fill, uint16_t border);
    void _drawHeart(int cx, int cy, int sz, uint16_t color);

    // ── Gauge primitives ──────────────────────────────────────
    //  Arc gauge: startDeg/endDeg in standard math degrees
    //  (0 = right, CCW positive).  frac = 0..1.
    void _drawGaugeArc(int cx, int cy, int r, int strokeW,
                       int startDeg, int endDeg,
                       uint16_t trackCol, uint16_t fillCol, float frac);

    //  Horizontal bar (legacy, kept for accel bars)
    void _drawHBar(int x, int y, int w, int h,
                   float frac, uint16_t color);

    // ── Compass (Home screen, top-right) ──────────────────────
    void _drawCompass(int cx, int cy, int r, float bearing);

    // ── Threshold colour selectors ────────────────────────────
    uint16_t _hrColor(uint8_t hr);
    uint16_t _spo2Color(uint8_t s);
    uint16_t _tempColor(float t);
    uint16_t _humidColor(float h);
    uint16_t _co2Color(uint16_t c);
    uint16_t _tvocColor(uint16_t t);
    uint16_t _aqiColor(uint8_t aqi);
};