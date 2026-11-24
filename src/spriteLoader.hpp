#pragma once
#include <Arduino.h>

// Loads a TGA image (24- or 32-bit, plain or run-length compressed) from
// memory into a small texture: an RGB565 colour and an opaque flag per texel.
class SpriteLoader {
public:
  static const int MAX_TEXTURE_WIDTH  = 32;
  static const int MAX_TEXTURE_HEIGHT = 64;

  // Texels with alpha (opacity, 0..255) below this count as see-through.
  static const uint8_t ALPHA_THRESHOLD = 128;

  // Returns false and sets errorMessage if the file can't be loaded.
  bool loadTga(const uint8_t *file, unsigned int fileSize, const char *&errorMessage);

  // --- Reading the loaded texture ---
  int      width()  const { return textureWidth; }
  int      height() const { return textureHeight; }
  uint16_t colourAt(int row, int column)  const { return textureColors[row][column]; }
  bool     isOpaque(int row, int column) const { return texelIsOpaque[row][column]; }

private:
  // --- Helpers ---
  static uint16_t toRGB565(uint8_t red, uint8_t green, uint8_t blue);
  static int      readUint16(const uint8_t *bytes);

  // --- Texture data ---
  int      textureWidth  = 0;
  int      textureHeight = 0;
  uint16_t textureColors[MAX_TEXTURE_HEIGHT][MAX_TEXTURE_WIDTH];  // RGB565 colour of each texel
  bool     texelIsOpaque[MAX_TEXTURE_HEIGHT][MAX_TEXTURE_WIDTH];  // false = see-through, skip it
};
