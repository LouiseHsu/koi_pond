#include "frameStream.hpp"
#include <Arduino.h>

static bool recording = false;

// If the viewer goes quiet for this long while recording (closed, crashed,
// unplugged), give up!!
static constexpr uint32_t RECORD_TIMEOUT_MS = 2000;

bool frameStreamIsRecording() {
  return recording;
}

// Reads every command waiting from the PC. Returns true if a frame was asked for.
static bool readCommands() {
  bool requested = false;
  while (Serial.available()) {
    switch (Serial.read()) {
      case 'F': requested = true;  break;
      case 'R': recording = true;  break;
      case 'N': recording = false; break;
    }
  }
  return requested;
}

static void sendFrame(LGFX_Sprite &canvas) {
  Serial.setTxTimeoutMs(250);

  uint16_t width  = canvas.width();
  uint16_t height = canvas.height();
  Serial.write((const uint8_t *)"KOIF", 4);
  Serial.write((const uint8_t *)&width, 2);    // little-endian, which is what the ESP32 uses
  Serial.write((const uint8_t *)&height, 2);
  Serial.write((const uint8_t *)canvas.getBuffer(), width * height * 2);

  Serial.setTxTimeoutMs(0);
}

void streamFrameIfRequested(LGFX_Sprite &canvas) {
  bool requested = readCommands();

  if (recording && !requested) {
    uint32_t waitStart = millis();
    while (recording && !requested) {
      if (millis() - waitStart > RECORD_TIMEOUT_MS) {
        recording = false;   // viewer gone
        break;
      }
      delay(1);
      requested = readCommands();
    }
  }

  if (requested) sendFrame(canvas);
}
