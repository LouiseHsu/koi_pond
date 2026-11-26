#include "koi.hpp"
#include "colours.hpp"



// The orange painted in the TGA files. Texels this colour get swapped for
// each koi's own mainColour when drawn.
static const uint16_t PLACEHOLDER_ORANGE = lgfx::color565(251, 107, 29);


// Oranges a fish can be. Each fish gets a special one in begin().
static const uint16_t mainColourChoices[] = {0xfb44, 0xe1e7};
static const int      NUM_MAIN_COLOURS    = sizeof(mainColourChoices) / sizeof(mainColourChoices[0]);

// =============================================================================
// Setup / per-frame
// =============================================================================

void Koi::begin(SpriteLoader &texture, float pondWidth, float pondHeight) {
  this->texture = &texture;
  depth = random(0, MAX_DEPTH);
  mainColour = mainColourChoices[random(0, NUM_MAIN_COLOURS)];
  mainColour = lerp565(mainColour, POND_COLOUR, depth * .05);
  bodyColour = lerp565(TFT_WHITE, POND_COLOUR, depth * .05);
  W = pondWidth;
  H = pondHeight;
  

  texelLength = segmentLength * NUM_SEG / texture.height();
  texelWidth  = texelLength * CHUB_FACTOR;
  texelLength *= 1 - (0.02 * depth);
  texelWidth *= 1 - (0.02 * depth);

  target = Vec2{(float)random(0, W), (float)random(0, H)};
  initSpine();
}

void Koi::update(float dt, float t) {
  updatePhase(dt);
  moveHead(dt, t);
  moveBody();

  for (int row = 0; row <= texture->height(); row++) {
    RowValues values = spriteToFishRow(row);
    for (int column = 0; column <= texture->width(); column++) {
      Vec2 screenPoint = spriteToFishCol((float)column, values);
      cornerScreenX[row][column] = (int16_t)lroundf(screenPoint.x);
      cornerScreenY[row][column] = (int16_t)lroundf(screenPoint.y);
    }
  }
}

void Koi::updatePhase(float dt) {
  phase += BODY_WAVE_SPEED * dt;
  if (phase > TWO_PI) phase -= TWO_PI;
}

// =============================================================================
// Spine
// =============================================================================

void Koi::initSpine() {
  Vec2 placement = {(float)random(0, W), (float)random(0, H)};
  for (int i = 0; i < NUM_JOINT; i++) {
    joints[i] = placement;
    placement.x -= segmentLength * 0.7071f;
    placement.y -= segmentLength * 0.7071f;
  }
}

void Koi::updateSpineAngles() {
  for (int i = 0; i < NUM_JOINT; i++) {
    Vec2 directionFromPrevious = (i > 0)
        ? normalize(joints[i] - joints[i - 1]) : Vec2{0, 0};
    Vec2 directionToNext = (i < NUM_SEG)
        ? normalize(joints[i + 1] - joints[i]) : Vec2{0, 0};
    jointAngles[i] = normalize(directionFromPrevious + directionToNext);
  }
}

// =============================================================================
// Movement
// =============================================================================

void Koi::moveHead(float dt, float t) {
  Vec2 &head = joints[0];

  if (length(target - head) < 20.0f) {
    target = Vec2{(float)random(0, W), (float)random(0, H)};
  }

  Vec2 headToTarget = target - head;

  // --- Pick where we want to go ---
  float wanderOffset   = 0.5f * sinf(t * 0.55f)          // slower, bigger curves
                       + 0.3f * sinf(t * 1.37f + 1.0f);  // faster, smaller wiggle
  float desiredHeading = atan2f(headToTarget.y, headToTarget.x) + wanderOffset;

  // --- Turn toward it. turn speed is constrained. ---
  float turnNeeded       = wrapAngle(desiredHeading - headingAngle);
  float maxTurnThisFrame = MAX_TURN_SPEED * dt;
  turnNeeded   = constrain(turnNeeded, -maxTurnThisFrame, maxTurnThisFrame);
  headingAngle = wrapAngle(headingAngle + turnNeeded);

  // --- Move the head forward, swinging side to side like it's swimming ---
  headingAngleWithWiggle = headingAngle + HEAD_WIGGLE_AMPLITUDE * sinf(t * HEAD_WIGGLE_FREQUENCY);
  head = head + directionFromAngle(headingAngleWithWiggle) * (SWIM_SPEED * dt);
}

void Koi::moveBody() {
  float lastAngle = headingAngleWithWiggle + PI;
  for (int i = 1; i < NUM_JOINT; i++) {
    float angle     = atan2f(joints[i].y - joints[i - 1].y, joints[i].x - joints[i - 1].x);
    float bendAngle = wrapAngle(angle - lastAngle);
    bendAngle = constrain(bendAngle, -MAX_BEND_ANGLE, MAX_BEND_ANGLE);
    angle     = lastAngle + bendAngle;

    joints[i] = joints[i - 1] + directionFromAngle(angle) * segmentLength;
    lastAngle = angle;
  }

  updateSpineAngles();
}

// =============================================================================
// Drawing
// =============================================================================

