#include "frameStream.hpp"
#include <Arduino.h>

void streamFrameIfRequested(LGFX_Sprite &canvas) {
  // --- Did the PC ask for a frame? ---
  bool requested = false;
  while (Serial.available()) {          // read everything waiting, so requests don't pile up
    if (Serial.read() == 'F') requested = true;
  }
  if (!requested) return;

  // --- Send it ---
  // setup() sets the send timeout to 0, which drops bytes when the USB buffer
  // is full. That's fine for text, but a frame with missing bytes is garbage,
  // so let it wait while we send, then put it back.
  Serial.setTxTimeoutMs(250);

  uint16_t width  = canvas.width();
  uint16_t height = canvas.height();
  Serial.write((const uint8_t *)"KOIF", 4);
  Serial.write((const uint8_t *)&width, 2);    // little-endian, which is what the ESP32 uses
  Serial.write((const uint8_t *)&height, 2);
  Serial.write((const uint8_t *)canvas.getBuffer(), width * height * 2);

  Serial.setTxTimeoutMs(0);
}
