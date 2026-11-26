#pragma once
#include <LovyanGFX.hpp>
#include "spriteLoader.hpp"
#include "pondConfig.hpp"
#include "vec2.hpp"
#include "lilypads.h"

struct LilyPad {
  SpriteLoader *texture = nullptr;
  Vec2 position;
  Vec2 homePosition;
  Vec2 velocity = {0, 0};
  float wanderSpeedX;
  float wanderSpeedY;
};

class WaterEffects {
public:
  // --- Caustic drift (pixels per second) ---
  // Two copies ("sheets") of the caustic texture drift different ways. Where
  // their lines cross, the light is brighter, so the pattern shimmers.
  static constexpr float SHEET_A_SPEED_X =  3.0f;
  static constexpr float SHEET_A_SPEED_Y =  1.0f;
  static constexpr float SHEET_B_SPEED_X = -1.0f;
  static constexpr float SHEET_B_SPEED_Y =  2.0f;

  // How far each caustic colour is from pond (0) to white (1).
  static constexpr float SOFT_CAUSTIC_AMOUNT   = 0.2f;  // one sheet has a line here
  static constexpr float BRIGHT_CAUSTIC_AMOUNT = 0.7f;  // both sheets have a line here

  // --- Ripple grid ---
  static constexpr int WATER_W = CANVAS_W / 2;
  static constexpr int WATER_H = CANVAS_H / 2;

  // Call once in setup(). Loads the caustic texture and builds the tile sprite.
  void begin();

  // --- Ripples ---
  void startRippleAt(int canvas_x, int canvas_y, int16_t strength);
  void updateRipples();

  // --- Caustics ---
  // Call once per frame. t = seconds since boot.
  void updateCausticsBottom(LGFX_Sprite &canvas, float t);
  void updateCausticsTop(LGFX_Sprite &canvas, float t);

  // --- Lily pads ---
  void updateLilyPads(float dt);
  void drawLilyPadShadows(LGFX_Sprite &canvas, float t, float dt);
  void drawLilyPads(LGFX_Sprite &canvas, float t, float dt);

  // --- Surface ---
  void drawSurface(LGFX_Sprite &underwater, LGFX_Sprite &screen);

private:
  // --- Setup helpers ---
  void beginLilyPads();
  void copyTextureToSprite(const SpriteLoader &texture, LGFX_Sprite &target);

  // --- Caustic helpers ---
  bool isLight(int row, int column);

  // --- Lily pad helpers ---
  void updatePhase(float dt);
  void drawLilyPadTexture(const SpriteLoader &texture, LGFX_Sprite &canvas, int16_t offset_x, int16_t offset_y);
  void drawShadow(const SpriteLoader &texture, LGFX_Sprite &canvas, int16_t offset_x, int16_t offset_y);

  // --- Ripples ---
  int16_t water[2][WATER_W][WATER_H];
  int now = 1;

  // --- Caustics ---
  SpriteLoader causticTexture;
  LGFX_Sprite  tile;

  // --- Lily pads ---
  LilyPad      lilypads[NUM_LILYPADS];
  SpriteLoader lilypadsTextures[NUM_LILYPADS];
  float phase;
};
