#include "fbblit.h"

#include <algorithm>

namespace sd {

int bytesPerPixel(PixFmt f) {
  switch (f) {
    case PixFmt::RGBA8: return 4;
    case PixFmt::BGR8: return 3;
    default: return 2;
  }
}

FbTarget ctruTarget(uint8_t *fb, int screenWidth, PixFmt fmt) {
  int bpp = bytesPerPixel(fmt);
  // pixel (x, y) at ((x * 240) + (239 - y)) * bpp
  return FbTarget{fb + 239 * bpp, 240 * bpp, -bpp, screenWidth, 240, fmt};
}

static inline Color load(const uint8_t *p, PixFmt f) {
  switch (f) {
    case PixFmt::RGBA8: return Color(p[3], p[2], p[1], 255);  // bytes A B G R
    case PixFmt::BGR8: return Color(p[2], p[1], p[0], 255);   // bytes B G R
    case PixFmt::RGB565: {
      uint16_t v = p[0] | (p[1] << 8);
      int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
      return Color((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2), 255);
    }
    case PixFmt::RGB5A1: {
      uint16_t v = p[0] | (p[1] << 8);
      int r = (v >> 11) & 31, g = (v >> 6) & 31, b = (v >> 1) & 31;
      return Color((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), 255);
    }
    case PixFmt::RGBA4: {
      uint16_t v = p[0] | (p[1] << 8);
      return Color(((v >> 12) & 15) * 17, ((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, 255);
    }
  }
  return Color();
}

static inline void store(uint8_t *p, PixFmt f, Color c) {
  switch (f) {
    case PixFmt::RGBA8: p[0] = 255; p[1] = c.b; p[2] = c.g; p[3] = c.r; break;
    case PixFmt::BGR8: p[0] = c.b; p[1] = c.g; p[2] = c.r; break;
    case PixFmt::RGB565: {
      uint16_t v = ((c.r >> 3) << 11) | ((c.g >> 2) << 5) | (c.b >> 3);
      p[0] = v & 0xff; p[1] = v >> 8; break;
    }
    case PixFmt::RGB5A1: {
      uint16_t v = ((c.r >> 3) << 11) | ((c.g >> 3) << 6) | ((c.b >> 3) << 1) | 1;
      p[0] = v & 0xff; p[1] = v >> 8; break;
    }
    case PixFmt::RGBA4: {
      uint16_t v = ((c.r >> 4) << 12) | ((c.g >> 4) << 8) | ((c.b >> 4) << 4) | 0xf;
      p[0] = v & 0xff; p[1] = v >> 8; break;
    }
  }
}

void blit(const Canvas &cv, Rect src, const FbTarget &t, int dx, int dy, bool blendOn) {
  const uint32_t *px = cv.pixels();
  for (int y = 0; y < src.h; y++) {
    int ty = dy + y, sy = src.y + y;
    if (ty < 0 || ty >= t.height || sy < 0 || sy >= cv.height()) continue;
    for (int x = 0; x < src.w; x++) {
      int tx = dx + x, sx = src.x + x;
      if (tx < 0 || tx >= t.width || sx < 0 || sx >= cv.width()) continue;
      uint32_t s = px[sy * cv.width() + sx];
      int a = s >> 24;
      if (blendOn && a == 0) continue;
      uint8_t *p = t.origin + tx * t.stepX + ty * t.stepY;
      Color c((s >> 16) & 0xff, (s >> 8) & 0xff, s & 0xff, 255);
      if (blendOn && a < 255) c = mix(load(p, t.fmt), c, a / 255.f);
      store(p, t.fmt, c);
    }
  }
}

Color readPixel(const FbTarget &t, int x, int y) { return load(t.origin + x * t.stepX + y * t.stepY, t.fmt); }

void dim(const FbTarget &t, Rect r, int keep256) {
  for (int y = std::max(0, r.y); y < std::min(t.height, r.y + r.h); y++)
    for (int x = std::max(0, r.x); x < std::min(t.width, r.x + r.w); x++) {
      uint8_t *p = t.origin + x * t.stepX + y * t.stepY;
      Color c = load(p, t.fmt);
      store(p, t.fmt, Color(c.r * keep256 >> 8, c.g * keep256 >> 8, c.b * keep256 >> 8));
    }
}

}  // namespace sd
