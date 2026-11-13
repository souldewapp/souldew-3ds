#include "ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "i18n.h"

namespace sd {

Color typeColor(int t) {
  static const uint32_t C[18] = {0xa8a68e, 0xb56a63, 0xa59bd0, 0xa07aa8, 0xc2ae7a, 0xac9f6a, 0xa2ad62, 0x877ca2, 0xadadba,
                                 0xd39469, 0x869ed0, 0x8fb67a, 0xd6c06b, 0xcf849b, 0x9cc2c2, 0x8a77c8, 0x877b72, 0xd3a3ae};
  return (t >= 0 && t < 18) ? rgb(C[t]) : th::ink3;
}
Color toneColor(Tone t) {
  switch (t) {
    case Tone::Ok: return th::tri;
    case Tone::Danger: return th::cir;
    case Tone::Warn: return th::warn;
    case Tone::Muted: return th::edgeHi;
    default: return th::crs;
  }
}
Color toneInk(Tone t) {
  switch (t) {
    case Tone::Ok: return th::okInk;
    case Tone::Danger: return th::badInk;
    case Tone::Warn: return th::warnInk;
    case Tone::Muted: return th::ink2;
    default: return th::crsInk;
  }
}
Color gameColor(const std::string &g) {
  if (g == "xy") return th::crs;
  if (g == "oras") return th::cir;
  if (g == "sm") return th::warn;
  if (g == "usum") return th::sqr;
  return th::tri;
}
Color rarityColor(int r) {
  static const uint32_t C[5] = {0xc48a5c, 0xb6bdca, 0xe1b05a, 0x94c9c7, 0xc59af5};
  return rgb(C[std::max(0, std::min(4, r))]);
}

std::string shortName(int species) {
  // the desktop app shows a short label instead of a sprite where there is none: first letters, upper case
  std::string n = speciesName(species, g_lang);
  std::string out;
  size_t i = 0;
  int count = 0;
  while (i < n.size() && count < 4) {
    size_t st = i;
    uint32_t cp = utf8Next(n, i);
    if (cp == ' ' || cp == '-' || cp == '.' || cp == ':' || cp == '\'') continue;
    if (cp >= 'a' && cp <= 'z') out += (char)(cp - 32);
    else if (cp == 0xE4) out += "\xc3\x84";
    else if (cp == 0xF6) out += "\xc3\x96";
    else if (cp == 0xFC) out += "\xc3\x9c";
    else if (cp == 0xE9) out += "E";
    else out += n.substr(st, i - st);
    count++;
  }
  return out;
}

std::string monLabel(const RunMon &m) {
  std::string sp = speciesName(m.species, Lang::En), spDe = speciesName(m.species, Lang::De);
  if (!m.nickname.empty() && m.nickname != sp && m.nickname != spDe) return m.nickname;
  return speciesName(m.species, g_lang);
}

// ---- backdrop + logo -------------------------------------------------------------------------------------------------

bool g_cacheBackdrops = false;

void drawBackdrop(Canvas &c, Backdrop kind) {
  int w = c.width(), h = c.height();
  static std::vector<uint32_t> cache[3][2];
  std::vector<uint32_t> *slot = (g_cacheBackdrops && (w == 400 || w == 320) && h == 240) ? &cache[(int)kind][w == 400 ? 0 : 1] : nullptr;
  if (slot && !slot->empty()) {
    std::memcpy(c.pixels(), slot->data(), slot->size() * sizeof(uint32_t));
    return;
  }
  Color top = kind == Backdrop::Paused ? rgb(0x18171c) : rgb(0x1e1d23), bottom = th::stage;
  for (int y = 0; y < h; y++) c.fill(0, y, w, 1, mix(top, bottom, (float)y / (h - 1)));
  if (kind == Backdrop::Hero) {
    // the website's hero: a blue glow on the left, a pink one on the right
    c.glow(w * 18 / 100, h * 30 / 100, w * 46 / 100, h * 62 / 100, th::p1.alpha(30));
    c.glow(w * 82 / 100, h * 38 / 100, w * 46 / 100, h * 62 / 100, th::p2.alpha(24));
  } else if (kind == Backdrop::Plain) {
    c.glow(w / 6, 0, w * 2 / 3, h / 2, th::p1.alpha(12));
  } else {
    c.glow(w / 2, 0, w * 2 / 3, h / 2, th::lcd3.alpha(14));
  }
  if (slot) slot->assign(c.pixels(), c.pixels() + (size_t)w * h);
}

void drawLogoMark(Canvas &c, int x, int y, int s, float link, bool glow) {
  // the favicon's pixel art: ring 1 (blue), ring 2 (pink), the link bar between them
  struct R { int x, y, w, h; };
  static const R ring1[] = {{4, 2, 6, 2}, {2, 4, 2, 8}, {10, 4, 2, 2}, {10, 10, 2, 2}, {4, 12, 6, 2}};
  static const R ring2[] = {{22, 2, 6, 2}, {28, 4, 2, 8}, {20, 4, 2, 2}, {20, 10, 2, 2}, {22, 12, 6, 2}};
  float k = std::max(0.f, std::min(1.f, link));
  float e = 1 - (1 - k) * (1 - k) * (1 - k);  // ease out
  int off = (int)std::lround((1 - e) * 6) * s;  // the rings slide in from the sides
  if (glow) {
    c.glow(x + 7 * s - off, y + 8 * s, 11 * s, 9 * s, rgb(0x6f9bff, (uint8_t)(70 * e)));
    c.glow(x + 25 * s + off, y + 8 * s, 11 * s, 9 * s, rgb(0xff74d4, (uint8_t)(64 * e)));
  }
  for (auto &r : ring1) c.fill(x + r.x * s - off, y + r.y * s, r.w * s, r.h * s, th::p1);
  for (auto &r : ring2) c.fill(x + r.x * s + off, y + r.y * s, r.w * s, r.h * s, th::p2);
  if (k > 0.75f) {
    uint8_t a = (uint8_t)std::min(255.f, (k - 0.75f) * 4 * 255);
    if (glow) c.glow(x + 16 * s, y + 8 * s, 7 * s, 5 * s, rgb(0xeaffc9, (uint8_t)(a / 3)));
    c.fill(x + 12 * s, y + 6 * s, 8 * s, 4 * s, th::link.alpha(a));
  }
}

void drawLogo(Canvas &c, int x, int y, bool small) {
  // the app icon in small: a handheld body with an LCD showing the rings
  int w = 34, h = 26;
  c.roundRectVGradient(x, y, w, h, 7, rgb(0x2e2c38), rgb(0x0f0e13));
  c.roundRectStroke(x, y, w, h, 7, 1, Color(255, 255, 255, 22));
  c.roundRectVGradient(x + 3, y + 3, w - 6, 15, 3, rgb(0x1d2f24), rgb(0x0b120d));
  // the mark at 1:1 is 28×12 cells (x 2…29, y 2…13): crop into the LCD
  Rect old = c.clip();
  Rect lcd{x + 3, y + 3, w - 6, 15};
  int x0 = std::max(old.x, lcd.x), y0 = std::max(old.y, lcd.y);
  int x1 = std::min(old.x + old.w, lcd.x + lcd.w), y1 = std::min(old.y + old.h, lcd.y + lcd.h);
  c.setClip({x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)});
  c.glow(x + 10, y + 10, 9, 7, rgb(0x6f9bff, 60));
  c.glow(x + 24, y + 10, 9, 7, rgb(0xff74d4, 50));
  drawLogoMark(c, x + 1, y + 3, 1);
  c.setClip(old);
  c.fill(x + 5, y + 21, 2, 2, rgb(0xff4a3d));   // power LED
  c.fill(x + w - 10, y + 21, 2, 2, th::p2.alpha(200));
  c.fill(x + w - 7, y + 20, 2, 2, th::p1.alpha(200));
  if (!small) c.text(font_head, x + w + 8, y + 4, "souldew", th::ink);
}

// ---- icons (pixel art, '#' = filled) -------------------------------------------------------------------------------

struct IconDef {
  const char *name;
  int rows;
  const char *px[12];
};
static const IconDef ICONS[] = {
    {"play", 8, {"##......", "####....", "######..", "########", "########", "######..", "####....", "##......"}},
    {"back", 8, {"....##....", "...##.....", "..##......", ".#########", ".#########", "..##......", "...##.....", "....##...."}},
    {"plus", 8, {"...##...", "...##...", "...##...", "########", "########", "...##...", "...##...", "...##..."}},
    {"minus", 2, {"########", "########"}},
    {"check", 7, {"........##", ".......##.", "......##..", "##...##...", ".##.##....", "..###.....", "...#......"}},
    {"chevron", 7, {"##...", ".##..", "..##.", "...##", "..##.", ".##..", "##..."}},
    {"trophy", 9, {"#########", "#########", "##.###.##", ".#.###.#.", "...###...", "....#....", "....#....", "..#####..", "..#####.."}},
    {"gear", 9, {"...###...", ".#######.", ".##...##.", "###...###", "##.....##", "###...###", ".##...##.", ".#######.", "...###..."}},
    {"list", 8, {"##.######", "##.######", ".........", "##.######", "##.######", ".........", "##.######", "##.######"}},
    {"help", 9, {".######.", "##....##", "......##", "....###.", "...##...", "...##...", "........", "...##...", "...##..."}},
    {"power", 9, {"....#....", ".#..#..#.", "#...#...#", "#...#...#", "#.......#", "#.......#", "#.......#", ".#.....#.", "..#####.."}},
    {"globe", 9, {"..#####..", ".#..#..#.", "#..#.#..#", "#########", "#..#.#..#", "#########", "#..#.#..#", ".#..#..#.", "..#####.."}},
    {"eye", 7, {"...#####...", ".##.....##.", "#...###...#", "#..#####..#", "#...###...#", ".##.....##.", "...#####..."}},
    {"lock", 9, {"..####..", ".#....#.", ".#....#.", ".#....#.", "########", "########", "###..###", "###..###", "########"}},
    {"cross", 8, {"##....##", "###..###", ".######.", "..####..", "..####..", ".######.", "###..###", "##....##"}},
    {"star", 9, {"....#....", "....#....", "...###...", "#########", ".#######.", "..#####..", "..##.##..", ".##...##.", "##.....##"}},
    {"pin", 9, {"..####..", ".######.", "###..###", "###..###", ".######.", "..####..", "...##...", "...##...", "........"}},
    {"info", 8, {"##", "..", "##", "##", "##", "##", "##", "##"}},
    {"warn", 8, {"##", "##", "##", "##", "##", "..", "##", "##"}},
    {"pause", 8, {"##..##", "##..##", "##..##", "##..##", "##..##", "##..##", "##..##", "##..##"}},
    {"flag", 9, {"##.......", "#####....", "########.", "#####....", "##.......", "##.......", "##.......", "##.......", "##......."}},
    {"ball", 9, {"..#####..", ".#.....#.", "#.......#", "#.......#", "####.####", "#...#...#", "#.......#", ".#.....#.", "..#####.."}},
    {"bolt", 9, {".....##..", "....##...", "...##....", "..######.", "....##...", "...##....", "..##.....", ".##......", ".#......."}},
    {"grin", 9, {"..#####..", ".#.....#.", "#..#.#..#", "#.......#", "#.#...#.#", "#..###..#", "#.......#", ".#.....#.", "..#####.."}},
    {"crown", 7, {"#...#...#", "##.###.##", "#########", "#########", "#.#.#.#.#", "#########", "........."}},
    {"sparkle", 7, {"...#...", "...#...", "..###..", "#######", "..###..", "...#...", "...#..."}},
    {"heart", 7, {".##.##.", "#######", "#######", "#######", ".#####.", "..###..", "...#..."}},
    {"cart", 9, {"#######..", "#.....##.", "#.###..#.", "#.###..#.", "#.###..#.", "#.......#", "#.#.#.#.#", "#.#.#.#.#", "#########"}},
};

