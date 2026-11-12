#include "ui.h"

#include <algorithm>
#include <cstdio>
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

std::string shortName(int species) {
  // the desktop app shows a short label instead of a sprite where there is none: first letters, upper case
  std::string n = speciesName(species, g_lang);
  std::string out;
  size_t i = 0;
  int count = 0;
  while (i < n.size() && count < 4) {
    size_t st = i;
    uint32_t cp = utf8Next(n, i);
    if (cp == ' ' || cp == '-' || cp == '.' || cp == ':') continue;
    if (cp >= 'a' && cp <= 'z') out += (char)(cp - 32);
    else if (cp == 0xE4) out += "\xc3\x84";
    else if (cp == 0xF6) out += "\xc3\x96";
    else if (cp == 0xFC) out += "\xc3\x9c";
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

void drawLogo(Canvas &c, int x, int y, bool small) {
  int s = small ? 16 : 24;
  c.card(x, y, s, s, small ? 4 : 6, th::shell3, th::edge);
  // two linked rings: the SoulLink mark
  int r = small ? 3 : 4, cy = y + s / 2;
  int cx1 = x + s / 2 - r + 1, cx2 = x + s / 2 + r - 1;
  for (int k = 0; k < 2; k++) {
    int cx = k ? cx2 : cx1;
    c.circle(cx, cy, r + 1, th::crs);
    c.circle(cx, cy, r - 1 + (small ? 0 : 0), th::shell3);
  }
  if (!small) c.text(font_head, x + s + 7, y + 2, "souldew", th::ink);
}

void drawMonTile(Canvas &c, int x, int y, int size, int species, bool dead, bool shiny) {
  Color tc = typeColor(speciesType(species, 0));
  Color t2 = speciesType(species, 1) >= 0 ? typeColor(speciesType(species, 1)) : tc;
  if (dead) tc = t2 = th::ink3;
  int r = std::max(4, size / 5);
  c.roundRect(x, y, size, size, r, mix(th::shell, tc, dead ? 0.12f : 0.22f));
  c.roundRectStroke(x, y, size, size, r, 1, tc.alpha(dead ? 90 : 150));
  // second type: a small corner wedge
  if (t2.r != tc.r || t2.g != tc.g || t2.b != tc.b) c.roundRect(x + size - 9, y + 3, 6, 6, 2, t2);
  std::string s = shortName(species);
  const Font &f = font_label;
  int w = Canvas::textSpacedWidth(f, s, 0);
  if (w > size - 4 && s.size() > 3) {
    s = s.substr(0, 3);
    w = Canvas::textSpacedWidth(f, s, 0);
  }
  c.text(f, x + (size - w) / 2, y + (size - Canvas::lineHeight(f)) / 2 + 1, s, dead ? th::ink3 : mix(tc, th::ink, 0.55f));
  if (shiny && !dead) {
    c.fill(x + 3, y + 3, 2, 2, th::warn);
    c.fill(x + 5, y + 5, 1, 1, th::warn);
  }
  if (dead) {
    // a small cross: dead, never colour alone
    for (int i = 0; i < 6; i++) {
      c.blendPixel(x + size - 9 + i, y + size - 9 + i, th::cir);
      c.blendPixel(x + size - 4 - i, y + size - 9 + i, th::cir);
    }
  }
}

int chipWidth(const std::string &text) { return Canvas::textSpacedWidth(font_label, text, 1) + 12; }

int drawChip(Canvas &c, int x, int y, const std::string &text, Color fg, Color bg, Color border) {
  int w = chipWidth(text), h = 13;
  c.roundRect(x, y, w, h, 6, border);
  c.roundRect(x + 1, y + 1, w - 2, h - 2, 5, bg);
  c.textSpaced(font_label, x + 6, y + 2, text, fg, 1);
  return w;
}

void drawHpBar(Canvas &c, int x, int y, int w, int h, int hp, int maxHp) {
  c.roundRect(x, y, w, h, h / 2, rgb(0x0f0e12));
  if (maxHp <= 0 || hp <= 0) return;
  float r = (float)hp / maxHp;
  Color col = r > 0.5f ? th::tri : r > 0.2f ? th::warn : th::cir;
  int fw = std::max(h, (int)(w * r + 0.5f));
  c.roundRect(x, y, fw, h, h / 2, col);
}

void drawStatTile(Canvas &c, int x, int y, int w, int h, int value, const char *label, Color valueColor) {
  c.card(x, y, w, h, 7, rgb(0x1d1c22), rgb(0x302f37));
  std::string v = std::to_string(value);
  c.text(font_num, x, y + 6, v, valueColor, w, ALIGN_CENTER);
  c.text(font_small, x, y + h - 16, label, th::ink3, w, ALIGN_CENTER);
}

void drawButton(Canvas &c, Rect r, const std::string &label, BtnStyle s, bool focused, const char *icon) {
  Color bg, edge, fg;
  switch (s) {
    case BtnStyle::Primary: bg = rgb(0x24332f); edge = rgb(0x4f8574); fg = th::okInk; break;
    case BtnStyle::Accent: bg = th::crs; edge = rgb(0x9db4e6); fg = rgb(0x15141a); break;
    case BtnStyle::Danger: bg = rgb(0x3a2426); edge = rgb(0x8a4a45); fg = th::badInk; break;
    default: bg = th::shell3; edge = th::edge; fg = th::ink; break;
  }
  int rad = std::min(r.h / 2, 14);
  if (focused) c.roundRect(r.x - 2, r.y - 2, r.w + 4, r.h + 4, rad + 2, th::crs.alpha(150));
  c.card(r.x, r.y, r.w, r.h, rad, bg, edge);
  if (s != BtnStyle::Accent) c.hline(r.x + rad, r.y + 1, r.w - 2 * rad, Color(255, 255, 255, 14));
  int tw = label.empty() ? 0 : Canvas::textWidth(font_body_b, label);
  int iw = icon ? (label.empty() ? 10 : 16) : 0;
  int tx = r.x + (r.w - tw - iw) / 2;
  int ty = r.y + (r.h - Canvas::lineHeight(font_body_b)) / 2;
  if (icon) drawIcon(c, icon, tx, r.y + r.h / 2, fg);
  if (!label.empty()) c.text(font_body_b, tx + iw, ty, label, fg, r.w - 12 - iw);
}

void drawIcon(Canvas &c, const char *name, int x, int cy, Color col) {
  std::string n = name;
  if (n == "play") {
    for (int i = 0; i < 9; i++) {
      int h = 9 - i;
      c.fill(x + i, cy - h / 2 - (h & 1 ? 0 : 0), 1, h, col);
      if (i < 8) c.blendPixel(x + i, cy - h / 2 - 1, col, 90);
    }
  } else if (n == "back") {
    c.fill(x + 1, cy - 1, 9, 2, col);
    for (int i = 0; i < 5; i++) {
      c.fill(x + i, cy - i - 1, 2, 1, col);
      c.fill(x + i, cy + i, 2, 1, col);
    }
  } else if (n == "plus") {
    c.fill(x, cy - 1, 10, 2, col);
    c.fill(x + 4, cy - 5, 2, 10, col);
  } else if (n == "minus") {
    c.fill(x, cy - 1, 10, 2, col);
  } else if (n == "check") {
    for (int i = 0; i < 4; i++) c.fill(x + i, cy + i - 1, 2, 2, col);
    for (int i = 0; i < 6; i++) c.fill(x + 4 + i, cy + 2 - i, 2, 2, col);
  }
}

void drawKeyHint(Canvas &c, int x, int y, const char *key, const char *label, int *outW) {
  int kw = Canvas::textSpacedWidth(font_label, key, 1) + 8;
  c.roundRect(x, y, kw, 12, 3, th::shell4);
  c.textSpaced(font_label, x + 4, y + 2, key, th::ink, 1);
  int lw = c.text(font_small, x + kw + 4, y - 1, label, th::ink3);
  if (outW) *outW = kw + 4 + lw;
}

void drawTabBar(Canvas &c, int y, const std::vector<std::string> &tabs, int active, std::vector<Hit> *hits, int hitBase) {
  int w = c.width(), n = (int)tabs.size();
  c.fill(0, y, w, 30, th::shell2);
  c.hline(0, y + 29, w, th::edge);
  for (int i = 0; i < n; i++) {
    int x0 = i * w / n, x1 = (i + 1) * w / n;
    bool on = i == active;
    c.text(font_body_b, x0, y + 7, tabs[i], on ? th::ink : th::ink3, x1 - x0, ALIGN_CENTER);
    if (on) c.roundRect(x0 + 14, y + 26, x1 - x0 - 28, 3, 1, th::crs);
    if (hits) hits->push_back({{x0, y, x1 - x0, 30}, hitBase, i});
  }
}

void drawToast(Canvas &c, int x, int y, int w, const Event &e, float t) {
  if (t <= 0) return;
  int h = 42;
  int dy = (int)((1 - t) * -14);
  uint8_t a = (uint8_t)(235 * t);
  Color accent = toneColor(e.tone);
  c.roundRect(x - 1, y + dy - 1 + 2, w + 2, h + 2, 10, Color(0, 0, 0, (uint8_t)(90 * t)));  // soft shadow
  c.roundRect(x, y + dy, w, h, 9, th::edge.alpha(a));
  c.roundRectGradient(x + 1, y + dy + 1, w - 2, h - 2, 8, mix(th::shell3, accent, 0.10f).alpha(a), th::shell2.alpha(a));
  c.roundRect(x + 1, y + dy + 6, 3, h - 12, 1, accent.alpha((uint8_t)(255 * t)));
  int tx = x + 12;
  if (e.species > 0) {
    drawMonTile(c, x + 10, y + dy + 7, 28, e.species, e.kind == EventKind::Death);
    tx = x + 46;
  } else {
    c.circle(x + 16, y + dy + 21, 4, accent.alpha((uint8_t)(255 * t)));
    tx = x + 28;
  }
  Color ink = th::ink.alpha((uint8_t)(255 * t));
  c.text(font_body_b, tx, y + dy + 5, e.title, ink, x + w - tx - 8);
  if (!e.sub.empty()) c.text(font_small, tx, y + dy + 22, e.sub, toneInk(e.tone).alpha((uint8_t)(255 * t)), x + w - tx - 8);
}

void drawBattleBadge(Canvas &c, int rightX, int y, const std::string &text, Tone tone) {
  int w = Canvas::textSpacedWidth(font_label, text, 1) + 22;
  int x = rightX - w;
  Color col = toneColor(tone);
  c.roundRect(x, y, w, 16, 8, Color(14, 13, 17, 215));
  c.roundRectStroke(x, y, w, 16, 8, 1, col.alpha(170));
  c.circle(x + 9, y + 8, 3, col);
  c.textSpaced(font_label, x + 16, y + 4, text, toneInk(tone), 1);
}

void drawCornerTab(Canvas &c, int x, int y, bool pressed) {
  // the small handle in the bottom screen's corner: hold it to open souldew
  c.roundRect(x, y, 30, 18, 6, Color(20, 19, 24, pressed ? 240 : 190));
  c.roundRectStroke(x, y, 30, 18, 6, 1, (pressed ? th::crs : th::edgeHi).alpha(pressed ? 255 : 170));
  for (int k = 0; k < 2; k++) {
    int cx = x + 12 + k * 6;
    c.circle(cx, y + 9, 4, th::crs.alpha(pressed ? 255 : 200));
    c.circle(cx, y + 9, 2, Color(20, 19, 24, 255));
  }
}

void drawScrollHint(Canvas &c, Rect a, int scroll, int content) {
  if (content <= a.h) return;
  int th_ = std::max(16, a.h * a.h / content);
  int ty = a.y + (a.h - th_) * scroll / std::max(1, content - a.h);
  c.roundRect(a.x + a.w - 4, a.y, 3, a.h, 1, th::shell3);
  c.roundRect(a.x + a.w - 4, ty, 3, th_, 1, th::edgeHi);
}

// ---- top screen: header + team -------------------------------------------------------------------------------------

void drawRunHeader(Canvas &c, int x, int y, int w, const ViewCtx &ctx) {
  const Run &run = *ctx.run;
  // the "game" box (paused game / game module)
  c.card(x, y, 92, 54, 8, th::lcd0, th::lcd2);
  if (ctx.inGame) {
    c.text(font_label, x, y + 15, tr("SPIEL", "GAME"), th::lcd3, 92, ALIGN_CENTER);
    c.text(font_label, x, y + 27, tr("PAUSIERT", "PAUSED"), th::lcd3, 92, ALIGN_CENTER);
  } else {
    c.text(font_label, x, y + 15, ctx.game ? ctx.game->shortName : "3DS", th::lcd4, 92, ALIGN_CENTER);
    c.text(font_label, x, y + 27, "NINTENDO 3DS", th::lcd3, 92, ALIGN_CENTER);
  }
  int cx = x + 100, cw = w - 100;
  Color acc = gameColor(run.game);
  c.roundRect(cx, y, cw, 54, 10, th::edge);
  c.roundRectGradient(cx + 1, y + 1, cw - 2, 52, 9, mix(rgb(0x26252d), acc, 0.14f), rgb(0x26252d));
  c.roundRect(cx + 1, y + 1, 4, 52, 2, acc);
  c.text(font_head, cx + 12, y + 4, run.name, th::crsInk, cw - 24);
  std::string sub = ctx.game ? ctx.game->name : run.game;
  const GameInfo *g = ctx.game;
  if (g) {
    if (g->caps.fromBadges) sub += fmt(tr(" \xc2\xb7 %d Orden", " \xc2\xb7 %d badges"), run.badges);
    else sub += fmt(tr(" \xc2\xb7 %d/4 Pr\xc3\xbc" "fungen", " \xc2\xb7 %d/4 trials"), run.milestone);
  }
  c.text(font_small, cx + 12, y + 22, sub, th::ink2, cw - 24);
  std::string boss;
  int cap = run.levelCap(&boss);
  int lx = cx + 12, ly = y + 37;
  if (cap) {
    lx += c.text(font_small, lx, ly, "Cap", th::ink2) + 4;
    lx += c.text(font_body_b, lx, ly - 2, std::to_string(cap), th::warn) + 4;
    lx += c.text(font_small, lx, ly, "\xc2\xb7 " + boss, th::ink2);
  }
  std::string where = ctx.area.empty() ? run.lastArea : ctx.area;
  if (!where.empty()) {
    std::string label = areaLabel(g ? g->gen : 6, ctx.loc, where, g_lang);
    if (ctx.loc == 0)  // the stored key is the English name: show the German one where the run knows its id
      for (auto &e : run.encounters)
        if (e.area == where && e.loc) {
          label = areaLabel(g ? g->gen : 6, e.loc, where, g_lang);
          break;
        }
    c.text(font_small, lx + (cap ? 4 : 0), ly, (cap ? "\xc2\xb7 " : "") + label, th::ink3, cx + cw - lx - 20);
  }
  if (run.status != "active") {
    std::string st = run.status == "lost" ? tr("VERLOREN", "LOST") : tr("GEWONNEN", "WON");
    drawChip(c, cx + cw - chipWidth(st) - 8, y + 6, st, run.status == "lost" ? th::badInk : th::warnInk);
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
  const int W = 123, H = 78, GAP = 5;
  for (int i = 0; i < 6; i++) {
    int cx = x + (i % 3) * (W + GAP), cy = y + (i / 3) * (H + 6);
    const RunMon *rm = slots[i];
    const Mon *lm = live[i];
    if (!rm && !lm) {
      c.roundRect(cx, cy, W, H, 10, th::shell2);
      c.roundRectStroke(cx, cy, W, H, 10, 1, th::shell4);
      c.text(font_label, cx, cy + H / 2 - 5, tr("FREI", "EMPTY"), th::edge, W, ALIGN_CENTER);
      continue;
    }
    int species = lm ? lm->species : rm->species;
    int level = lm ? lm->level : rm->level;
    int hp = lm ? lm->hp : rm->hp, maxHp = lm ? lm->maxHp : rm->maxHp;
    bool dead = rm && rm->dead;
    bool egg = lm && lm->isEgg;
    std::string name = rm ? monLabel(*rm) : (lm->nickname.empty() ? speciesName(species, g_lang) : lm->nickname);
    if (egg) name = tr("Ei", "Egg");
    Color acc = dead ? th::cir : th::tri;
    c.card(cx, cy, W, H, 10, dead ? rgb(0x2a2226) : th::panel, dead ? rgb(0x4a3236) : th::edge);
    c.roundRect(cx + 1, cy + 1, W - 2, 3, 1, acc.alpha(dead ? 200 : 110));
    drawMonTile(c, cx + 7, cy + 9, 30, species, dead, rm ? rm->shiny : (lm && lm->shiny));
    c.text(font_body_b, cx + 43, cy + 7, name, dead ? th::ink2 : th::ink, W - 48);
    int lx = cx + 43;
    lx += c.textSpaced(font_label, lx, cy + 25, fmt("LV %d", level), th::ink2, 1) + 5;
    if (dead) drawChip(c, lx, cy + 22, tr("TOT", "DEAD"), th::badInk, rgb(0x3a2426), rgb(0x2a1a1c));
    else if (lm && lm->status) {
      const char *st = lm->status == 0x40 ? tr("PAR", "PAR") : lm->status <= 7 ? tr("SCH", "SLP") : lm->status == 0x20 ? tr("GEF", "FRZ") : lm->status == 0x10 ? tr("BRT", "BRN") : tr("GIF", "PSN");
      drawChip(c, lx, cy + 22, st, rgb(0xe6c0de), rgb(0x3a2a38), rgb(0x2a1e28));
    }
    drawHpBar(c, cx + 8, cy + 46, W - 16, 5, dead ? 0 : hp, maxHp);
    std::string hpText = egg ? "" : fmt("%d/%d", dead ? 0 : hp, maxHp);
    std::string area = rm ? areaLabel(run.game == "sm" || run.game == "usum" ? 7 : 6, rm->loc, rm->area, g_lang) : "";
    c.text(font_small, cx + 8, cy + 56, area, th::ink3, W - 60);
    c.text(font_small, cx + W - 58, cy + 56, hpText, th::ink2, 50, ALIGN_RIGHT);
  }
}

// ---- RunView -------------------------------------------------------------------------------------------------------

enum HitId { H_TAB = 1, H_ROW, H_BACK, H_START, H_RESUME, H_SHEET_A, H_SHEET_B, H_SHEET_C, H_CAP_MINUS, H_CAP_PLUS, H_DELETE, H_OVERLAYS };

void RunView::drawTop(Canvas &c, const ViewCtx &ctx) {
  c.clear(th::shell);
  drawRunHeader(c, 10, 8, 380, ctx);
  drawTeamGrid(c, 10, 70, *ctx.run, ctx.party);
}

void RunView::listTeam(Canvas &c, const ViewCtx &ctx, Rect a) {
  Run &run = *ctx.run;
  rows_.clear();
  // party first (slot order), then the box – alive only (the dead are in the graveyard)
  std::vector<int> idx;
  for (int s = 0; s < 6; s++)
    for (size_t i = 0; i < run.mons.size(); i++)
      if (!run.mons[i].dead && run.mons[i].inParty && run.mons[i].partySlot == s) idx.push_back((int)i);
  for (size_t i = 0; i < run.mons.size(); i++)
    if (!run.mons[i].dead && !run.mons[i].inParty) idx.push_back((int)i);
  const int RH = 34;
  content_[0] = (int)idx.size() * RH + 4;
  if (idx.empty()) {
    c.textWrapped(font_body, a.x + 14, a.y + 30, a.w - 28,
                  tr("Noch kein Pok\xc3\xa9mon im Lauf. Sobald das Spiel l\xc3\xa4uft, \xc3\xbc" "bernimmt souldew dein Team.",
                     "No Pok\xc3\xa9mon in the run yet. Once the game runs, souldew picks up your team."),
                  th::ink3);
    return;
  }
  int y = a.y + 2 - scroll_[0];
  for (size_t k = 0; k < idx.size(); k++, y += RH) {
    const RunMon &m = run.mons[idx[k]];
    rows_.push_back(idx[k]);
    if (y + RH < a.y || y > a.y + a.h) continue;
    bool foc = (int)k == focus_;
    c.card(a.x + 8, y, a.w - 18, RH - 3, 8, foc ? th::shell4 : th::shell2, foc ? th::edgeHi : th::shell4);
    drawMonTile(c, a.x + 13, y + 3, 25, m.species, false, m.shiny);
    c.text(font_body_b, a.x + 44, y + 2, monLabel(m), th::ink, 130);
    std::string sub = fmt("Lv %d \xc2\xb7 ", m.level) + areaLabel(ctx.game ? ctx.game->gen : 6, m.loc, m.area, g_lang);
    c.text(font_small, a.x + 44, y + 17, sub, th::ink3, 150);
    std::string where = m.inParty ? tr("TEAM", "PARTY") : tr("BOX", "BOX");
    drawChip(c, a.x + a.w - 16 - chipWidth(where), y + 9, where, m.inParty ? th::okInk : th::ink2);
    hits_.push_back({{a.x + 8, std::max(y, a.y), a.w - 18, RH - 3}, H_ROW, (int)k});
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
  const int RH = 34;
  content_[1] = (int)idx.size() * RH + 4;
  if (idx.empty()) {
    c.textWrapped(font_body, a.x + 14, a.y + 30, a.w - 28,
                  tr("Noch keine Begegnung. Betritt ein neues Gebiet \xe2\x80\x93 die erste Begegnung dort z\xc3\xa4hlt.",
                     "No encounter yet. Enter a new area \xe2\x80\x93 the first encounter there counts."),
                  th::ink3);
    return;
  }
  int gen = ctx.game ? ctx.game->gen : 6;
  int y = a.y + 2 - scroll_[1];
  for (size_t k = 0; k < idx.size(); k++, y += RH) {
    const Encounter &e = run.encounters[idx[k]];
    rows_.push_back(idx[k]);
    if (y + RH < a.y || y > a.y + a.h) continue;
    bool foc = (int)k == focus_;
    c.card(a.x + 8, y, a.w - 18, RH - 3, 8, foc ? th::shell4 : th::shell2, foc ? th::edgeHi : th::shell4);
    std::string place = e.starter ? tr("Starter", "Starter") : areaLabel(gen, e.loc, e.area, g_lang);
    c.text(font_body_b, a.x + 16, y + 2, place, th::ink, 150);
    const RunMon *m = e.monKey.empty() ? nullptr : run.mon(e.monKey);
    std::string sub = std::string(speciesName(e.species, g_lang)) + (e.level ? fmt(" \xc2\xb7 Lv %d", e.level) : "");
    if (m && m->dead) sub += tr(" \xc2\xb7 tot", " \xc2\xb7 dead");
    c.text(font_small, a.x + 16, y + 17, sub, th::ink3, 160);
    Color fg = (e.status == "caught" || e.status == "gift") ? th::okInk : e.status == "missed" ? th::badInk : e.status == "open" ? th::warnInk : th::ink3;
    std::string st = statusLabel(e.status);
    drawChip(c, a.x + a.w - 16 - chipWidth(st), y + 9, st, fg);
    hits_.push_back({{a.x + 8, std::max(y, a.y), a.w - 18, RH - 3}, H_ROW, (int)k});
  }
}

void RunView::listGrave(Canvas &c, const ViewCtx &ctx, Rect a) {
  Run &run = *ctx.run;
  rows_.clear();
  std::vector<int> idx;
  for (size_t i = 0; i < run.mons.size(); i++)
    if (run.mons[i].dead) idx.push_back((int)i);
  std::sort(idx.begin(), idx.end(), [&](int l, int r) { return run.mons[l].deathTs > run.mons[r].deathTs; });
  const int RH = 44;
  content_[2] = (int)idx.size() * RH + 4;
  if (idx.empty()) {
    c.text(font_head, a.x, a.y + 40, tr("Noch niemand gefallen", "Nobody has fallen yet"), th::okInk, a.w, ALIGN_CENTER);
    c.text(font_small, a.x, a.y + 62, tr("Bleib vorsichtig.", "Stay careful."), th::ink3, a.w, ALIGN_CENTER);
    return;
  }
  int y = a.y + 2 - scroll_[2];
  for (size_t k = 0; k < idx.size(); k++, y += RH) {
    const RunMon &m = run.mons[idx[k]];
    rows_.push_back(idx[k]);
    if (y + RH < a.y || y > a.y + a.h) continue;
    bool foc = (int)k == focus_;
    c.card(a.x + 8, y, a.w - 18, RH - 4, 9, foc ? rgb(0x33282b) : rgb(0x2a2226), foc ? rgb(0x6a4044) : rgb(0x4a3236), th::cir, 3);
    drawMonTile(c, a.x + 15, y + 5, 30, m.species, true);
    std::string head = monLabel(m) + "  ";
    int hx = a.x + 52;
    hx += c.text(font_body_b, hx, y + 3, monLabel(m), th::ink, 110) + 6;
    c.text(font_small, hx, y + 4, fmt("%s \xc2\xb7 Lv %d", speciesName(m.species, g_lang), m.level), th::ink3, a.x + a.w - hx - 16);
    c.text(font_small, a.x + 52, y + 18, m.cause.empty() ? tr("gefallen", "fainted") : m.cause, th::badInk, a.w - 70);
    std::string caught = fmt(tr("Gefangen: %s", "Caught: %s"), areaLabel(ctx.game ? ctx.game->gen : 6, m.loc, m.area, g_lang).c_str());
    c.text(font_small, a.x + 52, y + 29, caught, th::ink3, a.w - 70);
    hits_.push_back({{a.x + 8, std::max(y, a.y), a.w - 18, RH - 4}, H_ROW, (int)k});
  }
}

static std::string ruleValue(const std::string &v) {
  if (v == "line") return tr("Entwicklungslinie", "evolution line");
  if (v == "species") return tr("Art", "species");
  if (v == "off") return tr("aus", "off");
  if (v == "warn") return tr("warnen", "warn");
  if (v == "strict") return tr("streng", "strict");
  if (v == "party") return tr("beendet den Lauf", "ends the run");
  if (v == "area") return tr("z\xc3\xa4hlen als Begegnung", "count as the encounter");
  if (v == "free") return tr("frei", "free");
  return v;
}

void RunView::listRules(Canvas &c, const ViewCtx &ctx, Rect a) {
  Run &run = *ctx.run;
  rows_.clear();
  int y = a.y + 4 - scroll_[3];
  // level cap card
  std::string boss;
  int cap = run.levelCap(&boss);
  c.card(a.x + 8, y, a.w - 18, 56, 10, th::shell2, th::shell4);
  c.text(font_small, a.x + 18, y + 5, tr("Level-Cap", "Level cap"), th::ink2);
  if (cap) {
    c.text(font_num, a.x + 18, y + 22, std::to_string(cap), th::ink);
    c.text(font_body, a.x + 60, y + 27, boss, th::ink2, 120);
  } else c.text(font_body, a.x + 18, y + 26, tr("keine (Regel aus)", "none (rule off)"), th::ink3);
  if (ctx.game && !ctx.game->caps.fromBadges) {
    // Gen 7: the player steps the grand trials
    Rect m{a.x + a.w - 98, y + 16, 34, 26}, p{a.x + a.w - 52, y + 16, 34, 26};
    drawButton(c, m, "", BtnStyle::Ghost, false, "minus");
    drawButton(c, p, "", BtnStyle::Ghost, false, "plus");
    c.text(font_label, a.x + a.w - 106, y + 5, tr("PR\xc3\x9c" "FUNGEN", "TRIALS"), th::ink3, 96, ALIGN_CENTER);
    hits_.push_back({m, H_CAP_MINUS, 0});
    hits_.push_back({p, H_CAP_PLUS, 0});
  } else if (ctx.game) {
    c.text(font_small, a.x + a.w - 130, y + 27, fmt(tr("%d Orden (live)", "%d badges (live)"), run.badges), th::ink3, 110, ALIGN_RIGHT);
  }
  y += 62;
  struct Row {
    const char *label;
    std::string value;
  } rows[] = {
      {tr("Modus", "Mode"), run.rules.presetName() == "hardcore" ? "Nuzlocke \xc2\xb7 Hardcore" : run.rules.presetName() == "classic" ? tr("Nuzlocke \xc2\xb7 Klassisch", "Nuzlocke \xc2\xb7 classic") : tr("Nuzlocke \xc2\xb7 eigene Regeln", "Nuzlocke \xc2\xb7 custom")},
      {tr("Erste Begegnung", "First encounter"), tr("pro Gebiet", "per area")},
      {tr("Dupes-Klausel", "Dupes clause"), ruleValue(run.rules.dupes)},
      {tr("Level-Caps", "Level caps"), ruleValue(run.rules.levelCaps)},
      {tr("Blackout", "Whiteout"), ruleValue(run.rules.blackout)},
      {tr("Geschenke", "Gifts"), ruleValue(run.rules.gifts)},
      {tr("Shiny-Klausel", "Shiny clause"), run.rules.shinyClause ? tr("an", "on") : tr("aus", "off")},
      {"Deathless", run.rules.deathless ? tr("an", "on") : tr("aus", "off")},
      {tr("Keine Items im Kampf", "No items in battle"), run.rules.noBattleItems ? tr("an", "on") : tr("aus", "off")},
      {tr("Set-Modus", "Set mode"), run.rules.setMode ? tr("an", "on") : tr("aus", "off")},
  };
  for (auto &r : rows) {
    if (y + 20 > a.y && y < a.y + a.h) {
      c.text(font_body, a.x + 16, y + 2, r.label, th::ink2, 140);
      c.text(font_body_b, a.x + 150, y + 2, r.value, th::ink, a.w - 170, ALIGN_RIGHT);
      c.hline(a.x + 14, y + 21, a.w - 30, th::shell3);
    }
    y += 23;
  }
  y += 6;
  Rect ov{a.x + 8, y, a.w - 18, 30};
  drawButton(c, ov, ctx.overlays ? tr("Overlays: an", "Overlays: on") : tr("Overlays: aus", "Overlays: off"), BtnStyle::Ghost);
  hits_.push_back({ov, H_OVERLAYS, 0});
  y += 36;
  if (!ctx.inGame) {
    Rect del{a.x + 8, y, a.w - 18, 30};
    drawButton(c, del, tr("Lauf l\xc3\xb6schen", "Delete run"), BtnStyle::Danger);
    hits_.push_back({del, H_DELETE, 0});
    y += 36;
  }
  content_[3] = y + scroll_[3] - a.y;
}

void RunView::drawSheet(Canvas &c, const ViewCtx &ctx) {
  if (!sheet_) return;
  Run &run = *ctx.run;
  c.fill(0, 0, c.width(), c.height(), Color(8, 8, 10, 170));
  int x = 10, w = c.width() - 20, h = 116, y = c.height() - h - 8;
  c.card(x, y, w, h, 12, th::shell2, th::edgeHi);
  std::string title, sub, a, b;
  BtnStyle sa = BtnStyle::Primary, sb = BtnStyle::Ghost;
  if (sheet_ == 1 && sheetArg_ >= 0 && sheetArg_ < (int)run.mons.size()) {
    const RunMon &m = run.mons[sheetArg_];
    title = monLabel(m);
    sub = fmt("%s \xc2\xb7 Lv %d", speciesName(m.species, g_lang), m.level);
    a = m.dead ? tr("Doch nicht tot", "Not dead after all") : tr("Als tot markieren", "Mark as dead");
    sa = m.dead ? BtnStyle::Primary : BtnStyle::Danger;
    b = tr("Abbrechen", "Cancel");
  } else if (sheet_ == 2 && sheetArg_ >= 0 && sheetArg_ < (int)run.encounters.size()) {
    const Encounter &e = run.encounters[sheetArg_];
    title = fmt("%s \xc2\xb7 %s", speciesName(e.species, g_lang), areaLabel(ctx.game ? ctx.game->gen : 6, e.loc, e.area, g_lang).c_str());
    sub = tr("Gefangen (z. B. in die Box) oder verpasst?", "Caught (e.g. into the box) or missed?");
    a = tr("Gefangen", "Caught");
    b = tr("Verpasst", "Missed");
    sb = BtnStyle::Danger;
  } else if (sheet_ == 3) {
    title = tr("Lauf l\xc3\xb6schen?", "Delete run?");
    sub = fmt(tr("\xe2\x80\x9e%s\xe2\x80\x9c wird von der SD-Karte entfernt.", "\xe2\x80\x9c%s\xe2\x80\x9d is removed from the SD card."), run.name.c_str());
    a = tr("L\xc3\xb6schen", "Delete");
    sa = BtnStyle::Danger;
    b = tr("Abbrechen", "Cancel");
  } else {
    sheet_ = 0;
    return;
  }
  c.text(font_head, x + 14, y + 10, title, th::ink, w - 28);
  c.text(font_small, x + 14, y + 32, sub, th::ink2, w - 28);
  Rect ra{x + 12, y + h - 50, (w - 36) / 2, 36}, rb{x + 24 + (w - 36) / 2, y + h - 50, (w - 36) / 2, 36};
  drawButton(c, ra, a, sa, focus_ == 0);
  drawButton(c, rb, b, sb, focus_ == 1);
  hits_.push_back({ra, H_SHEET_A, 0});
  hits_.push_back({rb, H_SHEET_B, 0});
}

void RunView::drawBottom(Canvas &c, const ViewCtx &ctx) {
  hits_.clear();
  c.clear(th::shell);
  std::vector<std::string> tabs = {tr("Team", "Team"), tr("Routen", "Routes"), tr("Friedhof", "Graves"), tr("Regeln", "Rules")};
  Rect area{0, 30, c.width(), 168};
  c.setClip(area);
  if (tab == 0) listTeam(c, ctx, area);
  else if (tab == 1) listRoutes(c, ctx, area);
  else if (tab == 2) listGrave(c, ctx, area);
  else listRules(c, ctx, area);
  c.resetClip();
  drawScrollHint(c, {area.x, area.y + 2, area.w - 1, area.h - 4}, scroll_[tab], content_[tab]);
  // the tab bar and the action bar stay on top
  drawTabBar(c, 0, tabs, tab, &hits_, H_TAB);
  c.fill(0, 199, c.width(), 41, th::shell);
  c.hline(0, 199, c.width(), th::shell3);
  if (ctx.inGame) {
    Rect r{10, 204, 300, 32};
    drawButton(c, r, tr("Zur\xc3\xbc" "ck zum Spiel", "Back to the game"), BtnStyle::Primary, false, "play");
    hits_.push_back({r, H_RESUME, 0});
  } else {
    Rect back{10, 204, 96, 32}, start{114, 204, 196, 32};
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
    if (in.pressed & (K_LEFT | K_RIGHT)) focus_ = 1 - focus_;
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
      return ViewAction::Changed;
    }
    return ViewAction::None;
  }
  if (in.pressed & K_L) tapId = H_TAB, tapArg = (tab + 3) % 4;
  if (in.pressed & K_R) tapId = H_TAB, tapArg = (tab + 1) % 4;
  int rows = (int)rows_.size();
  if (in.pressed & K_DOWN) {
    if (tab == 3) scroll_[3] = std::min(std::max(0, content_[3] - 168), scroll_[3] + 23);
    else if (rows) focus_ = std::min(rows - 1, focus_ + 1);
  }
  if (in.pressed & K_UP) {
    if (tab == 3) scroll_[3] = std::max(0, scroll_[3] - 23);
    else focus_ = std::max(0, focus_ - 1);
  }
  if ((in.pressed & (K_UP | K_DOWN)) && tab != 3) {
    // keep the focused row visible
    int rh = tab == 2 ? 44 : 34;
    int top = focus_ * rh, bottom = top + rh;
    if (top < scroll_[tab]) scroll_[tab] = top;
    if (bottom > scroll_[tab] + 168) scroll_[tab] = bottom - 168;
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
      if (run.milestone > 0) run.milestone--;
      return ViewAction::Changed;
    case H_CAP_PLUS:
      if (ctx.game && run.milestone < ctx.game->caps.count) run.milestone++;
      return ViewAction::Changed;
    case H_DELETE:
      sheet_ = 3;
      focus_ = 1;
      return ViewAction::Changed;
    case H_OVERLAYS: return ViewAction::ToggleOverlays;
  }
  return (in.touchHeld && dragging) ? ViewAction::Changed : ViewAction::None;
}

}  // namespace sd
