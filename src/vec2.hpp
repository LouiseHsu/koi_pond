#pragma once
#include <Arduino.h>

// =============================================================================
// Vector math
// =============================================================================

struct Vec2 { float x, y; };

static inline Vec2  operator+(Vec2 first, Vec2 second)  { return Vec2{first.x + second.x, first.y + second.y}; }
static inline Vec2  operator-(Vec2 first, Vec2 second)  { return Vec2{first.x - second.x, first.y - second.y}; }
static inline Vec2  operator*(Vec2 vector, float scale) { return Vec2{vector.x * scale, vector.y * scale}; }
static inline Vec2  operator*=(Vec2 &vector, float scale) { return vector = Vec2{vector.x * scale, vector.y * scale}; }
static inline Vec2  operator+=(Vec2 &first, Vec2 second) { return first = first + second ;};

static inline float length(Vec2 vector)                 { return sqrtf(vector.x * vector.x + vector.y * vector.y); }

static inline Vec2 normalize(Vec2 vector) {
  float vectorLength = length(vector);
  return vectorLength > 1e-6f ? vector * (1.0f / vectorLength) : Vec2{0, 1};
}

static inline Vec2 directionFromAngle(float angle) { return Vec2{cosf(angle), sinf(angle)}; }

static inline float wrapAngle(float angle) {
  while (angle >  PI) angle -= TWO_PI;
  while (angle < -PI) angle += TWO_PI;
  return angle;
}
