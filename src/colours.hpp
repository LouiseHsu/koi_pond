#pragma once
#include <LovyanGFX.hpp>




// Blend two RGB565 colours. t = 0 gives a, t = 1 gives b.
constexpr uint16_t lerp565(uint16_t a, uint16_t b, float t) {
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  int r = ar + (br - ar) * t;
  int g = ag + (bg - ag) * t;
  int bl = ab + (bb - ab) * t;
  return (r << 11) | (g << 5) | bl;
}

// constexpr uint16_t POND_COLOUR   = lgfx::color565(140, 210, 255);
constexpr uint16_t POND_COLOUR   = lgfx::color565(48 , 225, 185);

constexpr uint16_t SHADOW_COLOUR = lgfx::color565(84, 126, 153);
constexpr uint16_t BLACKLIST_COLOUR =  lgfx::color565(168, 132, 243);
constexpr uint16_t POND_SHADOW_COLOUR = lerp565(POND_COLOUR, SHADOW_COLOUR, 0.5f);