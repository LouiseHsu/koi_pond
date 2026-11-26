#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "pondConfig.hpp"
#include "colours.hpp"
#include "spriteLoader.hpp"
#include "koi.hpp"
#include "koi_patterns.h"
#include "waterEffects.hpp"
#include "frameStream.hpp"

// =============================================================================
// Display hardware (ST7789 over SPI)
// =============================================================================

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI      _bus;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 80000000; // Lowered to 20MHz for breadboard stability
      cfg.pin_sclk   = 2;  // CLK
      cfg.pin_mosi   = 1;  // DIN
      cfg.pin_miso   = -1;
      cfg.pin_dc     = 4;  // DC
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();
      cfg.pin_cs       = 3;  // CS
      cfg.pin_rst      = 5;  // RST
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.invert       = true;
      _panel.config(cfg);
    }

    setPanel(&_panel);
  }
};

// =============================================================================
// Globals / state
// =============================================================================

// --- Display ---
LGFX        tft;
LGFX_Sprite underwater(&tft);
LGFX_Sprite screen(&tft);

// --- Pond ---
SpriteLoader patternTextures[NUM_KOI_PATTERNS];
WaterEffects water;
Koi          kois[5];
Koi         *drawOrder[std::size(kois)];

static float nextPoke = random(0.0f, 3.0f);

// --- Timing ---
uint32_t lastTime = 0;
float    pondTime = 0;  // seconds of pond time + follows real time, except while recording

// =============================================================================
// Timing
// =============================================================================

float updateDeltaTime() {
  uint32_t currTime = millis();
  float dt = (currTime - lastTime) / 1000.0f;  // float division, in seconds
  lastTime = currTime;
  if (dt > 0.1f) dt = 0.1f;  // avoid a huge jump on the first frame
  return dt;
}

// =============================================================================
// Frame timing (debug)
// =============================================================================

struct FrameTimes {
  uint32_t clear   = 0;
  uint32_t shadows = 0;
  uint32_t kois    = 0;
  uint32_t push    = 0;
  uint32_t frames  = 0;
};
FrameTimes frameTimes;
uint32_t   lastReport = 0;

void reportFrameTimes() {
  if (millis() - lastReport < 1000) return;
  lastReport = millis();

  uint32_t n = frameTimes.frames;
  if (n == 0) return;
  uint32_t total = (frameTimes.clear + frameTimes.shadows + frameTimes.kois + frameTimes.push) / n;

  Serial.printf("clear %5lu us | shadows %5lu us | kois %5lu us | push %5lu us | total %5lu us (%.1f fps)\n",
                frameTimes.clear / n, frameTimes.shadows / n, frameTimes.kois / n, frameTimes.push / n,
                total, 1e6f / total);

  frameTimes = FrameTimes();  // reset for da next second
}

// =============================================================================
// Arduino entry points
// =============================================================================

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  randomSeed(esp_random());

  // --- Screen ---
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // --- Canvas ---
  underwater.setColorDepth(16);
  underwater.createSprite(CANVAS_W, CANVAS_H);

  screen.setColorDepth(16);
  screen.createSprite(CANVAS_W, CANVAS_H);

  // --- Koi ---
  for (int i = 0; i < NUM_KOI_PATTERNS; i++) {
    const char *err;
    if (!patternTextures[i].loadTga(KOI_PATTERNS[i].data, KOI_PATTERNS[i].length, err)) {
      Serial.printf("%s: %s\n", KOI_PATTERNS[i].name, err);
    }
  }

  for (Koi &koi : kois) {
    koi.begin(patternTextures[random(NUM_KOI_PATTERNS)], CANVAS_W, CANVAS_H);
  }

  for (size_t i = 0; i < std::size(kois); i++) drawOrder[i] = &kois[i];
  std::sort(std::begin(drawOrder), std::end(drawOrder),
            [](const Koi *a, const Koi *b) { return a->depth > b->depth; });

  // --- Water ---
  water.begin();
}

void loop() {
  uint32_t frameStart = micros();

  underwater.fillScreen(POND_COLOUR);
  uint32_t afterClear = micros();

  // --- Pond clock ---
  // normally dt is in real time. While viewer.py records, every frame is
  // exactly 1 / RECORD_FPS seconds apart instead, so the recording plays
  // even tho the board runs slowly while sending frames.
  float dt = updateDeltaTime();
  if (frameStreamIsRecording()) dt = 1.0f / RECORD_FPS;
  pondTime += dt;
  float t = pondTime;

  // --- Simulate ---
  water.updateRipples();
  water.updateLilyPads(dt);

  nextPoke -= dt;
  if (nextPoke < 0.0f) {
    water.startRippleAt((int)random(2, CANVAS_W - 3), (int)random(2, CANVAS_H - 3), 300);
    nextPoke = random(0.0f, 5.0f);
  }

  // --- Underwater ---
  water.updateCausticsBottom(underwater, t);

  for (Koi &koi : kois) {
    koi.update(dt, t);
    koi.drawShadow(underwater);
  }
  uint32_t afterShadows = micros();

  for (Koi *koi : drawOrder) {
    koi->drawFish(underwater);
  }
  water.drawLilyPadShadows(underwater, t, dt);
  water.updateCausticsTop(underwater, t);
  uint32_t afterKois = micros();

  // --- Surface ---
  water.drawSurface(underwater, screen);
  water.drawLilyPads(screen, t, dt);

  screen.pushRotateZoom(&tft, 120, 160, 0, 2, 2);
  uint32_t afterPush = micros();

  // --- Frame timing ---
  frameTimes.clear   += afterClear   - frameStart;
  frameTimes.shadows += afterShadows - afterClear;
  frameTimes.kois    += afterKois    - afterShadows;
  frameTimes.push    += afterPush    - afterKois;
  frameTimes.frames++;
  reportFrameTimes();

  // --- PC viewer (tools/viewer.py) ---
  // Not counted in the frame timing above, so the fps it prints is the
  // display's fps without the viewer.
  streamFrameIfRequested(screen);
}
