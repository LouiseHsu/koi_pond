#include "waterEffects.hpp"
#include "colours.hpp"
#include "caustic_tile_tga.h"
#include "lilypads.h"

// Caustic colours, worked out by the compiler at build time (lerp565 is
// constexpr). They follow POND_COLOUR and the *_AMOUNT constants automatically.
static constexpr uint16_t SOFT_CAUSTIC_COLOUR   = lerp565(POND_COLOUR, TFT_WHITE, WaterEffects::SOFT_CAUSTIC_AMOUNT);
static constexpr uint16_t BRIGHT_CAUSTIC_COLOUR = lerp565(POND_COLOUR, TFT_WHITE, WaterEffects::BRIGHT_CAUSTIC_AMOUNT);

// =============================================================================
// Setup
// =============================================================================

void WaterEffects::begin() {
  const char *err;
  causticTexture.loadTga(caustic_tile_tga, caustic_tile_tga_len, err);
  plainLilyPad.loadTga(lilypad_tga, lilypad_tga_len, err);
  pinkLilyPad.loadTga(flowerlilypad_tga, flowerlilypad_tga_len, err);
  whiteLilyPad.loadTga(flowerlilypad2_tga, flowerlilypad2_tga_len, err);


  tile.setColorDepth(16);
  tile.createSprite(causticTexture.width(), causticTexture.height());   // match the texture size
  copyTextureToSprite(causticTexture, tile);
}

void WaterEffects::copyTextureToSprite(const SpriteLoader &texture, LGFX_Sprite &target) {
  for (int row = 0; row < texture.height(); row++) {
    for (int column = 0; column < texture.width(); column++) {
      uint16_t colour = texture.isOpaque(row, column) ? texture.colourAt(row, column)
                                                      : BLACKLIST_COLOUR;
      target.drawPixel(column, row, colour);   // drawPixel is (x, y) = (column, row)
    }
  }
}

void WaterEffects::drawTexture(const SpriteLoader &texture, LGFX_Sprite &canvas, int16_t offset_x, int16_t offset_y) {
  for (int row = 0; row < texture.height(); row++) {
    for (int column = 0; column < texture.width(); column++) {
      if (!texture.isOpaque(row, column)) continue;
      if (texture.colourAt(row, column) == BLACKLIST_COLOUR) {
        uint16_t under = canvas.readPixel(column + offset_x, row + offset_y);
        canvas.drawPixel(column + offset_x, row + offset_y, lerp565(under, TFT_WHITE, 0.5f));
        continue;
      }
      else {
        canvas.drawPixel(column + offset_x, row + offset_y, texture.colourAt(row, column));
    }
    }
  }
}

void WaterEffects::drawShadow(const SpriteLoader &texture, LGFX_Sprite &canvas, int16_t offset_x, int16_t offset_y) {
  int offset = 8;
    for (int row = offset_y; row < offset_y + texture.height(); row++) {
    for (int column = offset_x; column < offset_x + texture.width(); column++) {
      if (!texture.isOpaque(row - offset_y, column - offset_x)) continue;
      uint16_t under = canvas.readPixel(column + offset, row + offset);
      if (under == POND_COLOUR || under == POND_SHADOW_COLOUR || under == BRIGHT_CAUSTIC_COLOUR || under == SOFT_CAUSTIC_COLOUR) {
        // shouldnt be on fish
        canvas.drawPixel(column + offset, row + offset, POND_SHADOW_COLOUR);
      }
      else {
        
        canvas.drawPixel(column + offset, row + offset, lerp565(under, POND_SHADOW_COLOUR, 0.2f));
      }
    }
  }
}

// =============================================================================
// Per-frame
// =============================================================================

// Like a % n, but never negative (C++ % keeps the sign of a, so -3 % 32 = -3).
static int wrapIndex(int a, int n) {
  return (a & (n - 1));
}

// Is this texel one of the light lines? (The background is the purple key
// colour, so being opaque isn't enough on its own.)
bool WaterEffects::isLight(int row, int column) {
  return causticTexture.isOpaque(row, column)
      && causticTexture.colourAt(row, column) != BLACKLIST_COLOUR;
}