static const IconDef *icon(const char *name) {
  for (auto &i : ICONS)
    if (!std::strcmp(i.name, name)) return &i;
  return nullptr;
}
int iconWidth(const char *name, int scale) {
  const IconDef *d = icon(name);
  return d ? (int)std::strlen(d->px[0]) * scale : 0;
}
int drawIcon(Canvas &c, const char *name, int x, int cy, Color col, int scale) {
  const IconDef *d = icon(name);
  if (!d) return 0;
  int y = cy - d->rows * scale / 2;
  for (int j = 0; j < d->rows; j++)
    for (int i = 0; d->px[j][i]; i++)
      if (d->px[j][i] == '#') c.fill(x + i * scale, y + j * scale, scale, scale, col);
  return (int)std::strlen(d->px[0]) * scale;
}

// ---- small components ----------------------------------------------------------------------------------------------

void drawSurface(Canvas &c, Rect r, int rad, Color top, Color bottom, Color edge, int shadow) {
  if (shadow) c.shadow(r.x, r.y, r.w, r.h, rad, shadow, 2, 110);
  c.roundRect(r.x, r.y, r.w, r.h, rad, edge);
  c.roundRectVGradient(r.x + 1, r.y + 1, r.w - 2, r.h - 2, std::max(0, rad - 1), top, bottom);
  c.hline(r.x + rad, r.y + 1, r.w - 2 * rad, Color(255, 255, 255, 16));
}

void drawCard(Canvas &c, Rect r, int rad, bool focused) {
  if (focused) {
    c.roundRect(r.x - 2, r.y - 2, r.w + 4, r.h + 4, rad + 2, th::p1.alpha(70));
    drawSurface(c, r, rad, rgb(0x34333d), rgb(0x2a2931), th::p1.alpha(200));
  } else
    drawSurface(c, r, rad, rgb(0x29282f), rgb(0x232229), th::edge);
}

void drawSectionLabel(Canvas &c, int x, int y, const std::string &text, Color col, int lineTo) {
  int w = c.textSpaced(font_label, x, y, text, col, 1);
  if (lineTo > x + w + 8) c.hline(x + w + 6, y + 4, lineTo - (x + w + 6), Color(255, 255, 255, 18));
}

void drawSwitch(Canvas &c, int x, int y, bool on) {
  if (on) {
    c.roundRectVGradient(x, y, 28, 16, 8, th::playHi, th::play);
    c.roundRectStroke(x, y, 28, 16, 8, 1, rgb(0x6fae8f));
    c.circle(x + 20, y + 8, 6, th::ink);
  } else {
    c.roundRect(x, y, 28, 16, 8, th::well);
    c.roundRectStroke(x, y, 28, 16, 8, 1, th::edgeHi);
    c.circle(x + 8, y + 8, 6, th::ink4);
  }
}

void drawProgress(Canvas &c, int x, int y, int w, int h, int n, int total, Color col) {
  c.roundRect(x, y, w, h, h / 2, th::well);
  if (total <= 0 || n <= 0) return;
  int fw = std::max(h, (int)((long long)w * std::min(n, total) / total));
  c.roundRect(x, y, fw, h, h / 2, col);
  if (h >= 4) c.hline(x + h / 2, y, fw - h, Color(255, 255, 255, 50));
}

void drawPips(Canvas &c, int x, int y, int n, int total, Color on) {
  for (int i = 0; i < total; i++) {
    int px = x + i * 9;
    bool lit = i < n;
    // a 7×7 diamond
    for (int k = 0; k < 4; k++) {
      Color col = lit ? (k == 0 ? mix(on, th::ink, 0.5f) : on) : th::shell4;
      c.fill(px + 3 - k, y + k, 1 + 2 * k, 1, col);
      c.fill(px + 3 - k, y + 6 - k, 1 + 2 * k, 1, lit ? mix(on, th::stage, 0.25f) : th::shell4);
    }
  }
}

void drawPageDots(Canvas &c, int cx, int y, int n, int active) {
  int w = n * 10 + 10;
  int x = cx - w / 2;
  for (int i = 0; i < n; i++) {
    if (i == active) {
      c.roundRect(x, y, 16, 6, 3, th::p1);
      x += 22;
    } else {
      c.roundRect(x, y, 6, 6, 3, th::shell4);
      x += 10;
    }
  }
}

void drawMonTile(Canvas &c, int x, int y, int size, int species, bool dead, bool shiny) {
  // no sprites (not redistributable): a tile in the Pokémon's type colours – the first type on top, the second
  // fading in below – with a short name
  int t0 = speciesType(species, 0), t1 = speciesType(species, 1);
  Color c0 = typeColor(t0), c1 = t1 >= 0 ? typeColor(t1) : c0;
  if (dead) c0 = c1 = rgb(0x6a6670);
  int r = std::max(4, size / 4);
  c.roundRect(x, y, size, size, r, mix(th::wellEdge, c0, dead ? 0.35f : 0.62f));
  c.roundRectVGradient(x + 1, y + 1, size - 2, size - 2, r - 1, mix(th::shell, c0, dead ? 0.22f : 0.66f), mix(th::shell, c1, dead ? 0.12f : 0.40f));
  c.hline(x + r, y + 1, size - 2 * r, Color(255, 255, 255, dead ? 18 : 46));
  if (t1 >= 0 && !dead) {
    // the second type: a small notch in the bottom-right corner
    c.roundRect(x + size - 8, y + size - 8, 5, 5, 2, mix(th::wellEdge, c1, 0.5f));
    c.roundRect(x + size - 7, y + size - 7, 3, 3, 1, c1);
  }
  std::string s = shortName(species);
  const Font &f = font_label;
  int w = Canvas::textSpacedWidth(f, s, 0);
  if (w > size - 5 && s.size() > 3) {
    s = s.substr(0, 3);
    w = Canvas::textSpacedWidth(f, s, 0);
  }
  int tx = x + (size - w) / 2, ty = y + (size - Canvas::lineHeight(f)) / 2;
  c.text(f, tx, ty + 1, s, Color(0, 0, 0, 120));
  c.text(f, tx, ty, s, dead ? th::ink3 : mix(c0, rgb(0xffffff), 0.86f));
  if (shiny && !dead) drawIcon(c, "sparkle", x + size - 9, y + 6, th::warn);
  if (dead) {
    c.circle(x + size - 5, y + size - 5, 5, th::wellEdge);
    c.circle(x + size - 5, y + size - 5, 4, th::cir);
    for (int i = 0; i < 4; i++) {
      c.fill(x + size - 7 + i, y + size - 7 + i, 1, 1, th::ink);
      c.fill(x + size - 4 - i, y + size - 7 + i, 1, 1, th::ink);
    }
  }
}

int chipWidth(const std::string &text) { return Canvas::textSpacedWidth(font_label, text, 1) + 12; }

int drawChip(Canvas &c, int x, int y, const std::string &text, Color fg, Color bg, Color border) {
  int w = chipWidth(text), h = 13;
  if (!bg.a) bg = mix(th::well, fg, 0.10f);
  if (!border.a) border = mix(th::well, fg, 0.32f);
  c.roundRect(x, y, w, h, 6, border);
  c.roundRect(x + 1, y + 1, w - 2, h - 2, 5, bg);
  c.textSpaced(font_label, x + 6, y + 2, text, fg, 1);
  return w;
}

void drawHpBar(Canvas &c, int x, int y, int w, int h, int hp, int maxHp) {
  c.roundRect(x, y, w, h, h / 2, rgb(0x0f0e12));
  c.roundRectStroke(x, y, w, h, h / 2, 1, Color(255, 255, 255, 10));
  if (maxHp <= 0 || hp <= 0) return;
  float r = (float)hp / maxHp;
  Color col = r > 0.5f ? th::tri : r > 0.2f ? th::warn : th::cir;
  int fw = std::max(h, (int)(w * r + 0.5f));
  c.roundRect(x, y, fw, h, h / 2, col);
  c.hline(x + 1, y + 1, fw - 2, mix(col, rgb(0xffffff), 0.35f).alpha(140));
}

void drawStatTile(Canvas &c, int x, int y, int w, int h, int value, const char *label, Color valueColor) {
  drawSurface(c, {x, y, w, h}, 8, rgb(0x1f1e24), rgb(0x18171c), rgb(0x302f37));
  std::string v = std::to_string(value);
  c.text(font_num, x, y + 7, v, valueColor, w, ALIGN_CENTER);
  c.text(font_small, x, y + h - 17, label, th::ink3, w, ALIGN_CENTER);
}

