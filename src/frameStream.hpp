#pragma once
#include <LovyanGFX.hpp>

// Sends the canvas to the PC over USB serial, for tools/viewer.py.
//
// The PC sends one-letter commands:
//   'F'  send me a frame
//   'R'  start recording mode
//   'N'  back to normal mode
//
// Normal mode: a frame is sent only when the PC asks, so when the viewer isn't
// running this costs nothing.
//
// Recording mode: the board waits for the PC to ask after EVERY frame, so every
// frame it draws gets captured. loop() also switches to a fake clock (each
// frame exactly 1 / RECORD_FPS seconds apart), so the recording plays back
// smoothly even though the board runs slowly while sending.
//
// Each frame is:  "KOIF"  width (2 bytes)  height (2 bytes)  pixels (w * h * 2 bytes)
// The pixels are the sprite's raw buffer: RGB565, two bytes each, high byte first.

constexpr int RECORD_FPS = 25;   // GIF frame delays are in 1/100 s, so 25 fps = exactly 4

// Call once per frame, after drawing.
void streamFrameIfRequested(LGFX_Sprite &canvas);

// True while the viewer is recording. loop() uses this to switch clocks.
bool frameStreamIsRecording();