Vec2 Koi::spriteToFish(float textureX, float textureY) {
  // --- Find the point on the spine for this texture row ---
  float distanceDownSpine = textureY * texelLength;
  int   segmentIndex      = (int)(distanceDownSpine / segmentLength);
  segmentIndex = constrain(segmentIndex, 0, NUM_SEG - 1);
  float segmentFraction = (distanceDownSpine - (segmentIndex * segmentLength)) / segmentLength;
  float spinePosition   = segmentIndex + segmentFraction;

  Vec2 segmentStart = joints[segmentIndex];
  Vec2 segmentEnd   = joints[segmentIndex + 1];
  Vec2 spinePoint   = segmentStart + (segmentEnd - segmentStart) * segmentFraction;

  // --- Find which way the spine points there ---
  Vec2 startDirection = jointAngles[segmentIndex];
  Vec2 endDirection   = jointAngles[segmentIndex + 1];
  Vec2 spineDirection = normalize(startDirection + (endDirection - startDirection) * segmentFraction);

  Vec2 perpendicularVector = Vec2{spineDirection.y, -spineDirection.x};

  // --- Body wave: sideways swing, bigger toward the tail ---
  float A_i    = 0.03f + (spinePosition / 8.0f) * 0.27f;
  float offset = A_i * 10.0f * sinf(phase - spinePosition * BODY_WAVE_LAG);

  // --- Step sideways for the texture column, from middle of the width ---
  float sidewaysDistance = (textureX - texture->width() * 0.5f) * texelWidth;
  Vec2  worldPoint       = spinePoint + perpendicularVector * (sidewaysDistance + offset);

  return worldPoint;
}

Koi::RowValues Koi::spriteToFishRow(float y) {
    // --- Find the point on the spine for this texture row ---
  float distanceDownSpine = y * texelLength;
  int   segmentIndex      = (int)(distanceDownSpine / segmentLength);
  segmentIndex = constrain(segmentIndex, 0, NUM_SEG - 1);
  float segmentFraction = (distanceDownSpine - (segmentIndex * segmentLength)) / segmentLength;
  float spinePosition   = segmentIndex + segmentFraction;

  Vec2 segmentStart = joints[segmentIndex];
  Vec2 segmentEnd   = joints[segmentIndex + 1];
  Vec2 spinePoint   = segmentStart + (segmentEnd - segmentStart) * segmentFraction;

  // --- Find which way the spine points there ---
  Vec2 startDirection = jointAngles[segmentIndex];
  Vec2 endDirection   = jointAngles[segmentIndex + 1];
  Vec2 spineDirection = normalize(startDirection + (endDirection - startDirection) * segmentFraction);

  Vec2 perpendicularVector = Vec2{spineDirection.y, -spineDirection.x};

  // --- fish body wave + bigger toward the tail ---
  float A_i    = 0.03f + (spinePosition / 8.0f) * 0.27f;
  float offset = A_i * 10.0f * sinf(phase - spinePosition * BODY_WAVE_LAG);

  return RowValues(spinePoint, perpendicularVector, offset);
}

  Vec2 Koi::spriteToFishCol(float x, RowValues val) {
    float sidewaysDistance = (x - texture->width() * 0.5f) * texelWidth;
    Vec2  worldPoint       = val.spinePoint + val.perpendicularVector * (sidewaysDistance + val.offset);
    return worldPoint;
  }

void Koi::drawShadow(LGFX_Sprite &sprite) {
  int offset = floor((MAX_DEPTH - depth) * 1.5);

  for (int row = 0; row < texture->height(); row++) {
    for (int column = 0; column < texture->width(); column++) {
      if (!texture->isOpaque(row, column)) continue;  // see-through

      int16_t topLeftX     = cornerScreenX[row][column],         topLeftY     = cornerScreenY[row][column];
      int16_t topRightX    = cornerScreenX[row][column + 1],     topRightY    = cornerScreenY[row][column + 1];
      int16_t bottomLeftX  = cornerScreenX[row + 1][column],     bottomLeftY  = cornerScreenY[row + 1][column];
      int16_t bottomRightX = cornerScreenX[row + 1][column + 1], bottomRightY = cornerScreenY[row + 1][column + 1];

      sprite.fillTriangle(topLeftX + offset, topLeftY, topRightX + offset,    topRightY,    bottomRightX + offset, bottomRightY, POND_SHADOW_COLOUR);
      sprite.fillTriangle(topLeftX + offset, topLeftY, bottomRightX + offset, bottomRightY, bottomLeftX + offset,  bottomLeftY,  POND_SHADOW_COLOUR);
    }
  }
}

void Koi::drawFish(LGFX_Sprite &sprite) {
  for (int row = 0; row < texture->height(); row++) {
    for (int column = 0; column < texture->width(); column++) {
      if (!texture->isOpaque(row, column)) continue;  // see-through
      uint16_t colour = texture->colourAt(row, column);
      if (colour == PLACEHOLDER_ORANGE) colour = mainColour;  // this fish's own orange
      if (colour == TFT_WHITE) colour = bodyColour;

      int16_t topLeftX     = cornerScreenX[row][column],         topLeftY     = cornerScreenY[row][column];
      int16_t topRightX    = cornerScreenX[row][column + 1],     topRightY    = cornerScreenY[row][column + 1];
      int16_t bottomLeftX  = cornerScreenX[row + 1][column],     bottomLeftY  = cornerScreenY[row + 1][column];
      int16_t bottomRightX = cornerScreenX[row + 1][column + 1], bottomRightY = cornerScreenY[row + 1][column + 1];

      sprite.fillTriangle(topLeftX, topLeftY, topRightX,    topRightY,    bottomRightX, bottomRightY, colour);
      sprite.fillTriangle(topLeftX, topLeftY, bottomRightX, bottomRightY, bottomLeftX,  bottomLeftY,  colour);
    }
  }
}