void drawButton(Canvas &c, Rect r, const std::string &label, BtnStyle s, bool focused, const char *icon) {
  Color top, bottom, edge, fg;
  switch (s) {
    case BtnStyle::Primary: top = th::playHi; bottom = th::play; edge = rgb(0x2d4f3e); fg = rgb(0xf4f1ea); break;
    case BtnStyle::Accent: top = rgb(0x9cbcff); bottom = rgb(0x7f9ddb); edge = rgb(0x5d78b4); fg = rgb(0x12131c); break;
    case BtnStyle::Danger: top = rgb(0x47292c); bottom = rgb(0x3a2326); edge = rgb(0x7d4540); fg = th::badInk; break;
    default: top = rgb(0x302f37); bottom = rgb(0x26252c); edge = th::edge; fg = th::ink; break;
  }
  int rad = std::min(r.h / 2, 10);
  if (focused) c.roundRect(r.x - 3, r.y - 3, r.w + 6, r.h + 6, rad + 3, th::p1.alpha(190));
  if (focused) c.roundRect(r.x - 1, r.y - 1, r.w + 2, r.h + 2, rad + 1, th::stage);
  c.roundRect(r.x, r.y, r.w, r.h, rad, edge);
  c.roundRectVGradient(r.x + 1, r.y + 1, r.w - 2, r.h - 3, std::max(0, rad - 1), top, bottom);
  c.hline(r.x + rad, r.y + 1, r.w - 2 * rad, Color(255, 255, 255, s == BtnStyle::Accent ? 70 : 30));
  int tw = label.empty() ? 0 : std::min(Canvas::textWidth(font_body_b, label), r.w - 12 - (icon ? iconWidth(icon) + 7 : 0));
  int iw = icon ? iconWidth(icon) + (label.empty() ? 0 : 7) : 0;
  int tx = r.x + (r.w - tw - iw) / 2;
  int ty = r.y + (r.h - Canvas::lineHeight(font_body_b)) / 2 - 1;
  bool after = icon && !std::strcmp(icon, "chevron");  // "next": the arrow follows the label
  if (icon) drawIcon(c, icon, after ? tx + tw + 7 : tx, r.y + r.h / 2 - 1, fg);
  if (!label.empty()) c.text(font_body_b, after ? tx : tx + iw, ty, label, fg, r.w - 12 - iw);
}

void drawKeyHint(Canvas &c, int x, int y, const char *key, const char *label, int *outW) {
  // a small key cap: round for the face buttons, a pill for the others
  int kl = (int)std::strlen(key);
  int kw = kl == 1 ? 13 : Canvas::textSpacedWidth(font_label, key, 1) + 10;
  c.roundRect(x, y, kw, 13, kl == 1 ? 6 : 4, rgb(0x0f0e12));
  c.roundRectVGradient(x, y, kw, 12, kl == 1 ? 6 : 4, rgb(0x3b3a44), rgb(0x2c2b33));
  c.textSpaced(font_label, x + (kw - Canvas::textSpacedWidth(font_label, key, 1)) / 2 + (kl == 1 ? 1 : 0), y + 1, key, th::ink, 1);
  int lw = c.text(font_small, x + kw + 4, y - 1, label, th::ink3);
  if (outW) *outW = kw + 4 + lw;
}

void drawTabBar(Canvas &c, int y, const std::vector<std::string> &tabs, int active, std::vector<Hit> *hits, int hitBase,
                const std::vector<int> *counts) {
  int w = c.width(), n = (int)tabs.size();
  // a segmented control: a dark track with the active tab raised in it
  c.fill(0, y, w, 30, th::shell);
  c.roundRect(5, y + 4, w - 10, 23, 11, th::wellEdge);
  c.roundRect(6, y + 5, w - 12, 21, 10, th::well);
  int tw = (w - 12) / n;
  for (int i = 0; i < n; i++) {
    int x0 = 6 + i * tw, x1 = i == n - 1 ? w - 6 : x0 + tw;
    bool on = i == active;
    if (on) {
      c.roundRect(x0 + 1, y + 6, x1 - x0 - 2, 19, 9, th::edgeHi);
      c.roundRectVGradient(x0 + 2, y + 7, x1 - x0 - 4, 17, 8, rgb(0x3b3a44), rgb(0x2e2d35));
    }
    std::string label = tabs[i];
    int cnt = counts && i < (int)counts->size() ? (*counts)[i] : 0;
    int lw = Canvas::textWidth(font_body_b, label), cw = 0;
    std::string cs = cnt > 0 ? std::to_string(cnt) : "";
    if (cnt > 0) cw = Canvas::textSpacedWidth(font_label, cs, 1) + 8;
    int tx = x0 + (x1 - x0 - lw - (cw ? cw + 4 : 0)) / 2;
    c.text(font_body_b, tx, y + 7, label, on ? th::ink : th::ink3);
    if (cw) {
      c.roundRect(tx + lw + 4, y + 10, cw, 11, 5, on ? th::cir : rgb(0x4a2a2c));
      c.textSpaced(font_label, tx + lw + 8, y + 11, cs, on ? rgb(0x1b1a1f) : th::badInk, 1);
    }
    if (hits) hits->push_back({{i * w / n, y, w / n, 30}, hitBase, i});
  }
}

static const char *toneIcon(const Event &e) {
  switch (e.kind) {
    case EventKind::NewArea: return "pin";
    case EventKind::Death: case EventKind::RunLost: case EventKind::Whiteout: return "cross";
    case EventKind::Badge: return "star";
    case EventKind::Caught: case EventKind::Gift: return "check";
    case EventKind::OverCap: case EventKind::DupeEncounter: return "warn";
    case EventKind::Evolved: return "sparkle";
    default: return e.tone == Tone::Warn || e.tone == Tone::Danger ? "warn" : "info";
  }
}

void drawToast(Canvas &c, int x, int y, int w, const Event &e, float t) {
  if (t <= 0) return;
  int h = 44;
  int dy = (int)((1 - t) * -14);
  uint8_t a = (uint8_t)(240 * t);
  bool trophy = e.kind == EventKind::Trophy;
  Color accent = trophy ? rarityColor(e.rarity) : toneColor(e.tone);
  int yy = y + dy;
  c.shadow(x, yy, w, h, 10, 4, 2, (uint8_t)(150 * t));
  c.roundRect(x, yy, w, h, 10, mix(th::edge, accent, trophy ? 0.45f : 0.18f).alpha(a));
  c.roundRectVGradient(x + 1, yy + 1, w - 2, h - 2, 9, mix(rgb(0x2f2e36), accent, trophy ? 0.16f : 0.10f).alpha(a), rgb(0x1f1e24).alpha(a));
  c.hline(x + 10, yy + 1, w - 20, Color(255, 255, 255, (uint8_t)(26 * t)));
  c.roundRect(x + 1, yy + 9, 3, h - 18, 1, accent.alpha((uint8_t)(255 * t)));
  int tx;
  if (trophy) {
    drawCoin(c, x + 10, yy + 8, 2, e.cat, e.rarity, false, false);
    tx = x + 46;
  } else if (e.species > 0) {
    drawMonTile(c, x + 10, yy + 7, 30, e.species, e.kind == EventKind::Death);
    tx = x + 48;
  } else {
    c.circle(x + 24, yy + 22, 12, mix(rgb(0x1f1e24), accent, 0.28f).alpha(a));
    c.roundRectStroke(x + 12, yy + 10, 24, 24, 12, 1, accent.alpha((uint8_t)(160 * t)));
    const char *ic = toneIcon(e);
    drawIcon(c, ic, x + 24 - iconWidth(ic) / 2, yy + 22, toneInk(e.tone).alpha((uint8_t)(255 * t)));
    tx = x + 44;
  }
  Color ink = th::ink.alpha((uint8_t)(255 * t));
  c.text(font_body_b, tx, yy + 6, e.title, ink, x + w - tx - 10);
  Color sub = trophy ? mix(accent, th::ink, 0.3f) : toneInk(e.tone);
  if (!e.sub.empty()) c.text(font_small, tx, yy + 24, e.sub, sub.alpha((uint8_t)(255 * t)), x + w - tx - 10);
}

void drawBattleBadge(Canvas &c, int rightX, int y, const std::string &text, Tone tone) {
  int w = Canvas::textSpacedWidth(font_label, text, 1) + 24;
  int x = rightX - w;
  Color col = toneColor(tone);
  c.shadow(x, y, w, 17, 8, 3, 1, 120);
  c.roundRect(x, y, w, 17, 8, mix(rgb(0x0e0d11), col, 0.5f).alpha(230));
  c.roundRectVGradient(x + 1, y + 1, w - 2, 15, 7, mix(rgb(0x24232a), col, 0.12f).alpha(235), Color(16, 15, 20, 235));
  c.circle(x + 10, y + 8, 4, col.alpha(90));
  c.circle(x + 10, y + 8, 2, mix(col, th::ink, 0.3f));
  c.textSpaced(font_label, x + 17, y + 4, text, toneInk(tone), 1);
}

void drawCornerTab(Canvas &c, int x, int y, bool pressed) {
  // the small handle in the bottom screen's corner: hold it to open souldew – the logo's rings
  c.roundRect(x, y, 30, 18, 6, Color(14, 13, 18, pressed ? 245 : 200));
  c.roundRectStroke(x, y, 30, 18, 6, 1, (pressed ? th::p1 : th::edgeHi).alpha(pressed ? 255 : 180));
  Rect old = c.clip();
  c.setClip({x + 1, y + 1, 28, 16});
  drawLogoMark(c, x - 1, y + 1, 1);
  c.setClip(old);
  if (pressed) c.roundRect(x + 1, y + 1, 28, 16, 5, th::p1.alpha(40));
}