// Bottom
void WaterEffects::updateCausticsBottom(LGFX_Sprite &canvas, float t) {

  // --- How far each sheet has drifted by now ---
  int offsetAx = (int)(t * SHEET_A_SPEED_X);
  int offsetAy = (int)(t * SHEET_A_SPEED_Y);
  int offsetBx = (int)(t * SHEET_B_SPEED_X);
  int offsetBy = (int)(t * SHEET_B_SPEED_Y);

  int textureW = causticTexture.width();
  int textureH = causticTexture.height();

  for (int y = 0; y < canvas.height(); y++) {

    int rowA = wrapIndex(y + offsetAy, textureH);
    int rowB = wrapIndex(y + offsetBy, textureH);
    for (int x = 0; x < canvas.width(); x++) {


      bool a = isLight(rowA, wrapIndex(x + offsetAx, textureW));
      bool b = isLight(rowB, wrapIndex(x + offsetBx, textureW));

      if (a && b)      canvas.drawPixel(x, y, BRIGHT_CAUSTIC_COLOUR);  // lines cross
      else if (a || b) canvas.drawPixel(x, y, SOFT_CAUSTIC_COLOUR);    // one line
    }
  }
}

void WaterEffects::updateCausticsTop(LGFX_Sprite &canvas, float t) {
  uint16_t brightColour = lerp565(POND_COLOUR, TFT_WHITE, 0.85);

  // --- How far each sheet has drifted by now ---
  int offsetAx = (int)(t * SHEET_A_SPEED_X);
  int offsetAy = (int)(t * SHEET_A_SPEED_Y);
  int offsetBx = (int)(t * SHEET_B_SPEED_X);
  int offsetBy = (int)(t * SHEET_B_SPEED_Y);

  int textureW = causticTexture.width();
  int textureH = causticTexture.height();

  for (int y = 0; y < canvas.height(); y++) {
    // Texture row for each sheet. Wrapping with % is what makes it tile.
    int rowA = wrapIndex(y + offsetAy, textureH);
    int rowB = wrapIndex(y + offsetBy, textureH);

    for (int x = 0; x < canvas.width(); x++) {
      
      
      bool a = isLight(rowA, wrapIndex(x + offsetAx, textureW));
      bool b = isLight(rowB, wrapIndex(x + offsetBx, textureW));
      if (!(a && b)) continue;

      uint16_t currPixel = canvas.readPixel(x, y);
      if (currPixel == POND_SHADOW_COLOUR) continue; 
      if (currPixel == brightColour) continue; 
      if (currPixel == SOFT_CAUSTIC_COLOUR) continue;
      if (currPixel == POND_COLOUR) continue; 

      canvas.drawPixel(x, y, lerp565(currPixel, brightColour, 0.6));  // lines cross
      // neither: leave the pond as it is
    }
  }
}
void WaterEffects::updatePhase(float dt) {
  phase += 1 * dt;
  if (phase > TWO_PI) phase -= TWO_PI;
}


void WaterEffects::drawLilyPadShadows(LGFX_Sprite &canvas, float t, float dt) {
    int plain_x = 50 + 1 + 3.6f * sinf(t * 0.2f + 10);
    int plain_y = 50 + 1 + 3.8f * sinf(t * 0.4f + 12);
    drawShadow(plainLilyPad, canvas, plain_x, plain_y);

    int pink_x = 100 + 1 + 3.2f * sinf(t * 0.2f + 10);
    int pink_y = 100 + 1 + 3.4f * sinf(t * 0.4f + 12);
    drawShadow(pinkLilyPad, canvas, pink_x, pink_y);

    int white_x = 20 + 1 + 3.3f * sinf(t * 0.2f + 10);
    int white_y = 70 + 1 + 2.4f * sinf(t * 0.4f + 12);
    drawShadow(whiteLilyPad, canvas, white_x, white_y);    
}



