#include "spriteLoader.hpp"

// =============================================================================
// Helpers
// =============================================================================

// Packs 8-bit red, green, blue into the screen's 16-bit format (5 red, 6 green, 5 blue bits).
uint16_t SpriteLoader::toRGB565(uint8_t red, uint8_t green, uint8_t blue) {
  return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

// Reads a 2-byte little-endian number.
int SpriteLoader::readUint16(const uint8_t *bytes) {
  return bytes[0] | (bytes[1] << 8);
}

// =============================================================================
// TGA reader
// =============================================================================
//
// A TGA file is:
//
//   18-byte header
//   [optional ID text]            length is header byte 0, we skip it
//   pixels                        width * height of them
//   [optional footer]             we ignore it
//
// Header bytes we care about (2-byte numbers are "little endian": low byte first):
//   byte  0      length of the ID text that follows the header
//   byte  1      colour map type: must be 0 (no palette)
//   byte  2      image type: 2 = plain pixels, 10 = run-length compressed pixels
//   bytes 12-13  width
//   bytes 14-15  height
//   byte  16     bits per pixel: 32 (blue, green, red, alpha) or 24 (blue, green, red)
//   byte  17     flags. Bit 5 set = rows stored top-to-bottom,
//                otherwise bottom-to-top (the TGA default).
//
// Note the colour order in the file is blue, green, red, not red, green, blue.

bool SpriteLoader::loadTga(const uint8_t *file, unsigned int fileSize, const char *&errorMessage) {
  // --- Read the header ---
  if (fileSize < 18) { errorMessage = "file too small"; return false; }

  int  idLength        = file[0];
  int  colorMapType    = file[1];
  int  imageType       = file[2];
  int  width           = readUint16(&file[12]);
  int  height          = readUint16(&file[14]);
  int  bitsPerPixel    = file[16];
  bool rowsTopToBottom = (file[17] & 0x20) != 0;  // 0x20 = bit 5

  // --- Check we can handle it ---
  if (colorMapType != 0)                        { errorMessage = "palette TGAs not supported"; return false; }
  if (imageType != 2 && imageType != 10)        { errorMessage = "not a colour TGA";           return false; }
  if (bitsPerPixel != 32 && bitsPerPixel != 24) { errorMessage = "must be 24 or 32 bit";       return false; }
  if (width > MAX_TEXTURE_WIDTH || height > MAX_TEXTURE_HEIGHT) {
    errorMessage = "image too big (raise MAX_TEXTURE_*)";
    return false;
  }

  textureWidth  = width;
  textureHeight = height;
  int  bytesPerPixel = bitsPerPixel / 8;
  bool isCompressed  = (imageType == 10);

  unsigned int readPosition = 18 + idLength;  // first pixel byte

  // --- Read the pixels ---
  // Pixels come one after another: the whole first row, then the next row...
  // For compressed files they come in "packets":
  //   a packet starts with one byte. Its top bit says what kind it is,
  //   and the low 7 bits + 1 say how many pixels it covers.
  //     top bit 1 = "run":  ONE pixel follows, repeat it that many times.
  //     top bit 0 = "raw":  that many pixels follow, use each once.
  int  pixelsLeftInPacket = 0;
  bool packetIsRun        = false;

  int totalPixels = width * height;
  for (int pixelIndex = 0; pixelIndex < totalPixels; pixelIndex++) {

    // Where in the file is this pixel's data?
    if (isCompressed) {
      if (pixelsLeftInPacket == 0) {                    // start a new packet
        if (readPosition >= fileSize) { errorMessage = "file ended early"; return false; }
        uint8_t packetHeader = file[readPosition++];
        packetIsRun        = (packetHeader & 0x80) != 0;
        pixelsLeftInPacket = (packetHeader & 0x7F) + 1;
        if (packetIsRun) {                              // read the one repeated pixel now
          readPosition += bytesPerPixel;
        }
      }
      pixelsLeftInPacket--;
    }
    unsigned int pixelStart;
    if (isCompressed && packetIsRun) {
      pixelStart = readPosition - bytesPerPixel;        // same pixel every time
    } else {
      pixelStart = readPosition;
      readPosition += bytesPerPixel;
    }
    if (pixelStart + bytesPerPixel > fileSize) { errorMessage = "file ended early"; return false; }

    // Pull out the colour (stored blue, green, red[, alpha])
    const uint8_t *pixel = &file[pixelStart];
    uint8_t blue  = pixel[0];
    uint8_t green = pixel[1];
    uint8_t red   = pixel[2];
    uint8_t alpha = (bytesPerPixel == 4) ? pixel[3] : 255;  // 24-bit = fully opaque

    // Which row and column is this? Flip if the file is stored bottom-to-top,
    // so that row 0 is always the top of the picture (the fish's head).
    int column  = pixelIndex % width;
    int fileRow = pixelIndex / width;
    int row     = rowsTopToBottom ? fileRow : (height - 1 - fileRow);

    textureColors[row][column] = toRGB565(red, green, blue);
    texelIsOpaque[row][column] = (alpha >= ALPHA_THRESHOLD);
  }
  return true;
}