void drawScrollHint(Canvas &c, Rect a, int scroll, int content) {
  if (content <= a.h) return;
  int th_ = std::max(16, a.h * a.h / content);
  int ty = a.y + (a.h - th_) * scroll / std::max(1, content - a.h);
  c.roundRect(a.x + a.w - 4, a.y, 3, a.h, 1, Color(255, 255, 255, 10));
  c.roundRect(a.x + a.w - 4, ty, 3, th_, 1, th::edgeHi);
}

void drawActionBar(Canvas &c, int y) {
  // the list fades out under the bar
  for (int i = 0; i < 10; i++) c.fill(0, y - 10 + i, c.width(), 1, th::stage.alpha((uint8_t)(i * 22)));
  for (int yy = y; yy < c.height(); yy++) c.fill(0, yy, c.width(), 1, mix(rgb(0x18171c), th::stage, (float)(yy - y) / (c.height() - y)));
  c.hline(0, y, c.width(), Color(255, 255, 255, 14));
}

// ---- medals + coins (the desktop app's web/lib/medals.mjs, cell by cell) --------------------------------------------

static const uint32_t METALS[6][5] = {
    {0x2e170b, 0x7d4523, 0xb06d3c, 0xd39561, 0xf1c595},  // bronze
    {0x22252d, 0x737a88, 0xa5acba, 0xcbd1dc, 0xf5f8fc},  // silver
    {0x33230a, 0xa06c12, 0xd6a020, 0xefc647, 0xfff1ad},  // gold
    {0x16292c, 0x5f9598, 0x94c9c7, 0xc3ebe8, 0xf2fffe},  // platinum
    {0x22132f, 0x6a4bc6, 0x9a78f5, 0xef8fd8, 0xfff0fb},  // master
    {0x0b0a0e, 0x1d1c22, 0x25242b, 0x2c2b33, 0x36353e},  // locked
};
static const uint32_t RIBBONS[5][2] = {{0xe0a458, 0x9c6a2c}, {0x6fb39b, 0x3d7563}, {0xdc7268, 0x94403a}, {0xe1b05a, 0x9b7329}, {0xff8fdc, 0x6f9bff}};
static const char *const EMBLEMS[5][9] = {
    {"....##...", "....####.", "....#####", "....####.", "....##...", "....#....", "....#....", "...###...", "..#####.."},  // flag
    {"..#####..", ".#.....#.", "#.......#", "#.......#", "###.#.###", "#..#.#..#", "#.......#", ".#.....#.", "..#####.."},  // ball
    {".....##..", "....##...", "...##....", "..######.", "....##...", "...##....", "..##.....", ".##......", ".#......."},  // bolt
    {"..#####..", ".#.....#.", "#..#.#..#", "#.......#", "#.#...#.#", "#..###..#", "#.......#", ".#.....#.", "..#####.."},  // grin
    {".........", "#...#...#", "##.###.##", "#########", "#########", "#.#.#.#.#", "#########", ".........", "........."},  // crown
};
// the coins' emblems: 7×7, simpler than the medals' (a coin is 13 cells across)
static const char *const COIN7[6][7] = {
    {".#.....", ".####..", ".#####.", ".####..", ".#.....", ".#.....", "###...."},  // flag
    {"..###..", ".#...#.", "#.....#", "###.###", "#.....#", ".#...#.", "..###.."},  // ball
    {"....##.", "...##..", "..##...", ".#####.", "...##..", "..##...", ".##...."},  // bolt
    {"..###..", ".#...#.", "#.#.#.#", "#.....#", "#.###.#", ".#...#.", "..###.."},  // grin
    {"..###..", ".#...#.", ".#...#.", "#######", "###.###", "###.###", "#######"},  // lock
    {".#####.", "##...##", "....##.", "...##..", "...##..", ".......", "...##.."},  // ?
};

static bool medalInside(int kind, float x, float y) {
  switch (kind) {
    case 1: return std::fabs(y) <= 10 && std::fabs(x) * 0.866f + std::fabs(y) * 0.5f <= 9.6f;  // hexagon
    case 2: {                                                                                    // star
      float r = std::sqrt(x * x + y * y), a = std::atan2(y, x) + 3.14159265f / 2;
      float k = (std::cos(5 * a) + 1) / 2;
      return r <= 6.6f + 4.6f * std::pow(k, 1.4f);
    }
    case 4: {  // burst
      float r = std::sqrt(x * x + y * y), a = std::atan2(y, x);
      return r <= 9.4f + 1.6f * std::cos(8 * a);
    }
    default: return x * x + y * y <= 10.4f * 10.4f;  // circle
  }
}

// the part of an emblem that has ink (some – the crown – don't fill their 9×9 box)
struct Ink {
  int x0 = 0, y0 = 0, w = 0, h = 0;
};
static Ink emblemInk(const char *const *em, int ew, int eh) {
  int x0 = ew, x1 = -1, y0 = eh, y1 = -1;
  for (int j = 0; j < eh; j++)
    for (int i = 0; i < ew; i++)
      if (em[j][i] == '#') x0 = std::min(x0, i), x1 = std::max(x1, i), y0 = std::min(y0, j), y1 = std::max(y1, j);
  Ink k;
  if (x1 < 0) return k;
  k.x0 = x0, k.y0 = y0, k.w = x1 - x0 + 1, k.h = y1 - y0 + 1;
  return k;
}

// a body (shape) with outline, bevel lit from the top left, sheen and an engraved emblem, into `grid` (0 = empty).
// The centre (CX, CY) is a whole cell of an odd-sized body, so it has a middle column and row; the emblem's ink goes
// onto it (an even extent leans up/left – the light lip below-right evens it out). `k`: the bevel's scale (1 = medal).
static void metalBody(uint32_t *grid, int W, int H, int CX, int CY, bool (*inside)(int, float, float), int kind, const uint32_t *pal,
                      bool locked, const char *const *em, int ew, int eh, float k, int *inkBox = nullptr) {
  auto in = [&](int x, int y) { return x >= 0 && y >= 0 && x < W && y < H && inside(kind, (float)(x - CX), (float)(y - CY)); };
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      if (!in(x, y)) continue;
      float dx = (float)(x - CX), dy = (float)(y - CY);
      uint32_t col;
      if (!in(x - 1, y) || !in(x + 1, y) || !in(x, y - 1) || !in(x, y + 1)) col = pal[0];
      else {
        bool rim = !in(x - 2, y) || !in(x + 2, y) || !in(x, y - 2) || !in(x, y + 2);
        bool lit = dx + dy < 0;
        if (rim) col = lit ? pal[4] : pal[1];
        else {
          float d = dx + dy;
          col = d < -9 * k ? pal[3] : d < 6 * k ? pal[2] : pal[1];
          if (!locked && std::fabs(dx - dy + 3 * k) <= 0.6f && d < -5 * k) col = pal[4];
        }
      }
      grid[y * W + x] = col | 0xff000000u;
    }
  Ink ink = emblemInk(em, ew, eh);
  if (!ink.w) return;
  int ox = CX - ink.w / 2, oy = CY - ink.h / 2;  // = CX - ceil((w - 1) / 2)
  if (inkBox) inkBox[0] = ox, inkBox[1] = oy, inkBox[2] = ox + ink.w - 1, inkBox[3] = oy + ink.h - 1;
  auto at = [&](int i, int j) { return i >= 0 && j >= 0 && i < ink.w && j < ink.h && em[ink.y0 + j][ink.x0 + i] == '#'; };
  uint32_t groove = (locked ? pal[1] : pal[0]) | 0xff000000u, lip = (locked ? pal[3] : pal[4]) | 0xff000000u;
  for (int j = 0; j < ink.h; j++)
    for (int i = 0; i < ink.w; i++) {
      if (!at(i, j)) continue;
      int x = ox + i + 1, y = oy + j + 1;
      if (x < W && y < H && grid[y * W + x] && !at(i + 1, j + 1)) grid[y * W + x] = lip;
    }
  for (int j = 0; j < ink.h; j++)
    for (int i = 0; i < ink.w; i++)
      if (at(i, j) && grid[(oy + j) * W + ox + i]) grid[(oy + j) * W + ox + i] = groove;
}

static void blitGrid(Canvas &c, int x, int y, const uint32_t *grid, int W, int H, int s) {
  for (int j = 0; j < H; j++)
    for (int i = 0; i < W; i++) {
      uint32_t p = grid[j * W + i];
      if (p) c.fill(x + i * s, y + j * s, s, s, rgb(p & 0xffffff));
    }
}

static const int MEDAL_SHAPE[5] = {0, 1, 2, 0, 4};  // journey circle, collection hexagon, skill star, fun circle, champion burst

MedalCells medalCells(int kind, int tier) {
  MedalCells m;
  m.px.assign(MedalCells::W * MedalCells::H, 0);
  uint32_t *grid = m.px.data();
  const int W = MedalCells::W;
  bool locked = tier < 0;
  const uint32_t *pal = METALS[locked ? 5 : std::min(tier, 4)];
  kind = std::max(0, std::min(4, kind));
  // the ribbon: two tails behind the medal, leaning outwards, a V notch at the top
  uint32_t rib0 = locked ? 0x25242a : RIBBONS[kind][0], rib1 = locked ? 0x1c1b20 : RIBBONS[kind][1];
  for (int yy = 0; yy < 13; yy++) {
    int lean = (int)std::lround((12 - yy) / 4.0);
    int tails[2][2] = {{4 - lean, 10 - lean}, {14 + lean, 20 + lean}};
    for (auto &t : tails) {
      float mid = (t[0] + t[1]) / 2.f;
      for (int xx = t[0]; xx <= t[1]; xx++) {
        if (yy < 2 && std::fabs(xx - mid) < 2 - yy) continue;
        bool e = xx == t[0] || xx == t[1] || xx == (int)std::lround(mid);
        if (xx >= 0 && xx < W) grid[yy * W + xx] = (e ? rib1 : rib0) | 0xff000000u;
      }
    }
  }
  int box[4] = {0, 0, -1, -1};
  metalBody(grid, W, MedalCells::H, MedalCells::CX, MedalCells::CY, medalInside, MEDAL_SHAPE[kind], pal, locked, EMBLEMS[kind], 9, 9, 1.f, box);
  for (int i = 0; i < 4; i++) m.ink[i] = box[i];
  for (int y = 0; y < MedalCells::H; y++)
    for (int x = 0; x < W; x++) m.body.push_back(medalInside(MEDAL_SHAPE[kind], (float)(x - MedalCells::CX), (float)(y - MedalCells::CY)));
  return m;
}

