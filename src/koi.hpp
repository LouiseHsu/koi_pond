#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "vec2.hpp"
#include "spriteLoader.hpp"

class Koi {
public:
  // --- Fish Spawn ---
  static constexpr int MAX_DEPTH   = 10;
  int depth = 0; 

  // --- Fish body ---
  static constexpr int   NUM_SEG       = 8;
  static constexpr int   NUM_JOINT     = NUM_SEG + 1;
  static constexpr float segmentLength = 5.0f;
  static constexpr float CHUB_FACTOR   = 1.0f;

  // --- Fish movement ---
  static constexpr float SWIM_SPEED            = 25.0f;  // world units per second
  static constexpr float MAX_TURN_SPEED        = 1.2f;
  static constexpr float TURN_MULTI            = 1.0f;
  static constexpr float EDGE_TURN_MULTI       = 2.5f;
  static constexpr float MAX_BEND_ANGLE        = 0.3f;
  static constexpr float HEAD_WIGGLE_AMPLITUDE = 0.7f;   // how far the head swings side to side (radians)
  static constexpr float HEAD_WIGGLE_FREQUENCY = 4.0f;   // how fast the head swings (radians per second)

  // --- Body wave (for drawing only) ---
  static constexpr float BODY_WAVE_SPEED = 5.0f;  // how fast the wave travels down the body (radians per second)
  static constexpr float BODY_WAVE_LAG   = 0.7f;  // how far each segment trails the one in front (radians per segment)

  // Call once in setup(), after the texture is loaded.
  void begin(SpriteLoader &texture, float pondWidth, float pondHeight);

  // Call once per frame: dt = seconds since last frame, t = seconds since boot.
  void update(float dt, float t);
  void drawFish(LGFX_Sprite &canvas);
  void drawShadow(LGFX_Sprite &canvas);

private:
  struct RowValues {
    Vec2  spinePoint;           // where this row crosses the spine
    Vec2  perpendicularVector;  // which way is "sideways" here
    float offset;               // body-wave swing for this row
  };

// Screen position of every texel corner. One more than the texture size in
// each direction, because 16 texels across have 17 edges between them.
int16_t cornerScreenX[SpriteLoader::MAX_TEXTURE_HEIGHT + 1][SpriteLoader::MAX_TEXTURE_WIDTH + 1];
int16_t cornerScreenY[SpriteLoader::MAX_TEXTURE_HEIGHT + 1][SpriteLoader::MAX_TEXTURE_WIDTH + 1];

  // --- Spine ---
  void initSpine();
  void updateSpineAngles();

  // --- Movement ---
  void updatePhase(float dt);
  void moveHead(float dt, float t);
  void moveBody();

  // --- Drawing ---
  Vec2 spriteToFish(float textureX, float textureY);
  RowValues spriteToFishRow(float textureY);
  Vec2 spriteToFishCol(float textureX, Koi::RowValues val);

  // --- Pond ---
  float W = 0, H = 0;  // pond size

  // --- Texture ---
  const SpriteLoader *texture = nullptr;
  float texelLength = 0.0f;
  float texelWidth  = 0.0f;
  uint16_t mainColour = 0;  // this fish's orange, picked in begin()
  uint16_t bodyColour = 0;  // this fish's orange, picked in begin()


  // --- Spine ---
  Vec2 joints[NUM_JOINT];
  Vec2 jointAngles[NUM_JOINT];  // direction of each joint from head to tail

  // --- Movement ---
  Vec2  target                 = {0, 0};
  float headingAngle           = 0.0f;
  float headingAngleWithWiggle = 0.0f;
  float phase                  = 0.0f;
};
