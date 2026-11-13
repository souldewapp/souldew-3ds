// Bitmap fonts (4-bit alpha), generated from the desktop app's fonts by tools/gen_fonts.py.
#pragma once
#include <cstdint>

namespace sd {

struct Glyph {
  uint16_t cp;     // Unicode code point
  int8_t x, y;     // bitmap offset from the pen position on the baseline (y < 0: above)
  uint8_t w, h;    // bitmap size
  uint8_t adv;     // advance
  uint32_t off;    // offset into the font's bits (rows of ceil(w/2) bytes, high nibble first)
};

struct Font {
  int ascent, descent;
  int count;
  const Glyph *glyphs;  // sorted by code point
  const unsigned char *bits;
};

extern const Font font_title, font_display, font_head, font_label, font_num, font_body, font_body_b, font_small;

}  // namespace sd