void drawMedal(Canvas &c, int x, int y, int kind, int tier, int scale) {
  MedalCells m = medalCells(kind, tier);
  blitGrid(c, x, y, m.px.data(), MedalCells::W, MedalCells::H, scale);
}

static bool coinInside(int, float x, float y) { return x * x + y * y <= 6.4f * 6.4f; }

void drawCoin(Canvas &c, int x, int y, int scale, int cat, int rarity, bool locked, bool hidden) {
  const int W = 13, H = 13;
  uint32_t grid[W * H] = {};
  const uint32_t *pal = METALS[locked ? 5 : std::max(0, std::min(4, rarity))];
  const char *const *em = COIN7[locked ? (hidden ? 5 : 4) : std::max(0, std::min(3, cat))];
  metalBody(grid, W, H, 6, 6, coinInside, 0, pal, locked, em, 7, 7, 13 / 24.f);
  blitGrid(c, x, y, grid, W, H, scale);
}

// ---- top screen: header + team -------------------------------------------------------------------------------------

void drawRunHeader(Canvas &c, int x, int y, int w, const ViewCtx &ctx) {
  const Run &run = *ctx.run;
  const GameInfo *g = ctx.game;
  // the "game" box: a small LCD (the paused game / the game module)
  c.roundRect(x, y, 92, 56, 9, rgb(0x050407));
  c.roundRectVGradient(x + 3, y + 3, 86, 50, 6, rgb(0x1b2b20), rgb(0x0a100c));
  for (int yy = y + 4; yy < y + 52; yy += 2) c.hline(x + 4, yy, 84, Color(0, 0, 0, 40));
  c.glow(x + 46, y + 26, 40, 22, th::lcd3.alpha(40));
  if (ctx.inGame) {
    drawIcon(c, "pause", x + 43, y + 19, th::lcd4);
    c.text(font_label, x, y + 33, tr("PAUSIERT", "PAUSED"), th::lcd3, 92, ALIGN_CENTER);
  } else {
    c.text(font_head, x, y + 11, g ? g->shortName : "3DS", th::lcd4, 92, ALIGN_CENTER);
    c.text(font_label, x, y + 34, "NINTENDO 3DS", th::lcd3, 92, ALIGN_CENTER);
  }
  // the run card: name, game, badges, level cap
  int cx = x + 100, cw = w - 100;
  Color acc = gameColor(run.game);
  c.roundRect(cx, y, cw, 56, 10, mix(th::edge, acc, 0.25f));
  c.roundRectGradient(cx + 1, y + 1, cw - 2, 54, 9, mix(rgb(0x2a2931), acc, 0.20f), rgb(0x232229));
  c.hline(cx + 10, y + 1, cw - 20, Color(255, 255, 255, 20));
  c.roundRect(cx + 1, y + 1, 4, 54, 2, acc);
  std::string boss;
  int cap = run.levelCap(&boss);
  int capW = cap ? 74 : 0;
  int tw = cw - 24 - capW;
  c.text(font_head, cx + 13, y + 5, run.name, mix(acc, th::ink, 0.5f), tw);
  std::string sub = g ? g->name : run.game;
  std::string where = ctx.area.empty() ? run.lastArea : ctx.area;
  if (!where.empty()) {
    std::string label = areaLabel(g ? g->gen : 6, ctx.loc, where, g_lang);
    if (ctx.loc == 0)  // the stored key is the English name: show the German one where the run knows its id
      for (auto &e : run.encounters)
        if (e.area == where && e.loc) {
          label = areaLabel(g ? g->gen : 6, e.loc, where, g_lang);
          break;
        }
    sub += " \xc2\xb7 " + label;
  }
  c.text(font_small, cx + 13, y + 23, sub, th::ink2, tw);
  // progress: badges (Gen 6) / grand trials (Gen 7) as pips
  int total = g ? g->caps.count : 8, n = run.progress();
  drawPips(c, cx + 13, y + 41, n, total, th::warn);
  std::string prog = g && !g->caps.fromBadges ? fmt(tr("%d/%d Pr\xc3\xbc" "fungen", "%d/%d trials"), n, total)
                                              : fmt(tr("%d/%d Orden", "%d/%d badges"), n, total);
  c.text(font_small, cx + 13 + total * 9 + 4, y + 37, prog, th::ink3, tw - total * 9);
  if (cap) {
    int bx = cx + cw - capW - 6;
    c.vline(bx, y + 9, 38, Color(255, 255, 255, 16));
    c.textSpaced(font_label, bx + 10, y + 7, "CAP", th::ink3, 1);
    c.text(font_num, bx + 10, y + 17, std::to_string(cap), th::warn);
    c.text(font_small, bx + 10, y + 37, boss, th::ink3, capW - 14);
  }
  if (run.status != "active") {
    std::string st = run.status == "lost" ? tr("VERLOREN", "LOST") : tr("GEWONNEN", "WON");
    drawChip(c, cx + cw - capW - 14 - chipWidth(st), y + 6, st, run.status == "lost" ? th::badInk : th::warnInk);
  }
}

void drawTeamGrid(Canvas &c, int x, int y, const Run &run, const std::vector<Mon> *party) {
  // the party (live when the plugin reads it, else the run's last known party), six cards
  std::vector<const RunMon *> slots(6, nullptr);
  std::vector<const Mon *> live(6, nullptr);
  if (party) {
    for (size_t i = 0; i < party->size() && i < 6; i++) {
      live[i] = &(*party)[i];
      slots[i] = run.mon((*party)[i].key);
    }
  } else {
    for (auto &m : run.mons)
      if (m.inParty && m.partySlot >= 0 && m.partySlot < 6) slots[m.partySlot] = &m;
  }
  const int W = 123, H = 78, GAP = 6;
  for (int i = 0; i < 6; i++) {
    int cx = x + (i % 3) * (W + GAP), cy = y + (i / 3) * (H + 6);
    const RunMon *rm = slots[i];
    const Mon *lm = live[i];
    if (!rm && !lm) {
      c.roundRect(cx, cy, W, H, 10, Color(255, 255, 255, 5));
      c.dashedRoundRect(cx, cy, W, H, 10, th::edge, 3);
      // ⊖: an empty slot
      c.roundRectStroke(cx + W / 2 - 8, cy + H / 2 - 8, 16, 16, 8, 1, th::edgeHi);
      c.hline(cx + W / 2 - 5, cy + H / 2 - 1, 10, th::edgeHi);
      c.hline(cx + W / 2 - 5, cy + H / 2, 10, th::edgeHi);
      continue;
    }
    int species = lm ? lm->species : rm->species;
    int level = lm ? lm->level : rm->level;
    int hp = lm ? lm->hp : rm->hp, maxHp = lm ? lm->maxHp : rm->maxHp;
    bool dead = rm && rm->dead;
    bool egg = lm && lm->isEgg;
    std::string name = rm ? monLabel(*rm) : (lm->nickname.empty() ? speciesName(species, g_lang) : lm->nickname);
    if (egg) name = tr("Ei", "Egg");
    Color tc = typeColor(speciesType(species, 0));
    if (dead) drawSurface(c, {cx, cy, W, H}, 10, rgb(0x2e2427), rgb(0x261e21), rgb(0x4a3236));
    else drawSurface(c, {cx, cy, W, H}, 10, rgb(0x2b2a31), rgb(0x232229), th::edge);
    // a strip in the Pokémon's type colour
    c.roundRect(cx + 10, cy + 1, W - 20, 2, 1, (dead ? th::cir : tc).alpha(dead ? 200 : 170));
    drawMonTile(c, cx + 8, cy + 9, 30, species, dead, rm ? rm->shiny : (lm && lm->shiny));
    c.text(font_body_b, cx + 45, cy + 8, name, dead ? th::ink2 : th::ink, W - 51);
    int lx = cx + 45;
    lx += c.textSpaced(font_label, lx, cy + 26, fmt("LV %d", level), th::ink2, 1) + 5;
    if (dead) drawChip(c, lx, cy + 23, tr("TOT", "DEAD"), th::badInk);
    else if (lm && lm->status) {
      const char *st = lm->status == 0x40 ? "PAR" : lm->status <= 7 ? tr("SCH", "SLP") : lm->status == 0x20 ? tr("GEF", "FRZ") : lm->status == 0x10 ? tr("BRT", "BRN") : tr("GIF", "PSN");
      drawChip(c, lx, cy + 23, st, rgb(0xecd27a));
    }
    // HP: the desktop app's little "HP" label + bar
    c.roundRect(cx + 8, cy + 44, 15, 9, 2, rgb(0x121116));
    c.textSpaced(font_label, cx + 10, cy + 43, "HP", dead ? th::ink4 : th::warnInk, 0);
    drawHpBar(c, cx + 26, cy + 46, W - 34, 5, dead ? 0 : hp, maxHp);
    c.hline(cx + 8, cy + 58, W - 16, Color(255, 255, 255, 12));
    std::string hpText = egg ? "" : fmt("%d/%d", dead ? 0 : hp, maxHp);
    std::string area = rm ? areaLabel(run.game == "sm" || run.game == "usum" ? 7 : 6, rm->loc, rm->area, g_lang) : "";
    int hw = Canvas::textWidth(font_small, hpText);
    c.text(font_small, cx + 8, cy + 61, area, th::ink3, W - 22 - hw);
    c.text(font_small, cx + W - 8 - hw, cy + 61, hpText, dead ? th::ink4 : th::ink2);
  }
}

// ---- RunView -------------------------------------------------------------------------------------------------------

enum HitId { H_TAB = 1, H_ROW, H_BACK, H_START, H_RESUME, H_SHEET_A, H_SHEET_B, H_SHEET_C, H_CAP_MINUS, H_CAP_PLUS, H_DELETE, H_OVERLAYS, H_END };

