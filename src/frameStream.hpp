#pragma once
#include <LovyanGFX.hpp>

// Sends the canvas to the PC over USB serial, for tools/viewer.py.
//
// The PC asks for a frame by sending the letter 'F'. Only then does the board
// send one, so when the viewer isn't running this costs nothing.
//
// Each frame is:  "KOIF"  width (2 bytes)  height (2 bytes)  pixels (w * h * 2 bytes)
// The pixels are the sprite's raw buffer: RGB565, two bytes each, high byte first.
//
// Call once per frame, after drawing.
void streamFrameIfRequested(LGFX_Sprite &canvas);
