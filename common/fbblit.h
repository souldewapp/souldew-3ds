// Copies a Canvas region onto a 3DS framebuffer (any GSP pixel format, any memory layout: the 3DS screens are stored
// rotated – a column of 240 pixels after the other, bottom to top – so the caller passes the byte step for +1 x and
// +1 y). Alpha-blends onto what is there (the game's picture) or replaces it.
#pragma once
#include <cstdint>
#include "gfx.h"

namespace sd {

enum class PixFmt { RGBA8 = 0, BGR8 = 1, RGB565 = 2, RGB5A1 = 3, RGBA4 = 4 };

struct FbTarget {
  uint8_t *origin;  // address of pixel (0, 0)
  int stepX, stepY; // byte offsets for +1 x / +1 y (may be negative)
  int width, height;
  PixFmt fmt;
};

int bytesPerPixel(PixFmt f);

// libctru framebuffer layout (gfxGetFramebuffer): 240-pixel columns, y = 0 at the end of each column
FbTarget ctruTarget(uint8_t *fb, int screenWidth, PixFmt fmt);

// copy canvas rect `src` (in canvas coordinates) to (dx, dy) on the target
void blit(const Canvas &cv, Rect src, const FbTarget &t, int dx, int dy, bool blend);

// reads a pixel back from a target (tests and the host renderer)
Color readPixel(const FbTarget &t, int x, int y);

// darkens a target region (the paused game behind souldew's screens)
void dim(const FbTarget &t, Rect r, int keep256);

}  // namespace sd