void RunView::drawTop(Canvas &c, const ViewCtx &ctx) {
  drawBackdrop(c, ctx.inGame ? Backdrop::Paused : Backdrop::Plain);
  drawRunHeader(c, 8, 8, 384, ctx);
  drawTeamGrid(c, 8, 72, *ctx.run, ctx.party);
}

static void listRow(Canvas &c, Rect r, bool focused, Color accent = Color()) {
  drawCard(c, r, 9, focused);
  if (accent.a) c.roundRect(r.x + 1, r.y + 7, 3, r.h - 14, 1, accent);
}

void RunView::listTeam(Canvas &c, const ViewCtx &ctx, Rect a) {
  Run &run = *ctx.run;
  rows_.clear();
  // party first (slot order), then the box – alive only (the dead are in the graveyard)
  std::vector<int> idx;
  for (int s = 0; s < 6; s++)
    for (size_t i = 0; i < run.mons.size(); i++)
      if (!run.mons[i].dead && run.mons[i].inParty && run.mons[i].partySlot == s) idx.push_back((int)i);
  int partyCount = (int)idx.size();
  for (size_t i = 0; i < run.mons.size(); i++)
    if (!run.mons[i].dead && !run.mons[i].inParty) idx.push_back((int)i);
  const int RH = 36;
  bool box = (int)idx.size() > partyCount;
  content_[0] = (int)idx.size() * RH + 24 + (box ? 20 : 0);
  if (idx.empty()) {
    drawIcon(c, "ball", a.x + a.w / 2 - 9, a.y + 44, th::edgeHi, 2);
    c.text(font_body_b, a.x, a.y + 64, tr("Noch kein Team", "No team yet"), th::ink, a.w, ALIGN_CENTER);
    c.textWrapped(font_small, a.x + 40, a.y + 84, a.w - 80,
                  tr("Sobald das Spiel l\xc3\xa4uft, \xc3\xbc" "bernimmt souldew dein Team.",
                     "Once the game runs, souldew picks up your team."),
                  th::ink3);
    return;
  }
  int y = a.y + 6 - scroll_[0];
  drawSectionLabel(c, a.x + 12, y + 2, fmt(tr("IM TEAM \xc2\xb7 %d", "PARTY \xc2\xb7 %d"), partyCount), th::ink3, a.x + a.w - 14);
  y += 16;
  for (size_t k = 0; k < idx.size(); k++, y += RH) {
    if ((int)k == partyCount) {
      drawSectionLabel(c, a.x + 12, y + 4, fmt(tr("IN DER BOX \xc2\xb7 %d", "IN THE BOX \xc2\xb7 %d"), (int)idx.size() - partyCount), th::ink3, a.x + a.w - 14);
      y += 20;
    }
    const RunMon &m = run.mons[idx[k]];
    rows_.push_back(idx[k]);
    if (y + RH < a.y || y > a.y + a.h) continue;
    bool foc = (int)k == focus_;
    Rect r{a.x + 8, y, a.w - 18, RH - 4};
    listRow(c, r, foc);
    drawMonTile(c, r.x + 5, y + 4, 24, m.species, false, m.shiny);
    c.text(font_body_b, r.x + 36, y + 2, monLabel(m), th::ink, 140);
    std::string sub = fmt("Lv %d \xc2\xb7 ", m.level) + areaLabel(ctx.game ? ctx.game->gen : 6, m.loc, m.area, g_lang);
    c.text(font_small, r.x + 36, y + 17, sub, th::ink3, 150);
    if (m.inParty && m.maxHp > 0) drawHpBar(c, r.x + r.w - 62, y + 13, 52, 5, m.hp, m.maxHp);
    else drawChip(c, r.x + r.w - 8 - chipWidth("BOX"), y + 9, "BOX", th::ink2);
    hits_.push_back({{r.x, std::max(y, a.y), r.w, r.h}, H_ROW, (int)k});
  }
}

static const char *statusLabel(const std::string &s) {
  if (s == "caught") return tr("GEFANGEN", "CAUGHT");
  if (s == "gift") return tr("GESCHENK", "GIFT");
  if (s == "missed") return tr("VERPASST", "MISSED");
  if (s == "dupe") return "DUPE";
  return tr("OFFEN", "OPEN");
}

void RunView::listRoutes(Canvas &c, const ViewCtx &ctx, Rect a) {
  Run &run = *ctx.run;
  rows_.clear();
  std::vector<int> idx;
  for (int i = (int)run.encounters.size() - 1; i >= 0; i--) idx.push_back(i);  // newest first
  const int RH = 36;
  content_[1] = (int)idx.size() * RH + 26;
  if (idx.empty()) {
    drawIcon(c, "pin", a.x + a.w / 2 - 8, a.y + 44, th::edgeHi, 2);
    c.text(font_body_b, a.x, a.y + 64, tr("Noch keine Begegnung", "No encounter yet"), th::ink, a.w, ALIGN_CENTER);
    c.textWrapped(font_small, a.x + 30, a.y + 84, a.w - 60,
                  tr("Betritt ein neues Gebiet \xe2\x80\x93 die erste Begegnung dort z\xc3\xa4hlt.",
                     "Enter a new area \xe2\x80\x93 the first encounter there counts."),
                  th::ink3);
    return;
  }
  int gen = ctx.game ? ctx.game->gen : 6;
  int y = a.y + 6 - scroll_[1];
  drawSectionLabel(c, a.x + 12, y + 2,
                   fmt(tr("%d GEFANGEN \xc2\xb7 %d VERPASST \xc2\xb7 %d OFFEN", "%d CAUGHT \xc2\xb7 %d MISSED \xc2\xb7 %d OPEN"), run.caught(), run.missed(), run.open()),
                   th::ink3, a.x + a.w - 14);
  y += 16;
  for (size_t k = 0; k < idx.size(); k++, y += RH) {
    const Encounter &e = run.encounters[idx[k]];
    rows_.push_back(idx[k]);
    if (y + RH < a.y || y > a.y + a.h) continue;
    bool foc = (int)k == focus_;
    Color fg = (e.status == "caught" || e.status == "gift") ? th::okInk : e.status == "missed" ? th::badInk : e.status == "open" ? th::warnInk : th::ink3;
    Rect r{a.x + 8, y, a.w - 18, RH - 4};
    listRow(c, r, foc);
    // a status disc on the left
    const char *ic = (e.status == "caught" || e.status == "gift") ? "check" : e.status == "missed" ? "cross" : e.status == "open" ? "warn" : "minus";
    c.circle(r.x + 17, y + 16, 11, mix(th::well, fg, 0.16f));
    c.roundRectStroke(r.x + 6, y + 5, 22, 22, 11, 1, mix(th::well, fg, 0.45f));
    int iw = iconWidth(ic);
    if (std::string(ic) == "minus") drawIcon(c, "minus", r.x + 13, y + 16, fg);
    else drawIcon(c, ic, r.x + 17 - iw / 2, y + 16, fg);
    std::string place = e.starter ? tr("Starter", "Starter") : areaLabel(gen, e.loc, e.area, g_lang);
    c.text(font_body_b, r.x + 36, y + 2, place, th::ink, 150);
    const RunMon *m = e.monKey.empty() ? nullptr : run.mon(e.monKey);
    std::string sub = std::string(speciesName(e.species, g_lang)) + (e.level ? fmt(" \xc2\xb7 Lv %d", e.level) : "");
    if (m && m->dead) sub += tr(" \xc2\xb7 tot", " \xc2\xb7 dead");
    c.text(font_small, r.x + 36, y + 17, sub, m && m->dead ? th::badInk : th::ink3, 160);
    std::string st = statusLabel(e.status);
    drawChip(c, r.x + r.w - 8 - chipWidth(st), y + 9, st, fg);
    hits_.push_back({{r.x, std::max(y, a.y), r.w, r.h}, H_ROW, (int)k});
  }
}

void RunView::listGrave(Canvas &c, const ViewCtx &ctx, Rect a) {
  Run &run = *ctx.run;
  rows_.clear();
  std::vector<int> idx;
  for (size_t i = 0; i < run.mons.size(); i++)
    if (run.mons[i].dead) idx.push_back((int)i);
  std::sort(idx.begin(), idx.end(), [&](int l, int r) { return run.mons[l].deathTs > run.mons[r].deathTs; });
  const int RH = 46;
  content_[2] = (int)idx.size() * RH + 26;
  if (idx.empty()) {
    drawIcon(c, "heart", a.x + a.w / 2 - 7, a.y + 46, th::tri, 2);
    c.text(font_head, a.x, a.y + 64, tr("Noch niemand gefallen", "Nobody has fallen yet"), th::okInk, a.w, ALIGN_CENTER);
    c.text(font_small, a.x, a.y + 86, tr("Bleib vorsichtig.", "Stay careful."), th::ink3, a.w, ALIGN_CENTER);
    return;
  }
  int y = a.y + 6 - scroll_[2];
  drawSectionLabel(c, a.x + 12, y + 2, fmt(tr("IN ERINNERUNG \xc2\xb7 %d", "IN MEMORY \xc2\xb7 %d"), (int)idx.size()), th::badInk, a.x + a.w - 14);
  y += 16;
  for (size_t k = 0; k < idx.size(); k++, y += RH) {
    const RunMon &m = run.mons[idx[k]];
    rows_.push_back(idx[k]);
    if (y + RH < a.y || y > a.y + a.h) continue;
    bool foc = (int)k == focus_;
    Rect r{a.x + 8, y, a.w - 18, RH - 5};
    if (foc) {
      c.roundRect(r.x - 2, r.y - 2, r.w + 4, r.h + 4, 11, th::cir.alpha(70));
      drawSurface(c, r, 9, rgb(0x3a2b2e), rgb(0x2e2326), th::cir.alpha(200));
    } else
      drawSurface(c, r, 9, rgb(0x2e2427), rgb(0x261e21), rgb(0x4a3236));
    c.roundRect(r.x + 1, y + 8, 3, r.h - 16, 1, th::cir);
    drawMonTile(c, r.x + 9, y + 5, 30, m.species, true);
    int hx = r.x + 46;
    hx += c.text(font_body_b, hx, y + 3, monLabel(m), th::ink, 110) + 6;
    c.text(font_small, hx, y + 4, fmt("%s \xc2\xb7 Lv %d", speciesName(m.species, g_lang), m.level), th::ink3, r.x + r.w - hx - 8);
    drawIcon(c, "cross", r.x + 46, y + 23, th::cir);
    c.text(font_small, r.x + 58, y + 17, m.cause.empty() ? tr("gefallen", "fallen") : m.cause, th::badInk, r.w - 66);
    std::string caught = fmt(tr("Gefangen: %s", "Caught: %s"), areaLabel(ctx.game ? ctx.game->gen : 6, m.loc, m.area, g_lang).c_str());
    c.text(font_small, r.x + 46, y + 29, caught, th::ink3, r.w - 54);
    hits_.push_back({{r.x, std::max(y, a.y), r.w, r.h}, H_ROW, (int)k});
  }
}