void WaterEffects::drawLilyPads(LGFX_Sprite &canvas, float t, float dt) {

    int plain_x = 50 + 3.1f * sinf(t * 0.2f + 10);
    int plain_y = 50 + 3.2f * sinf(t * 0.4f + 12);
    drawTexture(plainLilyPad, canvas, plain_x, plain_y);

    int pink_x = 100 + 2.8f * sinf(t * 0.2f + 10);
    int pink_y = 100 + 2.9f * sinf(t * 0.4f + 12);
    drawTexture(pinkLilyPad, canvas, pink_x, pink_y);

    int white_x = 20 + 2.8f * sinf(t * 0.2f + 10);
    int white_y = 70 + 1.9f * sinf(t * 0.4f + 12);
    drawTexture(whiteLilyPad, canvas, white_x, white_y);    

}

void WaterEffects::startRippleAt(int x, int y, int16_t strength) {
    int16_t (*cur)[WATER_H]  = water[now];
    cur[x][y] += strength;
    cur[x + 1][y] += strength/2;
    cur[x - 1][y] += strength/2;
    cur[x][y + 1] += strength/2; 
    cur[x][y - 1] += strength/2;   
    cur[x + 1][y + 1] += strength/4;
    cur[x - 1][y - 1] += strength/4;
    cur[x + 1][y - 1] += strength/4; 
    cur[x - 1][y + 1] += strength/4;   
}

void WaterEffects::updateRipples() {
    int16_t (*cur)[WATER_H]  = water[now];
    int16_t (*prev)[WATER_H]  = water[1 - now];
    for (int y = 0; y < WATER_H; y++) {
        for (int x = 0; x < WATER_W; x++) {
            int up = y > 0 ? cur[x][y - 1] : 0;
            int down = y < (WATER_H - 1) ? cur[x][y + 1] : 0;
            int left = x > 0 ? cur[x - 1][y] : 0;
            int right = x < (WATER_W - 1) ? cur[x + 1][y] : 0;
            int upleft = y > 0 && x > 0 ? cur[x - 1][y - 1] : 0;
            int downright = x < (WATER_W - 1) && y < (WATER_H - 1) ? cur[x + 1][y + 1] : 0;
            int downleft = y < (WATER_H - 1) && x > 0 ? cur[x - 1][y + 1] : 0;
            int upright = y > 0 && x < (WATER_W - 1) ? cur[x + 1][y - 1] : 0;



            prev[x][y] = (left + right + up + down + upleft + downright + downleft + upright) / 4 - prev[x][y];
            int damp = prev[x][y] / 16;
            if (damp == 0 && prev[x][y] != 0) damp = (prev[x][y] > 0) ? 1 : -1;
            prev[x][y] -= damp;
        }
    }
    now = 1 - now;
}

void WaterEffects::drawSurface(LGFX_Sprite &underwater, LGFX_Sprite &screen) {
    int16_t (*cur)[WATER_H]  = water[now];
    memcpy(screen.getBuffer(), underwater.getBuffer(), CANVAS_W * CANVAS_H * 2);   // 2 bytes per pixel

    uint16_t *src = (uint16_t *)underwater.getBuffer();
    uint16_t *dst = (uint16_t *)screen.getBuffer();

    for (int y = 1; y < WATER_H - 1; y++) {
        for (int x = 1; x < WATER_W - 1; x++) {
            int slopeX = (cur[x + 1][y] - cur[x - 1][y]) >> 3;
            int slopeY = (cur[x][y + 1] - cur[x][y - 1]) >> 3;

            if (slopeX == slopeY && slopeX == 0) continue;

            int facing = -(slopeX + slopeY);
            uint16_t target = (facing > 0) ? BRIGHT_CAUSTIC_COLOUR : POND_SHADOW_COLOUR;
            float    amount = (facing == 0) ? 0.0f : 0.15f;

            int dx = slopeX >> 3;
            int dy = slopeY >> 3;

            for (int py = 0; py < 2; py++) {
                for (int px = 0; px < 2; px++) {
                    int sx = x * 2 + px, sy = y * 2 + py;
                    uint16_t colour = __builtin_bswap16(src[(sy + dy) * CANVAS_W + (sx + dx)]);
                    
                    dst[sy * CANVAS_W + sx] = __builtin_bswap16(lerp565(colour, target, amount));
                }
            }
        }
    }
}