// A small software canvas (32-bit ARGB) with the drawing primitives souldew's 3DS screens need: filled and rounded
// rectangles with anti-aliased corners, gradients, bitmap text. The launcher, the plugin and the host renderer all
// draw through it, so screenshots rendered on a computer are the very pixels the console shows.
#pragma once
#include <cstdint>
#include <string>
#include "fonts.h"

namespace sd {

struct Color {
  uint8_t r, g, b, a;
  constexpr Color() : r(0), g(0), b(0), a(0) {}
  constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}
  constexpr Color alpha(uint8_t na) const { return Color(r, g, b, na); }
};

constexpr Color rgb(uint32_t hex, uint8_t a = 255) {
  return Color((hex >> 16) & 0xff, (hex >> 8) & 0xff, hex & 0xff, a);
}
Color mix(Color a, Color b, float t);  // a → b

struct Rect {
  int x, y, w, h;
  bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
};

enum Align { ALIGN_LEFT = 0, ALIGN_CENTER = 1, ALIGN_RIGHT = 2 };

class Canvas {
 public:
  Canvas(int w, int h);
  ~Canvas();
  Canvas(const Canvas &) = delete;
  Canvas &operator=(const Canvas &) = delete;

  int width() const { return w_; }
  int height() const { return h_; }
  const uint32_t *pixels() const { return px_; }  // 0xAARRGGBB, row-major
  uint32_t *pixels() { return px_; }

  void setClip(Rect r);
  void resetClip();
  Rect clip() const { return clip_; }

  void clear(Color c);  // replaces (also alpha) – the overlay canvas starts transparent
  void fill(int x, int y, int w, int h, Color c);  // blends
  void blendPixel(int x, int y, Color c, int cover = 255);
  void hline(int x, int y, int w, Color c) { fill(x, y, w, 1, c); }
  void vline(int x, int y, int h, Color c) { fill(x, y, 1, h, c); }
  // rounded rectangle, corners anti-aliased; r ≤ min(w, h) / 2
  void roundRect(int x, int y, int w, int h, int r, Color c);
  // a border of thickness t inside the rounded rect
  void roundRectStroke(int x, int y, int w, int h, int r, int t, Color c);
  // horizontal gradient clipped to a rounded rect
  void roundRectGradient(int x, int y, int w, int h, int r, Color left, Color right);
  void circle(int cx, int cy, int r, Color c);
  // the card used everywhere: fill + 1px edge (+ optional accent bar on the left)
  void card(int x, int y, int w, int h, int r, Color fill, Color edge, Color accent = Color(), int accentW = 0);

  // text: UTF-8; y is the top of the line box (ascent below it). Returns the advance width.
  int text(const Font &f, int x, int y, const std::string &s, Color c, int maxW = 0, Align align = ALIGN_LEFT);
  static int textWidth(const Font &f, const std::string &s);
  static int lineHeight(const Font &f) { return f.ascent + f.descent; }
  // letter-spaced (Silkscreen labels in the mockups are tracked out a little)
  int textSpaced(const Font &f, int x, int y, const std::string &s, Color c, int spacing);
  static int textSpacedWidth(const Font &f, const std::string &s, int spacing);
  // wraps at spaces; returns the height used
  int textWrapped(const Font &f, int x, int y, int w, const std::string &s, Color c, int lineGap = 1, int maxLines = 0);

  // copies a region of another canvas (alpha blended)
  void draw(const Canvas &src, int dx, int dy);

 private:
  int w_, h_;
  uint32_t *px_;
  Rect clip_;
  int coverage(int x, int y, int rx, int ry, int rw, int rh, int r) const;
};

// UTF-8 → code points (invalid bytes become U+FFFD)
uint32_t utf8Next(const std::string &s, size_t &i);
const Glyph *findGlyph(const Font &f, uint32_t cp);

}  // namespace sd