static std::string ruleValue(const std::string &v) {
  if (v == "line") return tr("Linie", "line");
  if (v == "species") return tr("Art", "species");
  if (v == "off") return tr("aus", "off");
  if (v == "warn") return tr("warnen", "warn");
  if (v == "strict") return tr("streng", "strict");
  if (v == "party") return tr("an", "on");
  if (v == "area") return tr("an", "on");
  if (v == "free") return tr("aus", "off");
  return v;
}

void RunView::listRules(Canvas &c, const ViewCtx &ctx, Rect a) {
  Run &run = *ctx.run;
  rows_.clear();
  int y = a.y + 6 - scroll_[3];
  // level cap card
  std::string boss;
  int cap = run.levelCap(&boss);
  Rect card{a.x + 8, y, a.w - 18, 58};
  drawSurface(c, card, 10, rgb(0x2e2b26), rgb(0x24221f), mix(th::edge, th::warn, 0.3f));
  c.textSpaced(font_label, card.x + 12, y + 7, tr("LEVEL-CAP", "LEVEL CAP"), th::warnInk, 1);
  if (cap) {
    c.text(font_num, card.x + 12, y + 20, std::to_string(cap), th::warn);
    c.text(font_body_b, card.x + 52, y + 19, boss, th::ink, 110);
  } else c.text(font_body, card.x + 12, y + 22, tr("keine (Regel aus)", "none (rule off)"), th::ink3);
  if (ctx.game) drawPips(c, card.x + 12, y + 42, run.progress(), ctx.game->caps.count, th::warn);
  if (ctx.game && !ctx.game->caps.fromBadges) {
    // Gen 7: the player steps the grand trials
    Rect m{card.x + card.w - 86, y + 18, 34, 28}, p{card.x + card.w - 46, y + 18, 34, 28};
    drawButton(c, m, "", BtnStyle::Ghost, false, "minus");
    drawButton(c, p, "", BtnStyle::Ghost, false, "plus");
    c.text(font_label, card.x + card.w - 92, y + 7, tr("PR\xc3\x9c" "FUNGEN", "TRIALS"), th::ink3, 86, ALIGN_CENTER);
    hits_.push_back({m, H_CAP_MINUS, 0});
    hits_.push_back({p, H_CAP_PLUS, 0});
  } else if (ctx.game) {
    c.text(font_small, card.x + card.w - 120, y + 38, fmt(tr("%d Orden \xc2\xb7 live", "%d badges \xc2\xb7 live"), run.badges), th::ink3, 108, ALIGN_RIGHT);
  }
  y += 66;
  std::string pn = run.rules.presetName();
  drawSectionLabel(c, a.x + 12, y, tr("REGELN", "RULES"), th::ink3, a.x + a.w - 14);
  std::string mode = pn == "hardcore" ? "HARDCORE" : pn == "classic" ? tr("KLASSISCH", "CLASSIC") : tr("EIGENE", "CUSTOM");
  drawChip(c, a.x + a.w - 12 - chipWidth(mode), y - 3, mode, pn == "hardcore" ? th::badInk : th::crsInk);
  y += 14;
  struct Row {
    const char *label;
    std::string value;
    bool on;
  } rows[] = {
      {tr("Erste Begegnung pro Gebiet", "First encounter per area"), tr("immer", "always"), true},
      {tr("Gefallen = tot", "Fallen = dead"), tr("immer", "always"), true},
      {tr("Dupes-Klausel", "Dupes clause"), ruleValue(run.rules.dupes), run.rules.dupes != "off"},
      {tr("Level-Caps", "Level caps"), ruleValue(run.rules.levelCaps), run.rules.levelCaps != "off"},
      {tr("Blackout beendet Lauf", "Whiteout ends the run"), ruleValue(run.rules.blackout), run.rules.blackout == "party"},
      {tr("Geschenke z\xc3\xa4hlen", "Gifts count"), ruleValue(run.rules.gifts), run.rules.gifts == "area"},
      {tr("Shiny-Klausel", "Shiny clause"), run.rules.shinyClause ? tr("an", "on") : tr("aus", "off"), run.rules.shinyClause},
      {"Deathless", run.rules.deathless ? tr("an", "on") : tr("aus", "off"), run.rules.deathless},
      {tr("Keine Items im Kampf", "No items in battle"), run.rules.noBattleItems ? tr("an", "on") : tr("aus", "off"), run.rules.noBattleItems},
      {tr("Set-Modus", "Set mode"), run.rules.setMode ? tr("an", "on") : tr("aus", "off"), run.rules.setMode},
  };
  int ry = y;
  int n = (int)(sizeof rows / sizeof rows[0]);
  drawSurface(c, {a.x + 8, ry, a.w - 18, n * 22 + 4}, 10, rgb(0x26252c), rgb(0x222128), th::edge);
  y += 2;
  for (int i = 0; i < n; i++) {
    auto &r = rows[i];
    if (y + 22 > a.y && y < a.y + a.h) {
      c.circle(a.x + 20, y + 11, 3, r.on ? th::tri : th::shell4);
      c.text(font_body, a.x + 30, y + 3, r.label, r.on ? th::ink : th::ink3, 170);
      c.text(font_body_b, a.x + 150, y + 3, r.value, r.on ? th::okInk : th::ink4, a.w - 172, ALIGN_RIGHT);
      if (i < n - 1) c.hline(a.x + 18, y + 21, a.w - 38, Color(255, 255, 255, 10));
    }
    y += 22;
  }
  y += 12;
  // settings for the game + the run's end
  Rect ov{a.x + 8, y, a.w - 18, 34};
  drawSurface(c, ov, 10, rgb(0x26252c), rgb(0x222128), th::edge);
  drawIcon(c, "eye", ov.x + 10, y + 17, th::ink2);
  c.text(font_body_b, ov.x + 28, y + 3, tr("Overlays im Spiel", "Overlays in game"), th::ink, 170);
  c.text(font_small, ov.x + 28, y + 18, ctx.overlays ? tr("Karten in wichtigen Momenten", "cards in the important moments") : tr("aus \xe2\x80\x93 souldew schweigt", "off \xe2\x80\x93 souldew stays quiet"), th::ink3, 200);
  drawSwitch(c, ov.x + ov.w - 38, y + 9, ctx.overlays);
  hits_.push_back({ov, H_OVERLAYS, 0});
  y += 42;
  Rect end{a.x + 8, y, a.w - 18, 30};
  drawButton(c, end, run.status == "active" ? tr("Lauf beenden", "End the run") : tr("Lauf wieder aufnehmen", "Reopen the run"), BtnStyle::Ghost, false, "trophy");
  hits_.push_back({end, H_END, 0});
  y += 38;
  if (!ctx.inGame) {
    Rect del{a.x + 8, y, a.w - 18, 30};
    drawButton(c, del, tr("Lauf l\xc3\xb6schen", "Delete run"), BtnStyle::Danger, false, "cross");
    hits_.push_back({del, H_DELETE, 0});
    y += 38;
  }
  content_[3] = y + scroll_[3] - a.y;
}

void RunView::drawSheet(Canvas &c, const ViewCtx &ctx) {
  if (!sheet_) return;
  Run &run = *ctx.run;
  c.fill(0, 0, c.width(), c.height(), Color(6, 6, 9, 175));
  int x = 8, w = c.width() - 16, h = 124, y = c.height() - h - 8;
  drawSurface(c, {x, y, w, h}, 14, rgb(0x2c2b33), rgb(0x222128), th::edgeHi, 6);
  c.roundRect(x + w / 2 - 16, y + 6, 32, 3, 1, th::edgeHi);  // a grabber
  std::string title, sub, a, b;
  const char *ia = nullptr, *ib = nullptr;
  BtnStyle sa = BtnStyle::Primary, sb = BtnStyle::Ghost;
  int species = 0;
  if (sheet_ == 1 && sheetArg_ >= 0 && sheetArg_ < (int)run.mons.size()) {
    const RunMon &m = run.mons[sheetArg_];
    species = m.species;
    title = monLabel(m);
    sub = fmt("%s \xc2\xb7 Lv %d", speciesName(m.species, g_lang), m.level);
    if (m.dead && !m.cause.empty()) sub += " \xc2\xb7 " + m.cause;
    a = m.dead ? tr("Doch nicht tot", "Not dead after all") : tr("Als tot markieren", "Mark as dead");
    ia = m.dead ? "heart" : "cross";
    sa = m.dead ? BtnStyle::Primary : BtnStyle::Danger;
    b = tr("Abbrechen", "Cancel");
  } else if (sheet_ == 2 && sheetArg_ >= 0 && sheetArg_ < (int)run.encounters.size()) {
    const Encounter &e = run.encounters[sheetArg_];
    species = e.species;
    title = fmt("%s \xc2\xb7 %s", speciesName(e.species, g_lang), areaLabel(ctx.game ? ctx.game->gen : 6, e.loc, e.area, g_lang).c_str());
    sub = tr("Gefangen (z. B. in die Box) oder verpasst?", "Caught (e.g. into the box) or missed?");
    a = tr("Gefangen", "Caught");
    ia = "check";
    b = tr("Verpasst", "Missed");
    ib = "cross";
    sb = BtnStyle::Danger;
  } else if (sheet_ == 3) {
    title = tr("Lauf l\xc3\xb6schen?", "Delete run?");
    sub = fmt(tr("\xe2\x80\x9e%s\xe2\x80\x9c wird von der SD-Karte entfernt.", "\xe2\x80\x9c%s\xe2\x80\x9d is removed from the SD card."), run.name.c_str());
    a = tr("L\xc3\xb6schen", "Delete");
    sa = BtnStyle::Danger;
    b = tr("Abbrechen", "Cancel");
  } else if (sheet_ == 4) {
    if (run.status == "active") {
      title = tr("Wie ist der Lauf ausgegangen?", "How did the run end?");
      sub = tr("Champ besiegt = gewonnen. Das z\xc3\xa4hlt f\xc3\xbcr die Erfolge.", "Champion beaten = won. It counts for the achievements.");
      a = tr("Gewonnen", "Won");
      ia = "trophy";
      b = tr("Verloren", "Lost");
      ib = "cross";
      sb = BtnStyle::Danger;
    } else {
      title = tr("Lauf wieder aufnehmen?", "Reopen the run?");
      sub = tr("Er gilt dann wieder als aktiv.", "It counts as active again.");
      a = tr("Aufnehmen", "Reopen");
      ia = "play";
      b = tr("Abbrechen", "Cancel");
    }
  } else {
    sheet_ = 0;
    return;
  }
  int tx = x + 16;
  if (species) {
    drawMonTile(c, x + 14, y + 18, 32, species, sheet_ == 1 && run.mons[sheetArg_].dead);
    tx = x + 56;
  }
  c.text(font_head, tx, y + 17, title, th::ink, x + w - tx - 14);
  c.text(font_small, tx, y + 38, sub, th::ink2, x + w - tx - 14);
  Rect ra{x + 12, y + h - 50, (w - 36) / 2, 36}, rb{x + 24 + (w - 36) / 2, y + h - 50, (w - 36) / 2, 36};
  drawButton(c, ra, a, sa, focus_ == 0, ia);
  drawButton(c, rb, b, sb, focus_ == 1, ib);
  hits_.push_back({ra, H_SHEET_A, 0});
  hits_.push_back({rb, H_SHEET_B, 0});
}

