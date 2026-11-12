#include "gfx.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace sd {

Color mix(Color a, Color b, float t) {
  if (t <= 0) return a;
  if (t >= 1) return b;
  auto m = [t](uint8_t x, uint8_t y) { return (uint8_t)(x + (y - x) * t + 0.5f); };
  return Color(m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a));
}

Canvas::Canvas(int w, int h) : w_(w), h_(h), px_(new uint32_t[(size_t)w * h]), clip_{0, 0, w, h} {
  std::memset(px_, 0, sizeof(uint32_t) * (size_t)w * h);
}
Canvas::~Canvas() { delete[] px_; }

void Canvas::setClip(Rect r) {
  int x0 = std::max(0, r.x), y0 = std::max(0, r.y);
  int x1 = std::min(w_, r.x + r.w), y1 = std::min(h_, r.y + r.h);
  clip_ = {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}
void Canvas::resetClip() { clip_ = {0, 0, w_, h_}; }

void Canvas::clear(Color c) {
  uint32_t v = ((uint32_t)c.a << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
  for (int y = clip_.y; y < clip_.y + clip_.h; y++)
    for (int x = clip_.x; x < clip_.x + clip_.w; x++) px_[y * w_ + x] = v;
}

// "over" compositing with straight alpha (the destination may be transparent: the overlay canvas)
static inline uint32_t over(uint32_t d, Color c, int cover) {
  int sa = c.a * cover / 255;
  if (sa <= 0) return d;
  int da = d >> 24;
  if (sa >= 255 || da == 0) {
    if (sa >= 255) return 0xff000000u | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
    return ((uint32_t)sa << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
  }
  int dr = (d >> 16) & 0xff, dg = (d >> 8) & 0xff, db = d & 0xff;
  int oa = sa + da * (255 - sa) / 255;
  if (oa <= 0) return 0;
  int r = (c.r * sa + dr * da * (255 - sa) / 255) / oa;
  int g = (c.g * sa + dg * da * (255 - sa) / 255) / oa;
  int b = (c.b * sa + db * da * (255 - sa) / 255) / oa;
  return ((uint32_t)oa << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void Canvas::blendPixel(int x, int y, Color c, int cover) {
  if (!clip_.contains(x, y)) return;
  uint32_t &d = px_[y * w_ + x];
  d = over(d, c, cover);
}

void Canvas::fill(int x, int y, int w, int h, Color c) {
  int x0 = std::max(x, clip_.x), y0 = std::max(y, clip_.y);
  int x1 = std::min(x + w, clip_.x + clip_.w), y1 = std::min(y + h, clip_.y + clip_.h);
  if (x0 >= x1 || y0 >= y1 || c.a == 0) return;
  for (int yy = y0; yy < y1; yy++) {
    uint32_t *row = px_ + yy * w_;
    for (int xx = x0; xx < x1; xx++) row[xx] = over(row[xx], c, 255);
  }
}

// coverage (0..255) of pixel (x, y) by a rounded rect – 4×4 samples in the corner squares, 255/0 elsewhere
int Canvas::coverage(int x, int y, int rx, int ry, int rw, int rh, int r) const {
  if (x < rx || y < ry || x >= rx + rw || y >= ry + rh) return 0;
  int cx, cy;
  if (x < rx + r) cx = rx + r;
  else if (x >= rx + rw - r) cx = rx + rw - r;
  else return 255;
  if (y < ry + r) cy = ry + r;
  else if (y >= ry + rh - r) cy = ry + rh - r;
  else return 255;
  int in = 0;
  float rr = (float)r * r;
  for (int sy = 0; sy < 4; sy++)
    for (int sx = 0; sx < 4; sx++) {
      float fx = x + (sx + 0.5f) / 4.f - cx, fy = y + (sy + 0.5f) / 4.f - cy;
      if (fx * fx + fy * fy <= rr) in++;
    }
  return in * 255 / 16;
}

void Canvas::roundRect(int x, int y, int w, int h, int r, Color c) {
  if (w <= 0 || h <= 0) return;
  r = std::max(0, std::min(r, std::min(w, h) / 2));
  if (r == 0) return fill(x, y, w, h, c);
  fill(x, y + r, w, h - 2 * r, c);
  fill(x + r, y, w - 2 * r, r, c);
  fill(x + r, y + h - r, w - 2 * r, r, c);
  for (int yy = 0; yy < r; yy++)
    for (int xx = 0; xx < r; xx++) {
      int pts[4][2] = {{x + xx, y + yy}, {x + w - 1 - xx, y + yy}, {x + xx, y + h - 1 - yy}, {x + w - 1 - xx, y + h - 1 - yy}};
      for (auto &p : pts) {
        int cov = coverage(p[0], p[1], x, y, w, h, r);
        if (cov) blendPixel(p[0], p[1], c, cov);
      }
    }
}

void Canvas::roundRectStroke(int x, int y, int w, int h, int r, int t, Color c) {
  if (w <= 0 || h <= 0 || t <= 0) return;
  r = std::max(0, std::min(r, std::min(w, h) / 2));
  int ri = std::max(0, r - t);
  int m = std::max(r, t);
  for (int yy = y; yy < y + h; yy++) {
    bool band = yy < y + m || yy >= y + h - m;
    for (int xx = x; xx < x + w; xx++) {
      if (!band && xx == x + m && w > 2 * m) xx = x + w - m;  // skip the inside
      int outer = coverage(xx, yy, x, y, w, h, r);
      if (!outer) continue;
      int inner = coverage(xx, yy, x + t, y + t, w - 2 * t, h - 2 * t, ri);
      int cov = outer - inner;
      if (cov > 0) blendPixel(xx, yy, c, cov);
    }
  }
}

void Canvas::roundRectGradient(int x, int y, int w, int h, int r, Color left, Color right) {
  if (w <= 0 || h <= 0) return;
  r = std::max(0, std::min(r, std::min(w, h) / 2));
  for (int xx = x; xx < x + w; xx++) {
    Color c = mix(left, right, w > 1 ? (float)(xx - x) / (w - 1) : 0);
    for (int yy = y; yy < y + h; yy++) {
      int cov = coverage(xx, yy, x, y, w, h, r);
      if (cov) blendPixel(xx, yy, c, cov);
    }
  }
}

void Canvas::circle(int cx, int cy, int r, Color c) { roundRect(cx - r, cy - r, 2 * r, 2 * r, r, c); }

void Canvas::card(int x, int y, int w, int h, int r, Color fillC, Color edge, Color accent, int accentW) {
  roundRect(x, y, w, h, r, edge);
  roundRect(x + 1, y + 1, w - 2, h - 2, std::max(0, r - 1), fillC);
  if (accentW > 0 && accent.a) {
    Rect old = clip_;
    setClip({x, y, accentW, h});
    Rect c2 = clip_;
    // intersect with the previous clip
    int x0 = std::max(old.x, c2.x), y0 = std::max(old.y, c2.y);
    int x1 = std::min(old.x + old.w, c2.x + c2.w), y1 = std::min(old.y + old.h, c2.y + c2.h);
    clip_ = {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
    roundRect(x, y, w, h, r, accent);
    clip_ = old;
  }
}

uint32_t utf8Next(const std::string &s, size_t &i) {
  unsigned char c = s[i++];
  if (c < 0x80) return c;
  int n = (c >= 0xf0) ? 3 : (c >= 0xe0) ? 2 : (c >= 0xc0) ? 1 : -1;
  if (n < 0) return 0xfffd;
  uint32_t cp = c & (0x3f >> n);
  for (int k = 0; k < n; k++) {
    if (i >= s.size() || (s[i] & 0xc0) != 0x80) return 0xfffd;
    cp = (cp << 6) | (s[i++] & 0x3f);
  }
  return cp;
}

const Glyph *findGlyph(const Font &f, uint32_t cp) {
  int lo = 0, hi = f.count - 1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    if (f.glyphs[mid].cp == cp) return &f.glyphs[mid];
    if (f.glyphs[mid].cp < cp) lo = mid + 1;
    else hi = mid - 1;
  }
  return nullptr;
}

static const Glyph *glyphOrFallback(const Font &f, uint32_t cp) {
  const Glyph *g = findGlyph(f, cp);
  if (g) return g;
  // fallbacks for characters a font lacks
  switch (cp) {
    case 0x2026: return findGlyph(f, '.');
    case 0x2013: case 0x2014: return findGlyph(f, '-');
    case 0x2018: case 0x2019: return findGlyph(f, '\'');
    case 0x201C: case 0x201D: return findGlyph(f, '"');
    default: return findGlyph(f, '?');
  }
}

int Canvas::textWidth(const Font &f, const std::string &s) { return textSpacedWidth(f, s, 0); }

int Canvas::textSpacedWidth(const Font &f, const std::string &s, int spacing) {
  int w = 0;
  size_t i = 0;
  int n = 0;
  while (i < s.size()) {
    uint32_t cp = utf8Next(s, i);
    const Glyph *g = glyphOrFallback(f, cp);
    if (g) w += g->adv + spacing;
    n++;
  }
  return n ? w - spacing : 0;
}

static void drawGlyph(Canvas &cv, const Font &f, const Glyph *g, int penX, int baseY, Color c) {
  int rowBytes = (g->w + 1) / 2;
  const unsigned char *bits = f.bits + g->off;
  for (int yy = 0; yy < g->h; yy++)
    for (int xx = 0; xx < g->w; xx++) {
      unsigned char b = bits[yy * rowBytes + xx / 2];
      int a = (xx & 1) ? (b & 0xf) : (b >> 4);
      if (a) cv.blendPixel(penX + g->x + xx, baseY + g->y + yy, c, a * 17);
    }
}

int Canvas::textSpaced(const Font &f, int x, int y, const std::string &s, Color c, int spacing) {
  int pen = x, base = y + f.ascent;
  size_t i = 0;
  while (i < s.size()) {
    uint32_t cp = utf8Next(s, i);
    const Glyph *g = glyphOrFallback(f, cp);
    if (!g) continue;
    drawGlyph(*this, f, g, pen, base, c);
    pen += g->adv + spacing;
  }
  return pen - x;
}

int Canvas::text(const Font &f, int x, int y, const std::string &s0, Color c, int maxW, Align align) {
  std::string s = s0;
  int w = textWidth(f, s);
  if (maxW > 0 && w > maxW) {
    // cut whole code points until it fits with an ellipsis
    std::string ell = findGlyph(f, 0x2026) ? "\xe2\x80\xa6" : "..";
    while (!s.empty() && textWidth(f, s + ell) > maxW) {
      size_t k = s.size() - 1;
      while (k > 0 && (s[k] & 0xc0) == 0x80) k--;
      s.erase(k);
    }
    while (!s.empty() && s.back() == ' ') s.pop_back();
    s += ell;
    w = textWidth(f, s);
  }
  int dx = x;
  if (maxW > 0 && align == ALIGN_CENTER) dx = x + (maxW - w) / 2;
  else if (maxW > 0 && align == ALIGN_RIGHT) dx = x + maxW - w;
  else if (maxW == 0 && align == ALIGN_CENTER) dx = x - w / 2;
  else if (maxW == 0 && align == ALIGN_RIGHT) dx = x - w;
  textSpaced(f, dx, y, s, c, 0);
  return w;
}

int Canvas::textWrapped(const Font &f, int x, int y, int w, const std::string &s, Color c, int lineGap, int maxLines) {
  int lh = lineHeight(f) + lineGap;
  int lines = 0;
  size_t pos = 0;
  while (pos < s.size()) {
    // longest prefix that fits, broken at a space
    size_t end = pos, lastSpace = std::string::npos;
    while (end < s.size()) {
      if (s[end] == '\n') break;
      size_t nx = end;
      utf8Next(s, nx);
      if (textWidth(f, s.substr(pos, nx - pos)) > w) break;
      if (s[end] == ' ') lastSpace = end;
      end = nx;
    }
    size_t cut = end;
    if (end < s.size() && s[end] != '\n' && lastSpace != std::string::npos && lastSpace > pos) cut = lastSpace;
    if (cut == pos) cut = end > pos ? end : pos + 1;
    bool last = maxLines && lines == maxLines - 1;
    std::string line = s.substr(pos, last ? std::string::npos : cut - pos);
    text(f, x, y + lines * lh, line, c, last ? w : 0);
    lines++;
    if (last) break;
    pos = cut;
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\n')) pos++;
  }
  return lines * lh;
}

void Canvas::draw(const Canvas &src, int dx, int dy) {
  for (int y = 0; y < src.h_; y++)
    for (int x = 0; x < src.w_; x++) {
      uint32_t p = src.px_[y * src.w_ + x];
      if (!(p >> 24)) continue;
      blendPixel(dx + x, dy + y, Color((p >> 16) & 0xff, (p >> 8) & 0xff, p & 0xff, p >> 24));
    }
}

}  // namespace sd
