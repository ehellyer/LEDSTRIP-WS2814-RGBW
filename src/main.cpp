/*
 * WS2814 RGBW LED Strip - Rainbow Spectrum Chase
 * Board  : Adeept Mega 2560 Rev3 (Arduino Mega 2560)
 * IDE    : PlatformIO (VSCode)
 * Library: Adafruit NeoPixel (declared in platformio.ini)
 *
 * Effect: A full HSV rainbow gradient spread across all LEDs,
 *         continuously scrolling to create a smooth chasing loop.
 *
 * Wiring:
 *   Strip DIN    → Arduino Pin 6  (via 470 Ω series resistor)
 *   Strip BACKUP → leave unconnected (single-strip use)
 *   Strip +12 V  → 12 V external supply (+)
 *   Strip GND    → 12 V supply (−)  AND  Arduino GND  (common ground!)
 *   1000 µF / 16 V capacitor across the supply leads at the strip connector
 */

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

// ── Configuration ────────────────────────────────────────────────────────────
// DIN pin (via 470 Ω resistor)
static constexpr uint8_t DATA_PIN = 6;  

// Actually the number of LED IC's in your strip.  (I have 3 LED per IC in my WS2814, so 150 actual LEDs)
static constexpr uint16_t NUM_LEDS = 50; 

// 0–255  (128 = 50 %)
static constexpr uint8_t BRIGHTNESS = 200; 

// 16ms works out to be 60 fps
static constexpr uint8_t FRAME_MS = 16;  

// Hue advance per frame; larger = faster chase
static constexpr uint16_t HUE_STEP = 250;

// Width of the white fade zone centred on hue=0 (red/wrap point).
// 8192 = 1/8 of the circle (~45°).  Increase for a wider white blend.
static constexpr uint16_t WHITE_ZONE = (16384 * 2);

// Set true for a static colour test; false to run the scrolling animation.
static constexpr bool TEST_MODE = false;

// Serial baud rate for debug output
static constexpr uint32_t SERIAL_BAUD = 115200;
// ─────────────────────────────────────────────────────────────────────────────

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_RGBW + NEO_KHZ800);

static uint16_t hueOffset = 0;
static unsigned long lastFrame = 0;

// Convert R,G,B (0–255 each) to a hue in the NeoPixel 16-bit range (0–65535).
// Returns 0 for achromatic colours (grey/white/black).
uint16_t rgbToHue(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t maxC = max(r, max(g, b));
  uint8_t minC = min(r, min(g, b));
  uint8_t delta = maxC - minC;

  if (delta == 0) return 0; // achromatic

  int32_t hue60; // hue in units of 1/60th of a circle (0–360 scaled)
  if      (maxC == r) hue60 = (int32_t)(g - b) * 60 / delta;
  else if (maxC == g) hue60 = (int32_t)(b - r) * 60 / delta + 120;
  else                hue60 = (int32_t)(r - g) * 60 / delta + 240;

  if (hue60 < 0) hue60 += 360;

  // Scale 0–359 → 0–65535
  return (uint16_t)((uint32_t)hue60 * 65536UL / 360);
}

uint16_t getHue(uint16_t position) {
    return (position * 65536UL) / NUM_LEDS; 
}

// Returns the mapped adafruit neo pixel 32-bit color value for a given hue, including white blending.
uint32_t getLEDColor(uint16_t hue) {
  
  uint32_t color = strip.gamma32(strip.ColorHSV(hue));

  uint8_t r = (color >> 24) & 0xFF;
  uint8_t g = (color >> 16) & 0xFF;
  uint8_t b = (color >> 8) & 0xFF;
  uint8_t w = color & 0xFF;

  return Adafruit_NeoPixel::Color(r, g, b, w);
}

void updateLEDStrip() {
  strip.clear();
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = getHue(i) + hueOffset; // uint16_t wraps naturally at
    strip.setPixelColor(i, getLEDColor(hue));
  }
  strip.show();
}

void setup() {

  Serial.begin(SERIAL_BAUD);
  Serial.println("Setup Begin");
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  
  if (TEST_MODE) {
    
    const uint32_t testColors[] = {
        rgbToHue(255,   0,   0),  // Red
        rgbToHue(  0, 255,   0),  // Green
        rgbToHue(  0,   0, 255),  // Blue
    };
    const unsigned long delay_ms = 1500;
    
    for (uint8_t i = 0; i < sizeof(testColors) / sizeof(testColors[0]); i++) {
      Serial.print("Setting LED color 0x");
      Serial.println(testColors[i], HEX);
      strip.clear();
        for (uint16_t pos = 0; pos < NUM_LEDS; pos++) {
          uint32_t color = getLEDColor(testColors[i]);
            strip.setPixelColor(pos, color);
        }
        strip.show();
        delay(delay_ms);
    }
  }
}

void loop() {
  if (TEST_MODE)
    return;

  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS)
    return;
  lastFrame = now;

  updateLEDStrip();
  hueOffset += HUE_STEP; 
}