void RunView::drawBottom(Canvas &c, const ViewCtx &ctx) {
  hits_.clear();
  drawBackdrop(c, ctx.inGame ? Backdrop::Paused : Backdrop::Plain);
  std::vector<std::string> tabs = {tr("Team", "Team"), tr("Routen", "Routes"), tr("Friedhof", "Graves"), tr("Regeln", "Rules")};
  std::vector<int> counts = {0, ctx.run->open(), ctx.run->dead(), 0};
  Rect area{0, 30, c.width(), 168};
  c.setClip(area);
  if (tab == 0) listTeam(c, ctx, area);
  else if (tab == 1) listRoutes(c, ctx, area);
  else if (tab == 2) listGrave(c, ctx, area);
  else listRules(c, ctx, area);
  c.resetClip();
  drawScrollHint(c, {area.x, area.y + 4, area.w - 1, area.h - 8}, scroll_[tab], content_[tab]);
  // the tab bar and the action bar stay on top
  drawTabBar(c, 0, tabs, tab, &hits_, H_TAB, &counts);
  drawActionBar(c, 199);
  if (ctx.inGame) {
    Rect r{10, 205, 300, 30};
    drawButton(c, r, tr("Zur\xc3\xbc" "ck zum Spiel", "Back to the game"), BtnStyle::Primary, false, "play");
    hits_.push_back({r, H_RESUME, 0});
  } else {
    Rect back{10, 205, 96, 30}, start{114, 205, 196, 30};
    drawButton(c, back, tr("Zur\xc3\xbc" "ck", "Back"), BtnStyle::Ghost, false, "back");
    drawButton(c, start, tr("Spiel starten", "Start game"), BtnStyle::Primary, false, "play");
    hits_.push_back({back, H_BACK, 0});
    hits_.push_back({start, H_START, 0});
  }
  drawSheet(c, ctx);
}

ViewAction RunView::handle(const Input &in, ViewCtx &ctx, long long now) {
  Run &run = *ctx.run;
  // a tap: the hit under the finger when it lifts (the last drawn frame's hits; the sheet's come last = on top)
  int tapId = 0, tapArg = 0;
  static int downX = 0, downY = 0;
  static bool dragging = false;
  if (in.touchDown) {
    downX = in.tx;
    downY = in.ty;
    dragging = false;
  }
  if (in.touchHeld && !sheet_ && std::abs(in.ty - downY) > 6 && downY > 30 && downY < 198) dragging = true;
  if (in.touchHeld && dragging) {
    scroll_[tab] = std::max(0, std::min(std::max(0, content_[tab] - 168), scroll_[tab] - in.scrollDy));
  }
  if (in.touchUp && !dragging) {
    for (int i = (int)hits_.size() - 1; i >= 0; i--)
      if (hits_[i].r.contains(in.tx, in.ty) && hits_[i].r.contains(downX, downY)) {
        tapId = hits_[i].id;
        tapArg = hits_[i].arg;
        break;
      }
    if (sheet_ && !tapId) {
      sheet_ = 0;
      return ViewAction::Changed;
    }
  }
  if (sheet_) {
    if (in.pressed & (K_LEFT | K_RIGHT)) {
      focus_ = 1 - focus_;
      return ViewAction::Changed;
    }
    if (in.pressed & K_A) tapId = focus_ == 0 ? H_SHEET_A : H_SHEET_B;
    if (in.pressed & K_B) {
      sheet_ = 0;
      focus_ = 0;
      return ViewAction::Changed;
    }
    if (tapId == H_SHEET_A || tapId == H_SHEET_B) {
      bool a = tapId == H_SHEET_A;
      int s = sheet_;
      sheet_ = 0;
      focus_ = 0;
      if (s == 1 && sheetArg_ < (int)run.mons.size()) {
        RunMon &m = run.mons[sheetArg_];
        if (a) Engine::setDead(run, m, !m.dead, tr("manuell", "by hand"), now);
        return ViewAction::Changed;
      }
      if (s == 2 && sheetArg_ < (int)run.encounters.size()) {
        Engine::resolve(run, run.encounters[sheetArg_], a ? "caught" : "missed", now);
        return ViewAction::Changed;
      }
      if (s == 3 && a) return ViewAction::DeleteRun;
      if (s == 4) {
        if (run.status == "active") {
          run.end(a ? "won" : "lost", now);
          run.addLog(now, "run", a ? tr("Lauf gewonnen", "Run won") : tr("Lauf verloren", "Run lost"));
        } else if (a) run.end("active", now);
      }
      return ViewAction::Changed;
    }
    return ViewAction::None;
  }
  if (in.pressed & K_L) tapId = H_TAB, tapArg = (tab + 3) % 4;
  if (in.pressed & K_R) tapId = H_TAB, tapArg = (tab + 1) % 4;
  int rows = (int)rows_.size();
  if (in.pressed & K_DOWN) {
    if (tab == 3) scroll_[3] = std::min(std::max(0, content_[3] - 168), scroll_[3] + 22);
    else if (rows) focus_ = std::min(rows - 1, focus_ + 1);
  }
  if (in.pressed & K_UP) {
    if (tab == 3) scroll_[3] = std::max(0, scroll_[3] - 22);
    else focus_ = std::max(0, focus_ - 1);
  }
  if ((in.pressed & (K_UP | K_DOWN)) && tab != 3) {
    // keep the focused row visible (the rows start below a section label)
    int rh = tab == 2 ? 46 : 36;
    int top = 22 + focus_ * rh, bottom = top + rh;
    if (top - 22 < scroll_[tab]) scroll_[tab] = std::max(0, top - 22);
    if (bottom + 24 > scroll_[tab] + 168) scroll_[tab] = bottom + 24 - 168;
    scroll_[tab] = std::max(0, std::min(scroll_[tab], std::max(0, content_[tab] - 168)));
  }
  if ((in.pressed & K_A) && tab != 3 && rows) tapId = H_ROW, tapArg = focus_;
  if (in.pressed & K_B) return ctx.inGame ? ViewAction::ResumeGame : ViewAction::Back;
  if (ctx.inGame && (in.pressed & (K_SELECT | K_START))) return ViewAction::ResumeGame;
  if (!ctx.inGame && (in.pressed & K_START)) return ViewAction::StartGame;
  switch (tapId) {
    case H_TAB:
      tab = tapArg;
      focus_ = 0;
      return ViewAction::Changed;
    case H_ROW:
      focus_ = tapArg;
      if (tapArg < rows) {
        int idx = rows_[tapArg];
        if (tab == 0 || tab == 2) {
          sheet_ = 1;
          sheetArg_ = idx;
        } else if (tab == 1) {
          const Encounter &e = run.encounters[idx];
          if (!e.starter && (e.status == "open" || e.status == "missed" || (e.status == "caught" && e.monKey.rfind("box-", 0) == 0))) {
            sheet_ = 2;
            sheetArg_ = idx;
          }
        }
      }
      return ViewAction::Changed;
    case H_BACK: return ViewAction::Back;
    case H_START: return ViewAction::StartGame;
    case H_RESUME: return ViewAction::ResumeGame;
    case H_CAP_MINUS:
      if (run.milestone > 0) {
        run.milestone--;
        if ((int)run.stats.badgeAt.size() > run.milestone) run.stats.badgeAt.resize(run.milestone);
      }
      return ViewAction::Changed;
    case H_CAP_PLUS:
      if (ctx.game && run.milestone < ctx.game->caps.count) {
        run.milestone++;
        while ((int)run.stats.badgeAt.size() < run.milestone) run.stats.badgeAt.push_back(now);
      }
      return ViewAction::Changed;
    case H_DELETE:
      sheet_ = 3;
      focus_ = 1;
      return ViewAction::Changed;
    case H_END:
      sheet_ = 4;
      focus_ = 0;
      return ViewAction::Changed;
    case H_OVERLAYS: return ViewAction::ToggleOverlays;
  }
  return (in.touchHeld && dragging) ? ViewAction::Changed : ViewAction::None;
}

}  // namespace sd
