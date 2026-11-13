#include "screens.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "i18n.h"

namespace sd {

// ---- shared bits ---------------------------------------------------------------------------------------------------

static const InstalledGame *findInstalled(const std::vector<InstalledGame> &inst, const std::string &key) {
  const InstalledGame *best = nullptr;
  for (auto &i : inst)
    if (i.game && key == i.game->key && (!best || i.cartridge)) best = &i;
  return best;
}

// the hit under the finger when it lifts (started on the same hit); the last drawn hits are on top
static const Hit *tapped(const std::vector<Hit> &hits, const Input &in, int downX, int downY) {
  if (!in.touchUp) return nullptr;
  for (int i = (int)hits.size() - 1; i >= 0; i--)
    if (hits[i].r.contains(in.tx, in.ty) && hits[i].r.contains(downX, downY)) return &hits[i];
  return nullptr;
}

// a top-screen header: the logo badge, a title in the pixel display face, an optional right-hand chip
static void header(Canvas &top, const std::string &title, const std::string &right = "", Color rightCol = th::ink2) {
  drawLogo(top, 12, 11, true);
  top.text(font_display, 56, 10, title, th::ink, 240);
  if (!right.empty()) drawChip(top, 388 - chipWidth(right), 17, right, rightCol);
}

static void keyHints(Canvas &top, int y, std::initializer_list<std::pair<const char *, const char *>> keys) {
  int hx = 14, w;
  for (auto &k : keys) {
    drawKeyHint(top, hx, y, k.first, k.second, &w);
    hx += w + 12;
  }
}

void gameVersionColors(const std::string &g, Color &a, Color &b) {
  if (g == "xy") a = rgb(0x4d7fd6), b = rgb(0xd65446);
  else if (g == "oras") a = rgb(0xc9473c), b = rgb(0x4673c4);
  else if (g == "sm") a = rgb(0xeba13c), b = rgb(0x8c63d4);
  else if (g == "usum") a = rgb(0xec7d3a), b = rgb(0x6a5fd8);
  else a = b = th::tri;
}

void drawCartridge(Canvas &c, int x, int y, int w, int h, const GameInfo *g, bool dim) {
  // a 3DS game card: grey shell with a notch, a label in the two versions' colours
  Color shellTop = dim ? rgb(0x3a3940) : rgb(0x55545c), shellBot = dim ? rgb(0x2c2b31) : rgb(0x3e3d45);
  c.roundRect(x, y, w, h, 4, rgb(0x0e0d11));
  c.roundRectVGradient(x + 1, y + 1, w - 2, h - 2, 3, shellTop, shellBot);
  c.fill(x + w - 7, y + 1, 6, 4, rgb(0x0e0d11));  // the corner notch
  c.hline(x + 4, y + 2, w - 12, Color(255, 255, 255, 40));
  Color a, b;
  gameVersionColors(g ? g->key : "", a, b);
  if (dim) a = mix(a, th::shell, 0.6f), b = mix(b, th::shell, 0.6f);
  int lx = x + 4, ly = y + 7, lw = w - 8, lh = h - 15;
  c.roundRect(lx, ly, lw, lh, 2, rgb(0x0e0d11));
  // label: diagonal split of the two version colours
  for (int yy = 0; yy < lh - 2; yy++)
    for (int xx = 0; xx < lw - 2; xx++) {
      bool first = xx * (lh - 2) + yy * (lw - 2) < (lw - 2) * (lh - 2);
      Color col = first ? a : b;
      col = mix(col, rgb(0xffffff), yy == 0 ? 0.25f : 0.f);
      c.fill(lx + 1 + xx, ly + 1 + yy, 1, 1, mix(col, rgb(0x000000), 0.12f * yy / lh));
    }
  for (int k = 0; k < 4; k++) c.fill(x + 6 + k * 4, y + h - 6, 2, 4, rgb(0x2a2930));  // contacts
}

void drawSceneArt(Canvas &c, Rect r, bool battle) {
  // an abstract night scene in place of game footage (no game graphics in this repository)
  Rect old = c.clip();
  c.roundRect(r.x - 2, r.y - 2, r.w + 4, r.h + 4, 10, rgb(0x050407));
  int x0 = std::max(old.x, r.x), y0 = std::max(old.y, r.y);
  int x1 = std::min(old.x + old.w, r.x + r.w), y1 = std::min(old.y + old.h, r.y + r.h);
  c.setClip({x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)});
  for (int y = 0; y < r.h; y++) c.fill(r.x, r.y + y, r.w, 1, mix(rgb(0x2b3a5c), rgb(0x55707e), (float)y / r.h));
  c.glow(r.x + r.w * 3 / 4, r.y + r.h / 4, r.w / 3, r.h / 3, rgb(0xf2ffdc, 40));
  int gy = r.y + r.h * 62 / 100;
  c.fill(r.x, gy, r.w, r.y + r.h - gy, rgb(0x3d5a45));
  if (battle) {
    c.roundRect(r.x + r.w * 58 / 100, r.y + r.h * 40 / 100, r.w * 30 / 100, r.h * 12 / 100, r.h * 6 / 100, rgb(0x4d7254));
    c.roundRect(r.x + r.w * 66 / 100, r.y + r.h * 18 / 100, r.w * 14 / 100, r.w * 14 / 100, r.w * 4 / 100, rgb(0xb79a5d));
    c.roundRect(r.x + r.w * 8 / 100, r.y + r.h * 72 / 100, r.w * 34 / 100, r.h * 14 / 100, r.h * 7 / 100, rgb(0x4d7254));
    c.roundRect(r.x + r.w * 16 / 100, r.y + r.h * 46 / 100, r.w * 16 / 100, r.w * 16 / 100, r.w * 5 / 100, rgb(0x5874a8));
  } else {
    c.circle(r.x + r.w / 5, gy + r.h / 2, r.h * 55 / 100, rgb(0x456a4c));
    c.circle(r.x + r.w * 4 / 5, gy + r.h * 6 / 10, r.h * 62 / 100, rgb(0x3a5b41));
    c.roundRect(r.x + r.w / 2 - r.w / 16, gy - 2, r.w / 8, r.y + r.h - gy + 2, 3, rgb(0x8c7c5c));
    c.roundRect(r.x + r.w / 2 - 4, gy - 14, 8, 12, 3, rgb(0xc7584a));
    c.circle(r.x + r.w / 2, gy - 16, 4, rgb(0xe8c9a8));
  }
  c.setClip(old);
}

void transition(Canvas &out, const Canvas &from, const Canvas &to, float t, int slide) {
  t = std::max(0.f, std::min(1.f, t));
  float e = 1 - (1 - t) * (1 - t);  // ease out
  int a = (int)(e * 256), dx = (int)std::lround((1 - e) * slide);
  int w = out.width(), h = out.height();
  const uint32_t *pf = from.pixels(), *pt = to.pixels();
  uint32_t *po = out.pixels();
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      int sx = std::max(0, std::min(w - 1, x - dx));
      uint32_t f = pf[y * w + x], n = pt[y * w + sx];
      uint32_t r = (((f >> 16) & 0xff) * (256 - a) + ((n >> 16) & 0xff) * a) >> 8;
      uint32_t g = (((f >> 8) & 0xff) * (256 - a) + ((n >> 8) & 0xff) * a) >> 8;
      uint32_t b = ((f & 0xff) * (256 - a) + (n & 0xff) * a) >> 8;
      po[y * w + x] = 0xff000000u | (r << 16) | (g << 8) | b;
    }
}

// ---- title menu ----------------------------------------------------------------------------------------------------

enum { MH_ITEM = 1 };

static const char *menuIcon(int i) {
  static const char *I[] = {"play", "plus", "list", "trophy", "gear", "help", "power"};
  return I[i];
}
static Color menuAccent(int i) {
  static const Color C[] = {th::tri, th::p1, th::tri, th::warn, th::ink2, th::sqr, th::cir};
  return C[i];
}
static const char *menuLabel(int i) {
  switch (i) {
    case 0: return tr("Weiterspielen", "Continue");
    case 1: return tr("Neuer Lauf", "New run");
    case 2: return tr("L\xc3\xa4ufe", "Runs");
    case 3: return tr("Erfolge", "Trophies");
    case 4: return tr("Einstellungen", "Settings");
    case 5: return tr("Hilfe", "Help");
    default: return tr("Beenden", "Quit");
  }
}

static Run *continueRun(RunStore &store) {
  Run *r = store.activeRun();
  if (!r && !store.runs.empty()) r = &store.runs.back();
  return r;
}

void MainMenu::drawTop(Canvas &top, RunStore &store, const std::vector<InstalledGame> &installed, const std::string &status) {
  drawBackdrop(top, Backdrop::Hero);
  // the handheld's screen with the linked rings
  float k = std::max(0.f, std::min(1.f, intro));
  Rect lcd{110, 16, 180, 92};
  top.shadow(lcd.x - 6, lcd.y - 6, lcd.w + 12, lcd.h + 12, 18, 6, 3, 140);
  top.roundRectVGradient(lcd.x - 6, lcd.y - 6, lcd.w + 12, lcd.h + 12, 18, rgb(0x2e2c38), rgb(0x0e0d12));
  top.roundRectStroke(lcd.x - 6, lcd.y - 6, lcd.w + 12, lcd.h + 12, 18, 1, Color(255, 255, 255, 26));
  top.roundRect(lcd.x - 2, lcd.y - 2, lcd.w + 4, lcd.h + 4, 12, rgb(0x050407));
  top.roundRectVGradient(lcd.x, lcd.y, lcd.w, lcd.h, 10, rgb(0x1f3326), rgb(0x080d0a));
  top.glow(lcd.x + lcd.w / 2, lcd.y + lcd.h * 2 / 5, lcd.w * 2 / 3, lcd.h * 3 / 4, rgb(0x22362a, 200));
  Rect old = top.clip();
  top.setClip(lcd);
  drawLogoMark(top, lcd.x + (lcd.w - 32 * 5) / 2, lcd.y + (lcd.h - 16 * 5) / 2, 5, k, true);
  for (int y = lcd.y + 1; y < lcd.y + lcd.h; y += 2) top.hline(lcd.x, y, lcd.w, Color(0, 0, 0, 38));
  top.fill(lcd.x, lcd.y, lcd.w, 22, Color(255, 255, 255, 6));  // the glass
  top.setClip(old);
  // the wordmark
  float wa = std::max(0.f, std::min(1.f, (k - 0.35f) / 0.65f));
  uint8_t a = (uint8_t)(255 * wa);
  int ww = Canvas::textWidth(font_title, "souldew");
  int wx = 200 - ww / 2, wy = 104 + (int)((1 - wa) * 6);
  top.text(font_title, wx + 2, wy + 3, "souldew", Color(0, 0, 0, (uint8_t)(110 * wa)));
  top.text(font_title, wx, wy, "souldew", th::ink.alpha(a));
  top.text(font_body, 0, 162, tr("Dein Nuzlocke-Begleiter auf dem 3DS", "Your Nuzlocke companion on the 3DS"), th::ink2.alpha(a), 400, ALIGN_CENTER);
  top.textSpaced(font_label, 386 - Canvas::textSpacedWidth(font_label, std::string("V") + kVersion + " \xc2\xb7 3DS", 1), 6,
                 std::string("V") + kVersion + " \xc2\xb7 3DS", th::ink4, 1);
  // a strip about what is focused below
  std::string line;
  const char *ic = menuIcon(focus);
  Color col = menuAccent(focus);
  Run *run = continueRun(store);
  TrophyTotals tt = trophyTotals(store, evaluateTrophies(store));
  switch (focus) {
    case 0:
      if (run) {
        const GameInfo *g = gameByKey(run->game.c_str());
        int cap = run->levelCap();
        line = run->name + " \xc2\xb7 " + (g ? g->shortName : run->game) +
               fmt(tr(" \xc2\xb7 %d gefangen \xc2\xb7 %d tot", " \xc2\xb7 %d caught \xc2\xb7 %d dead"), run->caught(), run->dead()) +
               (cap ? fmt(" \xc2\xb7 Cap %d", cap) : std::string());
      } else line = tr("Noch kein Lauf \xe2\x80\x93 leg deinen ersten an", "No run yet \xe2\x80\x93 create your first one");
      break;
    case 1: {
      if (installed.empty()) line = tr("Kein Spiel gefunden \xe2\x80\x93 Lauf trotzdem m\xc3\xb6glich", "No game found \xe2\x80\x93 you can still create a run");
      else {
        line = tr("Gefunden: ", "Found: ");
        for (size_t i = 0; i < installed.size(); i++) line += (i ? ", " : "") + std::string(installed[i].game->shortName);
      }
      break;
    }
    case 2: {
      int active = 0;
      for (auto &r : store.runs) active += r.status == "active" ? 1 : 0;
      line = fmt(tr("%d L\xc3\xa4ufe \xc2\xb7 %d aktiv", "%d runs \xc2\xb7 %d active"), (int)store.runs.size(), active);
      break;
    }
    case 3: line = fmt(tr("%d von %d freigeschaltet \xc2\xb7 %d Punkte", "%d of %d unlocked \xc2\xb7 %d points"), tt.unlocked, tt.total, tt.points); break;
    case 4:
      line = std::string(tr("Sprache: Deutsch", "Language: English")) + " \xc2\xb7 " +
             (store.settings.overlays ? tr("Overlays an", "overlays on") : tr("Overlays aus", "overlays off"));
      break;
    case 5: line = tr("So funktioniert souldew \xe2\x80\x93 im Spiel und mit den Regeln", "How souldew works \xe2\x80\x93 in game and the rules"); break;
    default: line = tr("Zur\xc3\xbc" "ck zum Homebrew Launcher", "Back to the Homebrew Launcher"); break;
  }
  if (!status.empty()) {
    line = status;
    ic = "warn";
    col = th::warn;
  }
  int lw = std::min(330, Canvas::textWidth(font_small, line)) + 44;
  int lx = 200 - lw / 2;
  top.roundRect(lx, 198, lw, 26, 13, Color(255, 255, 255, (uint8_t)(14 * wa)));
  top.roundRectStroke(lx, 198, lw, 26, 13, 1, Color(255, 255, 255, (uint8_t)(18 * wa)));
  top.circle(lx + 13, 211, 7, mix(th::stage, col, 0.3f).alpha(a));
  drawIcon(top, ic, lx + 13 - iconWidth(ic) / 2, 211, col.alpha(a));
  top.text(font_small, lx + 28, 204, line, th::ink2.alpha(a), lw - 40);
}

void MainMenu::draw(Canvas &top, Canvas &bottom, RunStore &store, const std::vector<InstalledGame> &installed, const std::string &status) {
  hits_.clear();
  drawTop(top, store, installed, status);
  // ---- bottom: continue + the grid
  drawBackdrop(bottom, Backdrop::Plain);
  Run *run = continueRun(store);
  Rect cont{10, 10, 300, 58};
  bool f0 = focus == 0;
  if (f0) bottom.roundRect(cont.x - 3, cont.y - 3, cont.w + 6, cont.h + 6, 15, th::p1.alpha(190));
  if (f0) bottom.roundRect(cont.x - 1, cont.y - 1, cont.w + 2, cont.h + 2, 13, th::stage);
  bottom.shadow(cont.x, cont.y, cont.w, cont.h, 12, 4, 2, 120);
  bottom.roundRect(cont.x, cont.y, cont.w, cont.h, 12, rgb(0x2d4f3e));
  bottom.roundRectVGradient(cont.x + 1, cont.y + 1, cont.w - 2, cont.h - 3, 11, rgb(0x55937a), th::play);
  bottom.hline(cont.x + 12, cont.y + 1, cont.w - 24, Color(255, 255, 255, 50));
  bottom.circle(cont.x + 30, cont.y + 28, 17, rgb(0x2a4a3a));
  bottom.circle(cont.x + 30, cont.y + 28, 16, rgb(0xf4f1ea).alpha(235));
  if (run) drawIcon(bottom, "play", cont.x + 27, cont.y + 28, th::play);
  else drawIcon(bottom, "plus", cont.x + 26, cont.y + 28, th::play);
  bottom.text(font_head, cont.x + 56, cont.y + 10, run ? tr("Weiterspielen", "Continue") : tr("Ersten Lauf anlegen", "Create your first run"), rgb(0xf6f3ec), 210);
  std::string sub;
  if (run) {
    const GameInfo *g = gameByKey(run->game.c_str());
    sub = run->name + " \xc2\xb7 " + (g ? g->shortName : run->game);
    const InstalledGame *ig = findInstalled(installed, run->game);
    if (!ig) sub += tr(" \xc2\xb7 Spiel fehlt", " \xc2\xb7 game missing");
  } else sub = tr("Spiel w\xc3\xa4hlen, Regeln, los", "pick the game, the rules, go");
  bottom.text(font_small, cont.x + 56, cont.y + 31, sub, rgb(0xcfe6da), 210);
  drawIcon(bottom, "chevron", cont.x + cont.w - 18, cont.y + 29, rgb(0xdcefe4));
  hits_.push_back({cont, MH_ITEM, 0});
  // the tiles
  TrophyTotals tt = trophyTotals(store, evaluateTrophies(store));
  for (int i = 1; i <= 6; i++) {
    int col = (i - 1) % 2, row = (i - 1) / 2;
    Rect r{10 + col * 154, 78 + row * 44, 146, 38};
    bool f = focus == i;
    drawCard(bottom, r, 10, f);
    Color acc = menuAccent(i);
    bottom.circle(r.x + 19, r.y + 19, 12, mix(th::well, acc, 0.20f));
    bottom.roundRectStroke(r.x + 7, r.y + 7, 24, 24, 12, 1, mix(th::well, acc, 0.45f));
    const char *ic = menuIcon(i);
    drawIcon(bottom, ic, r.x + 19 - iconWidth(ic) / 2, r.y + 19, mix(acc, th::ink, 0.2f));
    std::string val;
    if (i == 2 && !store.runs.empty()) val = std::to_string(store.runs.size());
    if (i == 3) val = fmt("%d/%d", tt.unlocked, tt.total);
    int vw = val.empty() ? 0 : Canvas::textSpacedWidth(font_label, val, 1);
    bottom.text(font_body_b, r.x + 38, r.y + 11, menuLabel(i), th::ink, r.w - 46 - (vw ? vw + 6 : 0));
    if (!val.empty()) bottom.textSpaced(font_label, r.x + r.w - 10 - vw, r.y + 14, val, th::ink3, 1);
    hits_.push_back({r, MH_ITEM, i});
  }
  // footer
  int fx = 0, w;
  Canvas &b = bottom;
  int total = 0;
  {
    int w1 = 0, w2 = 0;
    // measure: key + label widths (drawKeyHint draws; draw once off-screen-free by computing)
    w1 = 13 + 4 + Canvas::textWidth(font_small, tr("w\xc3\xa4hlen", "select"));
    w2 = Canvas::textSpacedWidth(font_label, "START", 1) + 10 + 4 + Canvas::textWidth(font_small, tr("beenden", "quit"));
    total = w1 + 14 + w2;
  }
  fx = 160 - total / 2;
  drawKeyHint(b, fx, 218, "A", tr("w\xc3\xa4hlen", "select"), &w);
  drawKeyHint(b, fx + w + 14, 218, "START", tr("beenden", "quit"), &w);
}

MenuAction MainMenu::handle(const Input &in, RunStore &store) {
  if (in.touchDown) downX_ = in.tx, downY_ = in.ty;
  int sel = -1;
  if (const Hit *h = tapped(hits_, in, downX_, downY_)) sel = h->arg;
  int old = focus;
  if (in.pressed & K_DOWN) focus = focus == 0 ? 1 : std::min(6, focus + 2);
  if (in.pressed & K_UP) focus = focus <= 2 ? 0 : focus - 2;
  if ((in.pressed & K_RIGHT) && focus >= 1 && focus % 2 == 1) focus++;
  if ((in.pressed & K_LEFT) && focus >= 1 && focus % 2 == 0) focus--;
  if (in.pressed & K_A) sel = focus;
  if (in.pressed & K_START) return MenuAction::Quit;
  if (in.pressed & K_Y) sel = 1;
  if (sel >= 0) {
    focus = sel;
    switch (sel) {
      case 0: return continueRun(store) ? MenuAction::Continue : MenuAction::NewRun;
      case 1: return MenuAction::NewRun;
      case 2: return MenuAction::Runs;
      case 3: return MenuAction::Trophies;
      case 4: return MenuAction::Settings;
      case 5: return MenuAction::Help;
      default: return MenuAction::Quit;
    }
  }
  return focus != old ? MenuAction::Changed : MenuAction::None;
}

// ---- onboarding ----------------------------------------------------------------------------------------------------

enum { OH_NEXT = 1, OH_SKIP, OH_LANG, OH_BACK };

void Onboarding::start(bool again) {
  fromMenu = again;
  page = again ? 1 : 0;
  focus = 1;
}

static void ruleTile(Canvas &c, int x, int y, int w, const char *icon, Color col, const std::string &title, const std::string &sub) {
  drawSurface(c, {x, y, w, 78}, 10, rgb(0x2b2a31), rgb(0x222128), th::edge, 3);
  c.circle(x + w / 2, y + 22, 14, mix(th::well, col, 0.22f));
  c.roundRectStroke(x + w / 2 - 14, y + 8, 28, 28, 14, 1, mix(th::well, col, 0.5f));
  drawIcon(c, icon, x + w / 2 - iconWidth(icon) / 2, y + 22, mix(col, th::ink, 0.15f));
  c.text(font_body_b, x + 4, y + 42, title, th::ink, w - 8, ALIGN_CENTER);
  c.text(font_small, x + 4, y + 58, sub, th::ink3, w - 8, ALIGN_CENTER);
}

void Onboarding::draw(Canvas &top, Canvas &bottom, const RunStore &store) {
  hits_.clear();
  drawBackdrop(top, Backdrop::Hero);
  drawBackdrop(bottom, Backdrop::Plain);
  std::string kicker = fmt(tr("EINF\xc3\x9cHRUNG \xc2\xb7 %d/%d", "INTRODUCTION \xc2\xb7 %d/%d"), page + 1, kPages);
  top.textSpaced(font_label, 14, 12, kicker, th::ink3, 1);
  std::string title, body;
  switch (page) {
    case 0: {
      title = "Willkommen \xc2\xb7 Welcome";
      body = tr("Ein kostenloser Begleiter f\xc3\xbcr Nuzlockes \xe2\x80\x93 direkt auf dem 3DS, w\xc3\xa4hrend du spielst. W\xc3\xa4hle zuerst deine Sprache.",
                "A free companion for Nuzlockes \xe2\x80\x93 right on the 3DS, while you play. First, pick your language.");
      // the logo, large
      top.glow(200, 112, 150, 70, rgb(0x22362a, 150));
      drawLogoMark(top, 200 - 64, 70, 4, 1.f, true);
      int ww = Canvas::textWidth(font_title, "souldew");
      top.text(font_title, 200 - ww / 2, 134, "souldew", th::ink);
      break;
    }
    case 1: {
      title = tr("Was souldew macht", "What souldew does");
      body = tr("souldew merkt sich Begegnungen, F\xc3\xa4nge, Tode und Level-Caps. Im Spiel bleibt alles wie gewohnt \xe2\x80\x93 nur in den wichtigen Momenten erscheint oben eine kleine Karte.",
                "souldew keeps track of encounters, catches, deaths and level caps. The game looks as usual \xe2\x80\x93 only in the important moments a small card appears at the top.");
      Rect art{24, 60, 352, 168};
      drawSceneArt(top, art, true);
      Event e{EventKind::FirstEncounter, Tone::Ok, fmt(tr("Erste Begegnung: %s", "First encounter: %s"), (std::string(speciesName(278, g_lang)) + " Lv 13").c_str()),
              tr("Route 8 \xc2\xb7 das ist dein Fang", "Route 8 \xc2\xb7 this is your catch"), 278};
      drawToast(top, 32, 68, 290, e, 1.f);
      Event d{EventKind::Death, Tone::Danger, fmt(tr("%s ist gefallen", "%s has fallen"), "Leo"), tr("gegen Tyrunt Lv 25 \xc2\xb7 in die Box", "vs Tyrunt Lv 25 \xc2\xb7 box it"), 667};
      drawToast(top, 78, 172, 290, d, 1.f);
      break;
    }
    case 2: {
      title = tr("Im Spiel \xc3\xb6" "ffnen", "Open it in game");
      body = tr("Starte das Spiel \xc3\xbc" "ber souldew (Lauf \xe2\x80\xba Spiel starten). Im Spiel \xc3\xb6" "ffnet SELECT souldew \xc3\xbc" "ber dem pausierten Spiel \xe2\x80\x93 oder halte die kleine Lasche unten rechts.",
                "Start the game through souldew (run \xe2\x80\xba Start game). In game, SELECT opens souldew over the paused game \xe2\x80\x93 or hold the small tab in the bottom-right corner.");
      // a 3DS: the top screen with a card, the bottom screen with the tab, SELECT lit
      int cx = 200, oy = 58;
      top.shadow(cx - 118, oy, 236, 78, 12, 4, 2, 110);
      top.roundRectVGradient(cx - 118, oy, 236, 78, 12, rgb(0x3a3944), rgb(0x24232b));
      top.roundRectStroke(cx - 118, oy, 236, 78, 12, 1, Color(255, 255, 255, 24));
      drawSceneArt(top, {cx - 96, oy + 8, 192, 62}, false);
      Event e{EventKind::NewArea, Tone::Info, "Route 8", tr("Neues Gebiet", "New area")};
      drawToast(top, cx - 90, oy + 12, 134, e, 1.f);
      int by = oy + 82;
      top.shadow(cx - 112, by, 224, 88, 12, 4, 2, 110);
      top.roundRectVGradient(cx - 112, by, 224, 88, 12, rgb(0x34333d), rgb(0x1f1e25));
      top.roundRectStroke(cx - 112, by, 224, 88, 12, 1, Color(255, 255, 255, 20));
      top.roundRect(cx - 60, by + 6, 120, 66, 4, rgb(0x050407));
      top.roundRectVGradient(cx - 57, by + 9, 114, 60, 3, rgb(0x2d4c78), rgb(0x1f3756));
      drawCornerTab(top, cx + 57 - 33, by + 69 - 21, true);
      top.glow(cx + 57 - 18, by + 69 - 12, 26, 18, th::p1.alpha(60));
      // d-pad + buttons + SELECT
      top.fill(cx - 96, by + 30, 18, 6, rgb(0x121116));
      top.fill(cx - 90, by + 24, 6, 18, rgb(0x121116));
      for (int k = 0; k < 4; k++) {
        static const int dx[4] = {0, 9, -9, 0}, dy[4] = {-9, 0, 0, 9};
        top.circle(cx + 86 + dx[k], by + 33 + dy[k], 4, rgb(0x121116));
      }
      top.roundRect(cx - 28, by + 77, 22, 6, 3, rgb(0x121116));
      top.roundRect(cx + 6, by + 77, 22, 6, 3, th::p1);
      top.glow(cx + 17, by + 80, 22, 10, th::p1.alpha(90));
      top.textSpaced(font_label, cx + 34, by + 74, "SELECT", th::crsInk, 1);
      break;
    }
    case 3: {
      title = tr("Die Nuzlocke-Regeln", "The Nuzlocke rules");
      body = tr("Pro Gebiet z\xc3\xa4hlt nur die erste Begegnung. Wer f\xc3\xa4llt, ist tot und kommt in die Box. Level-Caps und Dupes-Klausel w\xc3\xa4hlst du beim Anlegen \xe2\x80\x93 souldew z\xc3\xa4hlt mit.",
                "Only the first encounter per area counts. Whoever falls is dead and goes into the box. Level caps and the dupes clause are chosen when you create the run \xe2\x80\x93 souldew keeps count.");
      int w = 90, gap = 6, x = 200 - (4 * w + 3 * gap) / 2;
      ruleTile(top, x, 82, w, "pin", th::tri, tr("1 pro Gebiet", "1 per area"), tr("die erste z\xc3\xa4hlt", "the first counts"));
      ruleTile(top, x + (w + gap), 82, w, "cross", th::cir, tr("Gefallen", "Fallen"), tr("= tot", "= dead"));
      ruleTile(top, x + 2 * (w + gap), 82, w, "star", th::warn, "Level-Caps", tr("Ass des Leiters", "the leader's ace"));
      ruleTile(top, x + 3 * (w + gap), 82, w, "ball", th::p1, tr("Dupes", "Dupes"), tr("schon gefangen", "already caught"));
      top.text(font_small, 0, 180, tr("Shiny-Klausel, Items, Set-Modus: Ehrensache.", "Shiny clause, items, set mode: the honour system."), th::ink3, 400, ALIGN_CENTER);
      break;
    }
    default: {
      title = tr("Bereit", "Ready");
      body = tr("Leg deinen ersten Lauf an und starte das Spiel. Erfolge schaltest du nebenbei frei \xe2\x80\x93 du findest sie im Men\xc3\xbc.",
                "Create your first run and start the game. Achievements unlock along the way \xe2\x80\x93 you find them in the menu.");
      static const int tiers[5] = {0, 1, 2, 3, 4};
      for (int i = 0; i < 5; i++) {
        int mx = 200 - 5 * 34 + i * 68 + 10;
        top.glow(mx + 24, 120, 30, 30, rgb(0xfff1ad, 18));
        drawMedal(top, mx, 82, i, tiers[i], 2);
      }
      top.text(font_small, 0, 158, tr("Bronze, Silber, Gold, Platin, Meister", "Bronze, silver, gold, platinum, master"), th::ink3, 400, ALIGN_CENTER);
      break;
    }
  }
  if (page != 0) top.text(font_display, 14, 22, title, th::ink, 372);
  else top.text(font_display, 0, 26, title, th::ink, 400, ALIGN_CENTER);

  // ---- bottom: text, options, dots, buttons
  if (page == 0) {
    bottom.textWrapped(font_body, 16, 12, 288, body, th::ink2, 2, 3);
    for (int i = 0; i < 2; i++) {
      Rect r{16 + i * 148, 76, 140, 76};
      bool sel = (i == 0) == (store.settings.lang != "en");
      bool f = focus == i;
      drawCard(bottom, r, 12, f);
      if (sel) bottom.roundRectStroke(r.x, r.y, r.w, r.h, 12, 1, th::tri);
      bottom.text(font_display, r.x, r.y + 12, i == 0 ? "DE" : "EN", sel ? th::okInk : th::ink2, r.w, ALIGN_CENTER);
      bottom.text(font_body_b, r.x, r.y + 44, i == 0 ? "Deutsch" : "English", th::ink, r.w, ALIGN_CENTER);
      if (sel) {
        bottom.circle(r.x + r.w - 14, r.y + 14, 7, th::tri);
        drawIcon(bottom, "check", r.x + r.w - 19, r.y + 14, rgb(0x14251d));
      }
      hits_.push_back({r, OH_LANG, i});
    }
  } else {
    // the text, centred in the space above the page dots
    int lines = 0;
    {
      Canvas probe(1, 1);
      lines = probe.textWrapped(font_body, -1000, -1000, 288, body, th::ink2, 3, 8);
    }
    bottom.textWrapped(font_body, 16, std::max(10, (168 - lines) / 2), 288, body, th::ink2, 3, 8);
  }
  drawPageDots(bottom, 160, 176, kPages, page);
  drawActionBar(bottom, 196);
  bool last = page == kPages - 1;
  Rect skip{10, 203, 110, 30}, next{128, 203, 182, 30};
  const char *skipLabel = page > 0 && fromMenu ? tr("Zur\xc3\xbc" "ck", "Back") : last ? tr("Zum Men\xc3\xbc", "To the menu") : tr("\xc3\x9c" "berspringen", "Skip");
  if (page > 0 && fromMenu && last) skipLabel = tr("Zur\xc3\xbc" "ck", "Back");
  drawButton(bottom, skip, skipLabel, BtnStyle::Ghost, focus == 2 || (page == 0 && false), page > 0 && fromMenu ? "back" : nullptr);
  drawButton(bottom, next, last ? (fromMenu ? tr("Fertig", "Done") : tr("Neuer Lauf", "New run")) : tr("Weiter", "Next"),
             BtnStyle::Primary, focus == 3, last ? (fromMenu ? "check" : "plus") : "chevron");
  hits_.push_back({skip, OH_SKIP, 0});
  hits_.push_back({next, OH_NEXT, 0});
}

IntroAction Onboarding::handle(const Input &in, RunStore &store) {
  if (in.touchDown) downX_ = in.tx, downY_ = in.ty;
  int id = 0, arg = 0;
  if (const Hit *h = tapped(hits_, in, downX_, downY_)) id = h->id, arg = h->arg;
  bool last = page == kPages - 1;
  // D-pad: page 0 – left/right picks the language; elsewhere left/right moves between the two buttons
  if (page == 0 && (in.pressed & (K_LEFT | K_RIGHT))) {
    id = OH_LANG;
    arg = (in.pressed & K_LEFT) ? 0 : 1;
  } else if (in.pressed & K_LEFT) {
    focus = 2;
    return IntroAction::Changed;
  } else if (in.pressed & K_RIGHT) {
    focus = 3;
    return IntroAction::Changed;
  }
  if (in.pressed & K_A) id = (focus == 2 && page > 0) ? OH_SKIP : OH_NEXT;
  if (in.pressed & K_B) id = page > 0 ? OH_BACK : 0;
  if (in.pressed & K_X) id = OH_SKIP;
  switch (id) {
    case OH_LANG: {
      std::string want = arg == 0 ? "de" : "en";
      focus = arg;
      if (store.settings.lang != want) {
        store.settings.lang = want;
        g_lang = langOf(store.settings);
        return IntroAction::Language;
      }
      return IntroAction::Changed;
    }
    case OH_BACK:
      if (page > (fromMenu ? 1 : 0)) {
        page--;
        focus = 3;
        return IntroAction::Changed;
      }
      return fromMenu ? IntroAction::Done : IntroAction::None;
    case OH_SKIP:
      if (fromMenu && page > 0 && !(in.pressed & K_X)) {
        // "Back" when opened from the menu
        if (page > 1) {
          page--;
          focus = 3;
          return IntroAction::Changed;
        }
        return IntroAction::Done;
      }
      store.settings.onboarded = true;
      return IntroAction::Done;
    case OH_NEXT:
      if (last) {
        store.settings.onboarded = true;
        return fromMenu ? IntroAction::Done : IntroAction::NewRun;
      }
      page++;
      focus = 3;
      return IntroAction::Changed;
  }
  return IntroAction::None;
}

// ---- runs list -----------------------------------------------------------------------------------------------------

enum { HH_RUN = 1, HH_BACK, HH_NEW };

std::string HomeScreen::selectedId(const RunStore &store) const {
  if (selected >= 0 && selected < (int)store.runs.size()) return store.runs[store.runs.size() - 1 - selected].id;
  return "";
}

void HomeScreen::draw(Canvas &top, Canvas &bottom, RunStore &store, const std::vector<InstalledGame> &installed,
                      const std::string &status) {
  hits_.clear();
  drawBackdrop(top, Backdrop::Plain);
  {
    std::string right = installed.empty() ? tr("KEIN SPIEL GEFUNDEN", "NO GAME FOUND")
                                          : fmt(tr("%d SPIEL%s GEFUNDEN", "%d GAME%s FOUND"), (int)installed.size(),
                                                installed.size() == 1 ? "" : tr("E", "S"));
    header(top, tr("Deine L\xc3\xa4ufe", "Your runs"), right, installed.empty() ? th::warnInk : th::ink2);
  }
  Run *run = nullptr;
  if (!store.runs.empty()) {
    selected = std::max(0, std::min(selected, (int)store.runs.size() - 1));
    run = store.byId(selectedId(store));
  }
  if (!run) {
    drawIcon(top, "list", 200 - 9, 104, th::edgeHi, 2);
    top.text(font_head, 0, 124, tr("Noch kein Lauf", "No run yet"), th::ink, 400, ALIGN_CENTER);
    top.text(font_small, 0, 146, tr("Leg einen an: Spiel w\xc3\xa4hlen, Regeln, los.", "Create one: pick the game, the rules, go."), th::ink3, 400, ALIGN_CENTER);
  } else {
    const GameInfo *g = gameByKey(run->game.c_str());
    Color acc = gameColor(run->game);
    Rect card{12, 44, 376, 166};
    top.shadow(card.x, card.y, card.w, card.h, 12, 5, 3, 120);
    top.roundRect(card.x, card.y, card.w, card.h, 12, mix(th::edge, acc, 0.25f));
    top.roundRectGradient(card.x + 1, card.y + 1, card.w - 2, card.h - 2, 11, mix(rgb(0x2a2931), acc, 0.16f), rgb(0x232229));
    top.hline(card.x + 12, card.y + 1, card.w - 24, Color(255, 255, 255, 22));
    drawCartridge(top, card.x + 14, card.y + 14, 40, 46, g);
    top.text(font_display, card.x + 66, card.y + 10, run->name, th::ink, 230);
    std::string st = run->status == "lost" ? tr("VERLOREN", "LOST") : run->status == "won" ? tr("GEWONNEN", "WON") : tr("AKTIV", "ACTIVE");
    drawChip(top, card.x + card.w - 12 - chipWidth(st), card.y + 14, st, run->status == "lost" ? th::badInk : run->status == "won" ? th::warnInk : th::okInk);
    std::string sub = g ? g->name : run->game;
    const InstalledGame *ig = findInstalled(installed, run->game);
    sub += ig ? (ig->cartridge ? tr(" \xc2\xb7 Karte", " \xc2\xb7 cartridge") : tr(" \xc2\xb7 installiert", " \xc2\xb7 installed")) : tr(" \xc2\xb7 Spiel fehlt", " \xc2\xb7 game missing");
    top.text(font_small, card.x + 66, card.y + 36, sub, th::ink2, 300);
    std::string boss;
    int cap = run->levelCap(&boss);
    int total = g ? g->caps.count : 8;
    drawPips(top, card.x + 66, card.y + 53, run->progress(), total, th::warn);
    std::string capS = cap ? fmt(tr("Cap %d \xc2\xb7 %s", "cap %d \xc2\xb7 %s"), cap, boss.c_str()) : std::string(tr("ohne Level-Caps", "no level caps"));
    top.text(font_small, card.x + 66 + total * 9 + 6, card.y + 49, capS, th::ink3, 200);
    int tw = 66, gap = 6, tx = card.x + 14, ty = card.y + 74;
    drawStatTile(top, tx, ty, tw, 52, run->caught(), tr("gefangen", "caught"), th::ink); tx += tw + gap;
    drawStatTile(top, tx, ty, tw, 52, run->missed(), tr("verpasst", "missed"), th::ink); tx += tw + gap;
    drawStatTile(top, tx, ty, tw, 52, run->open(), tr("offen", "open"), th::warn); tx += tw + gap;
    drawStatTile(top, tx, ty, tw, 52, run->alive(), tr("lebend", "alive"), th::okInk); tx += tw + gap;
    drawStatTile(top, tx, ty, tw, 52, run->dead(), tr("tot", "dead"), th::badInk);
    if (!run->log.empty()) {
      drawIcon(top, "info", card.x + 16, card.y + 145, th::ink4);
      top.text(font_small, card.x + 26, card.y + 139, run->log.back().text, th::ink3, 330);
    }
  }
  keyHints(top, 222, {{"A", tr("\xc3\xb6" "ffnen", "open")}, {"Y", tr("neuer Lauf", "new run")}, {"B", tr("Men\xc3\xbc", "menu")}});
  if (!status.empty()) top.text(font_small, 200, 221, status, th::warnInk, 188, ALIGN_RIGHT);

  // ---- bottom: all runs + the two buttons
  drawBackdrop(bottom, Backdrop::Plain);
  drawSectionLabel(bottom, 14, 10, fmt(tr("ALLE L\xc3\x84UFE \xc2\xb7 %d", "ALL RUNS \xc2\xb7 %d"), (int)store.runs.size()), th::ink3, 306);
  Rect list{0, 24, 320, 172};
  bottom.setClip(list);
  const int RH = 50;
  int n = (int)store.runs.size();
  if (!n) {
    bottom.text(font_body, 0, 90, tr("Noch kein Lauf. Tippe auf \xe2\x80\x9eNeuer Lauf\xe2\x80\x9c.", "No run yet. Tap \xe2\x80\x9cNew run\xe2\x80\x9d."), th::ink3, 320, ALIGN_CENTER);
  }
  int maxScroll = std::max(0, n * RH - list.h + 8);
  scroll_ = std::max(0, std::min(scroll_, maxScroll));
  for (int i = 0; i < n; i++) {
    const Run &r = store.runs[n - 1 - i];  // newest first
    int y = list.y + 4 + i * RH - scroll_;
    if (y + RH < list.y || y > list.y + list.h) continue;
    bool sel = i == selected;
    const GameInfo *g = gameByKey(r.game.c_str());
    Rect rr{10, y, 300, RH - 7};
    drawCard(bottom, rr, 10, sel);
    drawCartridge(bottom, rr.x + 8, y + 6, 26, 31, g, r.status != "active");
    bottom.text(font_body_b, rr.x + 44, y + 5, r.name, th::ink, 170);
    std::string sub = std::string(g ? g->shortName : r.game.c_str()) +
                      (g ? (g->caps.fromBadges ? fmt(tr(" \xc2\xb7 %d Orden", " \xc2\xb7 %d badges"), r.badges) : fmt(tr(" \xc2\xb7 %d Pr\xc3\xbc" "fungen", " \xc2\xb7 %d trials"), r.milestone)) : "") +
                      fmt(tr(" \xc2\xb7 %d gefangen \xc2\xb7 %d tot", " \xc2\xb7 %d caught \xc2\xb7 %d dead"), r.caught(), r.dead());
    bottom.text(font_small, rr.x + 44, y + 23, sub, th::ink3, 186);
    if (r.status != "active") {
      std::string st = r.status == "lost" ? tr("VERLOREN", "LOST") : tr("GEWONNEN", "WON");
      drawChip(bottom, rr.x + rr.w - 10 - chipWidth(st), y + 15, st, r.status == "lost" ? th::badInk : th::warnInk);
    } else if (r.id == store.active) drawChip(bottom, rr.x + rr.w - 10 - chipWidth(tr("AKTIV", "ACTIVE")), y + 15, tr("AKTIV", "ACTIVE"), th::okInk);
    hits_.push_back({{rr.x, std::max(y, list.y), rr.w, rr.h}, HH_RUN, i});
  }
  bottom.resetClip();
  drawScrollHint(bottom, {list.x, list.y + 4, list.w - 2, list.h - 8}, scroll_, n * RH + 8);
  drawActionBar(bottom, 199);
  Rect back{10, 205, 110, 30}, nw{128, 205, 182, 30};
  drawButton(bottom, back, tr("Men\xc3\xbc", "Menu"), BtnStyle::Ghost, false, "back");
  drawButton(bottom, nw, tr("Neuer Lauf", "New run"), BtnStyle::Accent, false, "plus");
  hits_.push_back({back, HH_BACK, 0});
  hits_.push_back({nw, HH_NEW, 0});
}

HomeAction HomeScreen::handle(const Input &in, RunStore &store) {
  int n = (int)store.runs.size();
  if (in.touchDown) {
    downX_ = in.tx;
    downY_ = in.ty;
    drag_ = false;
  }
  if (in.touchHeld && std::abs(in.ty - downY_) > 6 && downY_ > 24 && downY_ < 196) drag_ = true;
  if (in.touchHeld && drag_) {
    scroll_ -= in.scrollDy;
    return HomeAction::Changed;
  }
  if (!drag_)
    if (const Hit *h = tapped(hits_, in, downX_, downY_)) {
      if (h->id == HH_RUN) {
        if (selected == h->arg) return HomeAction::OpenRun;
        selected = h->arg;
        return HomeAction::Changed;
      }
      if (h->id == HH_BACK) return HomeAction::Back;
      if (h->id == HH_NEW) return HomeAction::NewRun;
    }
  const int RH = 50;
  if ((in.pressed & K_DOWN) && selected < n - 1) {
    selected++;
    if ((selected + 1) * RH + 8 > scroll_ + 172) scroll_ = (selected + 1) * RH + 8 - 172;
    return HomeAction::Changed;
  }
  if ((in.pressed & K_UP) && selected > 0) {
    selected--;
    if (selected * RH < scroll_) scroll_ = selected * RH;
    return HomeAction::Changed;
  }
  if ((in.pressed & K_A) && n) return HomeAction::OpenRun;
  if (in.pressed & K_X) return n ? HomeAction::Continue : HomeAction::NewRun;
  if (in.pressed & K_Y) return HomeAction::NewRun;
  if (in.pressed & K_B) return HomeAction::Back;
  if (in.pressed & K_START) return HomeAction::Quit;
  return HomeAction::None;
}

// ---- new run -------------------------------------------------------------------------------------------------------

enum { WH_GAME = 1, WH_NEXT, WH_BACK, WH_PRESET, WH_RULE, WH_NAME };

void NewRunWizard::start(const std::vector<InstalledGame> &installed) {
  step = 0;
  focus = 0;
  rules = Rules();
  rules.applyPreset("classic");
  gameIndex = 0;
  version = 0;
  if (!installed.empty())
    for (int i = 0; i < gameCount(); i++)
      if (gameAt(i) == installed[0].game) {
        gameIndex = i;
        version = installed[0].version;
      }
  name = defaultName();
}

std::string NewRunWizard::defaultName() const {
  const GameInfo *g = gameAt(gameIndex);
  return std::string("Nuzlocke ") + (g ? g->versionNames[version] : "");
}

struct RuleRow {
  const char *de, *en;
  const char *helpDe, *helpEn;
};
static const RuleRow RULES[] = {
    {"Dupes-Klausel", "Dupes clause", "Eine Art (oder Entwicklungslinie), die du schon hast, z\xc3\xa4hlt nicht als Begegnung \xe2\x80\x93 die n\xc3\xa4" "chste z\xc3\xa4hlt.",
     "A species (or evolution line) you already have doesn't count as the encounter \xe2\x80\x93 the next one does."},
    {"Level-Caps", "Level caps", "Kein Pok\xc3\xa9mon \xc3\xbc" "ber dem Ass des n\xc3\xa4" "chsten Arenaleiters. Warnen: Hinweis. Streng: rote Warnung, nicht einsetzen.",
     "No Pok\xc3\xa9mon above the next leader's ace. Warn: a hint. Strict: a red warning, don't use it."},
    {"Blackout beendet Lauf", "Whiteout ends the run", "F\xc3\xa4llt das ganze Team, ist der Lauf vorbei.", "If the whole team goes down, the run is over."},
    {"Geschenke z\xc3\xa4hlen", "Gifts count", "Geschenkte und statische Pok\xc3\xa9mon sind die Begegnung ihres Gebiets.",
     "Gifted and static Pok\xc3\xa9mon are the encounter of their area."},
    {"Deathless", "Deathless", "Der erste Tod beendet den Lauf.", "The first death ends the run."},
    {"Keine Items im Kampf", "No items in battle", "Hardcore: Heiltr\xc3\xa4nke nur au\xc3\x9f" "erhalb von K\xc3\xa4mpfen (Ehrensache).",
     "Hardcore: healing items only outside battles (honour system)."},
    {"Set-Modus", "Set mode", "Hardcore: kein Wechsel, wenn der Gegner ein Pok\xc3\xa9mon besiegt hat (Ehrensache).",
     "Hardcore: no switching after you knock out an opponent's Pok\xc3\xa9mon (honour system)."},
};
constexpr int RULE_COUNT = 7;

static std::string ruleState(const Rules &r, int i) {
  switch (i) {
    case 0: return r.dupes == "line" ? tr("Linie", "line") : r.dupes == "species" ? tr("Art", "species") : tr("aus", "off");
    case 1: return r.levelCaps == "strict" ? tr("streng", "strict") : r.levelCaps == "warn" ? tr("warnen", "warn") : tr("aus", "off");
    case 2: return r.blackout == "party" ? tr("an", "on") : tr("aus", "off");
    case 3: return r.gifts == "area" ? tr("an", "on") : tr("aus", "off");
    case 4: return r.deathless ? tr("an", "on") : tr("aus", "off");
    case 5: return r.noBattleItems ? tr("an", "on") : tr("aus", "off");
    default: return r.setMode ? tr("an", "on") : tr("aus", "off");
  }
}
static bool ruleIsBinary(int i) { return i >= 2; }
static bool ruleOn(const Rules &r, int i) {
  switch (i) {
    case 0: return r.dupes != "off";
    case 1: return r.levelCaps != "off";
    case 2: return r.blackout == "party";
    case 3: return r.gifts == "area";
    case 4: return r.deathless;
    case 5: return r.noBattleItems;
    default: return r.setMode;
  }
}
static void ruleCycle(Rules &r, int i) {
  switch (i) {
    case 0: r.dupes = r.dupes == "line" ? "species" : r.dupes == "species" ? "off" : "line"; break;
    case 1: r.levelCaps = r.levelCaps == "warn" ? "strict" : r.levelCaps == "strict" ? "off" : "warn"; break;
    case 2: r.blackout = r.blackout == "party" ? "off" : "party"; break;
    case 3: r.gifts = r.gifts == "area" ? "free" : "area"; break;
    case 4: r.deathless = !r.deathless; break;
    case 5: r.noBattleItems = !r.noBattleItems; break;
    default: r.setMode = !r.setMode; break;
  }
}

void NewRunWizard::draw(Canvas &top, Canvas &bottom, const std::vector<InstalledGame> &installed) {
  hits_.clear();
  drawBackdrop(top, Backdrop::Plain);
  header(top, tr("Neuer Lauf", "New run"));
  // the steps: 1 game · 2 rules · 3 name
  const char *steps[] = {tr("Spiel", "Game"), tr("Regeln", "Rules"), tr("Name", "Name")};
  int sw = 0;
  for (int i = 0; i < 3; i++) sw += 18 + Canvas::textWidth(font_small, steps[i]) + (i < 2 ? 18 : 0);
  int sx = 388 - sw;
  for (int i = 0; i < 3; i++) {
    bool on = i == step, done = i < step;
    top.circle(sx + 7, 21, 7, on ? th::p1 : done ? th::tri : th::shell4);
    if (done) drawIcon(top, "check", sx + 2, 21, rgb(0x14251d));
    else top.text(font_label, sx, 15, std::to_string(i + 1), on ? rgb(0x12131c) : th::ink3, 15, ALIGN_CENTER);
    int tw = top.text(font_small, sx + 18, 15, steps[i], on ? th::ink : th::ink3);
    sx += 18 + tw;
    if (i < 2) {
      top.hline(sx + 4, 21, 10, done ? th::tri : th::shell4);
      sx += 18;
    }
  }
  const GameInfo *g = gameAt(gameIndex);
  drawBackdrop(bottom, Backdrop::Plain);
  if (step == 0) {
    top.textWrapped(font_body, 14, 48, 372, tr("W\xc3\xa4hle das Spiel. souldew sucht installierte Titel und die eingesteckte Karte.",
                                                "Pick the game. souldew looks for installed titles and the inserted cartridge."),
                    th::ink2);
    // details of the focused game
    Rect card{14, 92, 372, 116};
    Color acc = gameColor(g->key);
    top.shadow(card.x, card.y, card.w, card.h, 12, 5, 3, 110);
    top.roundRect(card.x, card.y, card.w, card.h, 12, mix(th::edge, acc, 0.3f));
    top.roundRectGradient(card.x + 1, card.y + 1, card.w - 2, card.h - 2, 11, mix(rgb(0x2a2931), acc, 0.18f), rgb(0x232229));
    top.hline(card.x + 12, card.y + 1, card.w - 24, Color(255, 255, 255, 22));
    drawCartridge(top, card.x + 16, card.y + 16, 62, 72, g);
    int tx = card.x + 96;
    top.text(font_head, tx, card.y + 14, g->name, th::ink, 260);
    const InstalledGame *ig = findInstalled(installed, g->key);
    std::string where = ig ? (ig->cartridge ? tr("Karte eingesteckt", "cartridge inserted") : tr("auf der SD-Karte installiert", "installed on the SD card"))
                           : tr("nicht gefunden \xe2\x80\x93 Lauf trotzdem m\xc3\xb6glich", "not found \xe2\x80\x93 you can still create the run");
    drawIcon(top, ig ? "check" : "warn", tx, card.y + 41, ig ? th::okInk : th::warnInk);
    top.text(font_small, tx + 14, card.y + 35, where, ig ? th::okInk : th::warnInk, 250);
    std::string live = fmt(tr("Generation %d \xc2\xb7 live: Team, K\xc3\xa4mpfe, Gebiet%s", "Generation %d \xc2\xb7 live: team, battles, area%s"),
                           g->gen, g->caps.fromBadges ? tr(", Orden", ", badges") : "");
    top.text(font_small, tx, card.y + 55, live, th::ink3, 260);
    std::string caps = fmt(tr("Level-Caps %d \xe2\x80\x93 %d \xc2\xb7 %s", "Level caps %d \xe2\x80\x93 %d \xc2\xb7 %s"), g->caps.caps[0], g->caps.finalCap,
                           g->caps.fromBadges ? tr("nach Orden", "by badges") : tr("nach Pr\xc3\xbc" "fungen", "by trials"));
    top.text(font_small, tx, card.y + 71, caps, th::ink3, 260);
    // the four games
    for (int i = 0; i < gameCount(); i++) {
      const GameInfo *gi = gameAt(i);
      Rect r{10 + (i % 2) * 154, 10 + (i / 2) * 90, 146, 82};
      bool sel = i == gameIndex;
      drawCard(bottom, r, 12, sel);
      const InstalledGame *in2 = findInstalled(installed, gi->key);
      drawCartridge(bottom, r.x + 10, r.y + 12, 34, 40, gi, !in2);
      bottom.text(font_head, r.x + 52, r.y + 12, gi->shortName, th::ink, 88);
      bottom.text(font_small, r.x + 52, r.y + 31, fmt("Gen %d", gi->gen), th::ink3);
      std::string chip = in2 ? (in2->cartridge ? tr("KARTE", "CART") : tr("INSTALLIERT", "INSTALLED")) : tr("FEHLT", "MISSING");
      drawChip(bottom, r.x + 10, r.y + 60, chip, in2 ? th::okInk : th::ink3);
      hits_.push_back({r, WH_GAME, i});
    }
  } else if (step == 1) {
    top.text(font_small, 14, 48, fmt(tr("%s \xc2\xb7 Regeln", "%s \xc2\xb7 rules"), g->name), th::ink2);
    int f = std::max(0, std::min(focus, RULE_COUNT));
    std::string help = f == 0 ? tr("Klassisch: Dupes-Klausel, Level-Caps als Warnung. Hardcore: strenge Caps, keine Items im Kampf, Set-Modus.",
                                   "Classic: dupes clause, level caps as a warning. Hardcore: strict caps, no items in battle, set mode.")
                              : tr(RULES[f - 1].helpDe, RULES[f - 1].helpEn);
    Rect card{14, 68, 372, 96};
    drawSurface(top, card, 12, rgb(0x2b2a33), rgb(0x232229), th::edge, 4);
    top.roundRect(card.x + 1, card.y + 10, 3, card.h - 20, 1, th::p1);
    top.textSpaced(font_label, card.x + 16, card.y + 10, f == 0 ? tr("VOREINSTELLUNG", "PRESET") : tr("REGEL", "RULE"), th::ink3, 1);
    top.text(font_head, card.x + 16, card.y + 22, f == 0 ? tr("Klassisch oder Hardcore", "Classic or Hardcore") : tr(RULES[f - 1].de, RULES[f - 1].en), th::crsInk, 340);
    top.textWrapped(font_small, card.x + 16, card.y + 44, 340, help, th::ink2, 2, 3);
    drawIcon(top, "info", 16, 186, th::ink4);
    top.text(font_small, 26, 180, tr("Immer an: nur die erste Begegnung pro Gebiet, gefallen = tot.", "Always on: only the first encounter per area, fallen = dead."), th::ink3, 362);
    // presets
    std::string pn = rules.presetName();
    Rect pc{10, 8, 148, 30}, ph{162, 8, 148, 30};
    drawButton(bottom, pc, tr("Klassisch", "Classic"), pn == "classic" ? BtnStyle::Accent : BtnStyle::Ghost, focus == 0 && pn != "hardcore", pn == "classic" ? "check" : nullptr);
    drawButton(bottom, ph, "Hardcore", pn == "hardcore" ? BtnStyle::Danger : BtnStyle::Ghost, focus == 0 && pn == "hardcore", pn == "hardcore" ? "check" : nullptr);
    hits_.push_back({pc, WH_PRESET, 0});
    hits_.push_back({ph, WH_PRESET, 1});
    drawSurface(bottom, {8, 44, 304, RULE_COUNT * 21 + 4}, 10, rgb(0x26252c), rgb(0x222128), th::edge);
    for (int i = 0; i < RULE_COUNT; i++) {
      int y = 46 + i * 21;
      bool foc = focus == i + 1;
      if (foc) {
        bottom.roundRect(10, y, 300, 20, 7, th::p1.alpha(40));
        bottom.roundRectStroke(10, y, 300, 20, 7, 1, th::p1.alpha(160));
      }
      bottom.text(font_body, 18, y + 2, tr(RULES[i].de, RULES[i].en), th::ink, 180);
      if (ruleIsBinary(i)) drawSwitch(bottom, 276, y + 2, ruleOn(rules, i));
      else {
        std::string v = ruleState(rules, i);
        drawChip(bottom, 304 - chipWidth(v), y + 4, v, ruleOn(rules, i) ? th::okInk : th::ink3);
      }
      if (i < RULE_COUNT - 1 && !foc && focus != i + 2) bottom.hline(18, y + 20, 284, Color(255, 255, 255, 10));
      hits_.push_back({{8, y, 304, 21}, WH_RULE, i});
    }
  } else {
    top.textWrapped(font_body, 14, 48, 372, tr("Gib dem Lauf einen Namen. Tippe auf das Feld, um ihn zu \xc3\xa4ndern.",
                                                "Name the run. Tap the field to change it."),
                    th::ink2);
    Rect card{14, 84, 372, 120};
    Color acc = gameColor(g->key);
    top.shadow(card.x, card.y, card.w, card.h, 12, 5, 3, 110);
    top.roundRect(card.x, card.y, card.w, card.h, 12, mix(th::edge, acc, 0.3f));
    top.roundRectGradient(card.x + 1, card.y + 1, card.w - 2, card.h - 2, 11, mix(rgb(0x2a2931), acc, 0.18f), rgb(0x232229));
    drawCartridge(top, card.x + 16, card.y + 18, 62, 72, g);
    top.text(font_display, card.x + 96, card.y + 16, name, th::ink, 260);
    top.text(font_small, card.x + 96, card.y + 46, fmt("%s \xc2\xb7 %s", g->name, g->versionNames[version]), th::ink2, 260);
    std::string pn = rules.presetName();
    drawChip(top, card.x + 96, card.y + 68, pn == "hardcore" ? "HARDCORE" : pn == "classic" ? tr("KLASSISCH", "CLASSIC") : tr("EIGENE REGELN", "CUSTOM RULES"),
             pn == "hardcore" ? th::badInk : th::crsInk);
    int on = 0;
    for (int i = 0; i < RULE_COUNT; i++) on += ruleOn(rules, i) ? 1 : 0;
    top.text(font_small, card.x + 96, card.y + 88, fmt(tr("%d von %d Regeln an", "%d of %d rules on"), on, RULE_COUNT), th::ink3);
    Rect nf{10, 18, 300, 46};
    drawSectionLabel(bottom, 14, 6, tr("NAME", "NAME"), th::ink3);
    bottom.roundRect(nf.x, nf.y, nf.w, nf.h, 10, th::p1.alpha(160));
    bottom.roundRect(nf.x + 1, nf.y + 1, nf.w - 2, nf.h - 2, 9, th::well);
    bottom.text(font_body_b, 22, 32, name, th::ink, 230);
    bottom.fill(22 + std::min(230, Canvas::textWidth(font_body_b, name)) + 2, 31, 2, 16, th::p1);  // a caret
    bottom.text(font_label, 0, 35, tr("\xc3\x84NDERN", "EDIT"), th::crsInk, 296, ALIGN_RIGHT);
    hits_.push_back({nf, WH_NAME, 0});
    // version choice (X / Y …)
    drawSectionLabel(bottom, 14, 78, tr("EDITION", "VERSION"), th::ink3);
    for (int v = 0; v < 2; v++) {
      Rect r{10 + v * 154, 92, 146, 40};
      Color a, b;
      gameVersionColors(g->key, a, b);
      bool sel = v == version;
      drawCard(bottom, r, 10, sel);
      bottom.circle(r.x + 20, r.y + 20, 8, v == 0 ? a : b);
      bottom.text(font_body_b, r.x + 36, r.y + 11, g->versionNames[v], th::ink, 100);
      if (sel) drawIcon(bottom, "check", r.x + r.w - 20, r.y + 20, th::okInk);
      hits_.push_back({r, WH_GAME, 100 + v});
    }
  }
  // navigation
  drawActionBar(bottom, 199);
  Rect back{10, 205, 110, 30}, next{128, 205, 182, 30};
  drawButton(bottom, back, step == 0 ? tr("Abbrechen", "Cancel") : tr("Zur\xc3\xbc" "ck", "Back"), BtnStyle::Ghost, false, "back");
  drawButton(bottom, next, step == 2 ? tr("Lauf anlegen", "Create run") : tr("Weiter", "Next"), step == 2 ? BtnStyle::Primary : BtnStyle::Accent, false,
             step == 2 ? "check" : "chevron");
  hits_.push_back({back, WH_BACK, 0});
  hits_.push_back({next, WH_NEXT, 0});
  if (step == 1) keyHints(top, 222, {{"A", tr("\xc3\xa4ndern", "change")}, {"B", tr("zur\xc3\xbc" "ck", "back")}, {"START", tr("weiter", "next")}});
  else if (step == 2) keyHints(top, 222, {{"A", tr("anlegen", "create")}, {"B", tr("zur\xc3\xbc" "ck", "back")}, {"Y", tr("Name", "name")}});
  else keyHints(top, 222, {{"A", tr("weiter", "next")}, {"B", tr("abbrechen", "cancel")}});
}

WizardAction NewRunWizard::handle(const Input &in, const std::vector<InstalledGame> &) {
  int id = 0, arg = 0;
  if (in.touchDown) {
    downX_ = in.tx;
    downY_ = in.ty;
  }
  if (const Hit *h = tapped(hits_, in, downX_, downY_)) id = h->id, arg = h->arg;
  if (in.pressed & K_A) id = (step == 1 && focus > 0) ? WH_RULE : WH_NEXT, arg = focus - 1;
  if ((in.pressed & K_START) && step < 2) id = WH_NEXT;
  if (in.pressed & K_B) id = WH_BACK;
  if (step == 0) {
    int n = gameCount();
    if (in.pressed & K_RIGHT) gameIndex = (gameIndex + 1) % n, version = 0;
    if (in.pressed & K_LEFT) gameIndex = (gameIndex + n - 1) % n, version = 0;
    if (in.pressed & K_DOWN) gameIndex = (gameIndex + 2) % n, version = 0;
    if (in.pressed & K_UP) gameIndex = (gameIndex + n - 2) % n, version = 0;
    if (in.pressed & (K_LEFT | K_RIGHT | K_UP | K_DOWN)) {
      name = defaultName();
      return WizardAction::Changed;
    }
  } else if (step == 1) {
    if (in.pressed & K_DOWN) focus = std::min(RULE_COUNT, focus + 1);
    if (in.pressed & K_UP) focus = std::max(0, focus - 1);
    if ((in.pressed & (K_LEFT | K_RIGHT)) && focus == 0) id = WH_PRESET, arg = rules.presetName() == "hardcore" ? 0 : 1;
    else if (in.pressed & (K_LEFT | K_RIGHT)) id = WH_RULE, arg = focus - 1;
    if (in.pressed & (K_UP | K_DOWN)) return WizardAction::Changed;
  } else {
    if (in.pressed & K_Y) return WizardAction::AskName;
    if (in.pressed & (K_LEFT | K_RIGHT)) {
      bool def = name == defaultName();
      version = 1 - version;
      if (def) name = defaultName();
      return WizardAction::Changed;
    }
  }
  switch (id) {
    case WH_GAME:
      if (arg >= 100) {
        bool def = name == defaultName();
        version = arg - 100;
        if (def) name = defaultName();
      } else {
        if (gameIndex == arg) {
          step = 1;
          focus = 0;
        }
        gameIndex = arg;
        version = 0;
        name = defaultName();
      }
      return WizardAction::Changed;
    case WH_PRESET:
      rules.applyPreset(arg ? "hardcore" : "classic");
      focus = 0;
      return WizardAction::Changed;
    case WH_RULE:
      if (arg >= 0 && arg < RULE_COUNT) {
        ruleCycle(rules, arg);
        focus = arg + 1;
      }
      return WizardAction::Changed;
    case WH_NAME: return WizardAction::AskName;
    case WH_BACK:
      if (step == 0) return WizardAction::Cancel;
      step--;
      focus = 0;
      return WizardAction::Changed;
    case WH_NEXT:
      if (step == 2) return WizardAction::Create;
      step++;
      focus = 0;
      return WizardAction::Changed;
  }
  return WizardAction::None;
}

// ---- achievements --------------------------------------------------------------------------------------------------

enum { TH_FILTER = 1, TH_ITEM, TH_BACK };

static std::string dateOf(long long ts) {
  // the console's clock: seconds since 1900 (local); the host's: since 1970. Only a day number fits both: show the
  // date for console times (> 1990 in 1900-seconds), else nothing
  if (ts < 2840140800LL) return "";
  long long days = ts / 86400 - 25567;  // → days since 1970-01-01
  // civil from days (Howard Hinnant)
  long long z = days + 719468, era = (z >= 0 ? z : z - 146096) / 146097;
  long long doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  long long y = yoe + era * 400, doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
  long long d = doy - (153 * mp + 2) / 5 + 1, m = mp < 10 ? mp + 3 : mp - 9;
  y += m <= 2;
  return g_lang == Lang::En ? fmt("%04lld-%02lld-%02lld", y, m, d) : fmt("%02lld.%02lld.%04lld", d, m, y);
}

void TrophyScreen::keepVisible() {
  const int RH = 44;
  int row = focus / 2;
  int top = row * RH, bottom = top + RH + 8;
  if (top < scroll_) scroll_ = top;
  if (bottom > scroll_ + 170) scroll_ = bottom - 170;
  scroll_ = std::max(0, std::min(scroll_, std::max(0, content_ - 170)));
}

void TrophyScreen::draw(Canvas &top, Canvas &bottom, const RunStore &store) {
  hits_.clear();
  std::vector<TrophyState> states = evaluateTrophies(store);
  TrophyTotals tt = trophyTotals(store, states);
  std::vector<MedalState> medals = medalStates(states);
  shown_.clear();
  for (int i = 0; i < (int)states.size(); i++)
    if (filter == 0 || (int)states[i].def->cat == filter - 1) shown_.push_back(i);
  // unlocked first, then by progress
  std::stable_sort(shown_.begin(), shown_.end(), [&](int a, int b) {
    const TrophyState &x = states[a], &y = states[b];
    if (x.ok != y.ok) return x.ok;
    if (x.ok) return x.at > y.at;
    return (float)x.n / x.def->target > (float)y.n / y.def->target;
  });
  focus = std::max(0, std::min(focus, (int)shown_.size() - 1));

  // ---- top: the medal case + the focused achievement
  drawBackdrop(top, Backdrop::Hero);
  header(top, tr("Erfolge", "Achievements"), fmt(tr("%d PUNKTE", "%d POINTS"), tt.points), th::warnInk);
  int mx = 18;
  for (int i = 0; i < 5; i++) {
    const MedalState &m = medals[i];
    int x = mx + i * 74;
    top.roundRect(x, 42, 66, 96, 10, Color(255, 255, 255, 8));
    if (m.tier >= 0) top.glow(x + 33, 82, 34, 34, rgb(0xfff1ad, 22));
    drawMedal(top, x + 9, 46, m.kind, m.tier, 2);
    top.text(font_label, x, 112, m.tier >= 0 ? tierLabel(m.tier) : "\xe2\x80\x94", m.tier >= 0 ? th::warnInk : th::ink4, 66, ALIGN_CENTER);
    std::string v = m.next ? fmt("%d/%d", m.value, m.next) : std::string(tr("MAX", "MAX"));
    top.text(font_label, x, 123, v, th::ink3, 66, ALIGN_CENTER);
  }
  if (!shown_.empty()) {
    const TrophyState &s = states[shown_[focus]];
    const TrophyDef &d = *s.def;
    bool secret = d.hidden && !s.ok;
    Rect card{12, 146, 376, 86};
    Color rc = rarityColor((int)d.rarity);
    drawSurface(top, card, 12, mix(rgb(0x2b2a33), rc, s.ok ? 0.10f : 0.f), rgb(0x222128), s.ok ? mix(th::edge, rc, 0.5f) : th::edge, 4);
    drawCoin(top, card.x + 12, card.y + 14, 4, (int)d.cat, (int)d.rarity, !s.ok, d.hidden);
    int tx = card.x + 76;
    std::string chip = std::string(rarityLabel(d.rarity)) + fmt(" \xc2\xb7 %d", trophyPoints(d.rarity));
    for (auto &ch : chip) ch = (char)toupper((unsigned char)ch);
    top.text(font_head, tx, card.y + 9, secret ? tr("Geheimer Erfolg", "Secret achievement") : trophyName(d), s.ok ? th::ink : th::ink2,
             card.x + card.w - 22 - chipWidth(chip) - tx);
    drawChip(top, card.x + card.w - 12 - chipWidth(chip), card.y + 11, chip, mix(rc, th::ink, 0.25f));
    top.textWrapped(font_small, tx, card.y + 30, card.w - 90,
                    secret ? tr("Spiel weiter \xe2\x80\x93 du merkst es, wenn es passiert.", "Keep playing \xe2\x80\x93 you'll know when it happens.") : trophyDesc(d),
                    th::ink3, 2, 2);
    if (s.ok) {
      drawIcon(top, "check", tx, card.y + 70, th::okInk);
      std::string date = dateOf(s.at);
      top.text(font_small, tx + 14, card.y + 64, date.empty() ? std::string(tr("Freigeschaltet", "Unlocked")) : fmt(tr("Freigeschaltet am %s", "Unlocked %s"), date.c_str()), th::okInk, 260);
    } else if (d.target > 1) {
      drawProgress(top, tx, card.y + 68, 200, 6, s.n, d.target, rc);
      top.text(font_small, tx + 208, card.y + 63, fmt("%d / %d", s.n, d.target), th::ink2);
    } else {
      drawIcon(top, "lock", tx, card.y + 70, th::ink4);
      top.text(font_small, tx + 14, card.y + 64, tr("Noch gesperrt", "Still locked"), th::ink4);
    }
  }

  // ---- bottom: the filter + the grid
  drawBackdrop(bottom, Backdrop::Plain);
  std::vector<std::string> tabs = {tr("Alle", "All"), tr("Reise", "Journey"), tr("Sammeln", "Collect"), tr("K\xc3\xb6nnen", "Skill"), tr("Spa\xc3\x9f", "Fun")};
  const int RH = 44;
  Rect area{0, 30, 320, 168};
  content_ = ((int)shown_.size() + 1) / 2 * RH + 26;
  scroll_ = std::max(0, std::min(scroll_, std::max(0, content_ - 170)));
  bottom.setClip(area);
  int unlockedHere = 0;
  for (int i : shown_) unlockedHere += states[i].ok ? 1 : 0;
  drawSectionLabel(bottom, 14, area.y + 6 - scroll_, fmt(tr("%d VON %d FREIGESCHALTET", "%d OF %d UNLOCKED"), unlockedHere, (int)shown_.size()), th::ink3, 306);
  for (int k = 0; k < (int)shown_.size(); k++) {
    const TrophyState &s = states[shown_[k]];
    const TrophyDef &d = *s.def;
    int col = k % 2, row = k / 2;
    Rect r{8 + col * 154, area.y + 20 + row * RH - scroll_, 150, RH - 5};
    if (r.y + r.h < area.y || r.y > area.y + area.h) continue;
    bool f = k == focus;
    bool secret = d.hidden && !s.ok;
    if (s.ok) {
      Color rc = rarityColor((int)d.rarity);
      if (f) bottom.roundRect(r.x - 2, r.y - 2, r.w + 4, r.h + 4, 11, th::p1.alpha(70));
      drawSurface(bottom, r, 9, mix(rgb(0x2c2b33), rc, 0.12f), rgb(0x232229), f ? th::p1.alpha(200) : mix(th::edge, rc, 0.35f));
    } else drawCard(bottom, r, 9, f);
    drawCoin(bottom, r.x + 6, r.y + 6, 2, (int)d.cat, (int)d.rarity, !s.ok, d.hidden);
    bottom.text(font_body_b, r.x + 37, r.y + 3, secret ? "???" : trophyName(d), s.ok ? th::ink : th::ink2, r.w - 42);
    if (s.ok) bottom.text(font_small, r.x + 37, r.y + 20, rarityLabel(d.rarity), mix(rarityColor((int)d.rarity), th::ink, 0.3f), r.w - 42);
    else if (d.target > 1) {
      drawProgress(bottom, r.x + 37, r.y + 25, r.w - 82, 4, s.n, d.target, th::ink3);
      std::string p = fmt("%d/%d", s.n, d.target);
      bottom.text(font_small, r.x + r.w - 6 - Canvas::textWidth(font_small, p), r.y + 20, p, th::ink3);
    } else bottom.text(font_small, r.x + 37, r.y + 20, secret ? tr("geheim", "secret") : tr("gesperrt", "locked"), th::ink4, r.w - 42);
    hits_.push_back({{r.x, std::max(r.y, area.y), r.w, r.h}, TH_ITEM, k});
  }
  bottom.resetClip();
  drawScrollHint(bottom, {area.x, area.y + 4, area.w - 1, area.h - 8}, scroll_, content_);
  drawTabBar(bottom, 0, tabs, filter, &hits_, TH_FILTER);
  drawActionBar(bottom, 199);
  Rect back{10, 205, 110, 30};
  drawButton(bottom, back, tr("Men\xc3\xbc", "Menu"), BtnStyle::Ghost, false, "back");
  hits_.push_back({back, TH_BACK, 0});
  bottom.text(font_small, 132, 212, fmt(tr("%d von %d \xc2\xb7 %d Arten", "%d of %d \xc2\xb7 %d species"), tt.unlocked, tt.total, tt.species), th::ink3, 178, ALIGN_RIGHT);
}

TrophyAction TrophyScreen::handle(const Input &in, const RunStore &) {
  if (in.touchDown) {
    downX_ = in.tx;
    downY_ = in.ty;
    drag_ = false;
  }
  if (in.touchHeld && std::abs(in.ty - downY_) > 6 && downY_ > 30 && downY_ < 198) drag_ = true;
  if (in.touchHeld && drag_) {
    scroll_ = std::max(0, std::min(std::max(0, content_ - 170), scroll_ - in.scrollDy));
    return TrophyAction::Changed;
  }
  int n = (int)shown_.size();
  if (!drag_)
    if (const Hit *h = tapped(hits_, in, downX_, downY_)) {
      if (h->id == TH_BACK) return TrophyAction::Back;
      if (h->id == TH_FILTER) {
        filter = h->arg;
        focus = 0;
        scroll_ = 0;
        return TrophyAction::Changed;
      }
      if (h->id == TH_ITEM) {
        focus = h->arg;
        return TrophyAction::Changed;
      }
    }
  if (in.pressed & K_B) return TrophyAction::Back;
  int old = focus;
  if (in.pressed & K_L) filter = (filter + 4) % 5, focus = 0, scroll_ = 0;
  if (in.pressed & K_R) filter = (filter + 1) % 5, focus = 0, scroll_ = 0;
  if (in.pressed & (K_L | K_R)) return TrophyAction::Changed;
  if ((in.pressed & K_RIGHT) && focus % 2 == 0 && focus + 1 < n) focus++;
  if ((in.pressed & K_LEFT) && focus % 2 == 1) focus--;
  if ((in.pressed & K_DOWN) && focus + 2 < n) focus += 2;
  if ((in.pressed & K_UP) && focus >= 2) focus -= 2;
  if (focus != old) {
    keepVisible();
    return TrophyAction::Changed;
  }
  return TrophyAction::None;
}

// ---- settings ------------------------------------------------------------------------------------------------------

enum { SH_ROW = 1, SH_LANG, SH_BACK };

void SettingsScreen::draw(Canvas &top, Canvas &bottom, const RunStore &store) {
  hits_.clear();
  drawBackdrop(top, Backdrop::Plain);
  header(top, tr("Einstellungen", "Settings"));
  const Settings &s = store.settings;
  // ---- top: what the focused setting does, with a small preview
  std::string title, text;
  switch (focus) {
    case 0:
      title = tr("Sprache", "Language");
      text = tr("souldew spricht Deutsch oder Englisch \xe2\x80\x93 im Launcher und im Spiel. Orte und Pok\xc3\xa9mon-Namen folgen der Sprache.",
                "souldew speaks German or English \xe2\x80\x93 in the launcher and in game. Places and Pok\xc3\xa9mon names follow the language.");
      break;
    case 1:
      title = tr("Overlays im Spiel", "Overlays in game");
      text = tr("Kleine Karten oben auf dem Spielbild: neues Gebiet, erste Begegnung, Fang, Tod, Level-Cap, Orden, Erfolge. Aus: souldew z\xc3\xa4hlt still mit.",
                "Small cards on top of the game: new area, first encounter, catch, death, level cap, badge, achievements. Off: souldew keeps count quietly.");
      break;
    case 2:
      title = tr("Lasche unten rechts", "Corner tab");
      text = tr("Ein kleiner Griff in der Ecke des Touchscreens. Kurz halten \xc3\xb6" "ffnet souldew \xe2\x80\x93 wie SELECT. Aus, wenn er im Spiel st\xc3\xb6rt.",
                "A small handle in the touch screen's corner. Hold it briefly to open souldew \xe2\x80\x93 like SELECT. Off if it gets in the game's way.");
      break;
    case 3:
      title = tr("Einf\xc3\xbchrung", "Introduction");
      text = tr("Die kurze Tour vom ersten Start noch einmal: was souldew macht, wie du es im Spiel \xc3\xb6" "ffnest, die Nuzlocke-Regeln.",
                "The short tour from the first start once more: what souldew does, how to open it in game, the Nuzlocke rules.");
      break;
    default:
      title = tr("\xc3\x9c" "ber souldew", "About souldew");
      text = tr("souldew ist kostenlos und quelloffen (GPL-3.0). Nicht verbunden mit Nintendo, Game Freak oder The Pok\xc3\xa9mon Company.",
                "souldew is free and open source (GPL-3.0). Not affiliated with Nintendo, Game Freak or The Pok\xc3\xa9mon Company.");
      break;
  }
  top.textSpaced(font_label, 16, 48, fmt("%d / 5", focus + 1), th::ink4, 1);
  top.text(font_head, 16, 58, title, th::ink, 368);
  top.textWrapped(font_small, 16, 80, 368, text, th::ink2, 2, 3);
  Rect prev{16, 128, 368, 92};
  if (focus == 0) {
    drawSurface(top, prev, 12, rgb(0x2b2a33), rgb(0x222128), th::edge, 3);
    top.text(font_display, prev.x + 18, prev.y + 14, tr("Hallo!", "Hello!"), th::ink);
    top.text(font_body, prev.x + 18, prev.y + 46, tr("Erste Begegnung: Wingull Lv 13", "First encounter: Wingull Lv 13"), th::ink2, 330);
    top.text(font_small, prev.x + 18, prev.y + 66, tr("Pok\xc3\xa9mon-Namen: Deutsch (Wingull, Quaxi, Dartignis)", "Pok\xc3\xa9mon names: English (Wingull, Frogadier, Fletchinder)"), th::ink3, 330);
  } else if (focus == 1) {
    drawSceneArt(top, prev, true);
    if (s.overlays) {
      Event e{EventKind::Caught, Tone::Ok, fmt(tr("Gefangen: %s", "Caught: %s"), speciesName(278, g_lang)), "Route 8", 278};
      drawToast(top, prev.x + 8, prev.y + 8, 250, e, 1.f);
    } else {
      top.roundRect(prev.x + 8, prev.y + 8, 160, 22, 11, Color(0, 0, 0, 120));
      drawIcon(top, "eye", prev.x + 16, prev.y + 19, th::ink3);
      top.text(font_small, prev.x + 34, prev.y + 13, tr("Overlays aus", "Overlays off"), th::ink2);
    }
  } else if (focus == 2) {
    top.roundRect(prev.x + 92, prev.y - 2, 184, 96, 8, rgb(0x050407));
    top.roundRectVGradient(prev.x + 94, prev.y, 180, 92, 6, rgb(0x2d4c78), rgb(0x1f3756));
    if (s.tab) {
      drawCornerTab(top, prev.x + 94 + 180 - 34, prev.y + 92 - 22, false);
      top.glow(prev.x + 94 + 180 - 19, prev.y + 92 - 13, 30, 20, th::p1.alpha(50));
    }
    top.text(font_small, prev.x + 94, prev.y + 38, tr("Touchscreen des Spiels", "the game's touch screen"), rgb(0xbcd4f0), 180, ALIGN_CENTER);
  } else if (focus == 3) {
    drawSurface(top, prev, 12, rgb(0x2b2a33), rgb(0x222128), th::edge, 3);
    top.glow(prev.x + 70, prev.y + 46, 70, 40, rgb(0x22362a, 160));
    drawLogoMark(top, prev.x + 22, prev.y + 22, 3, 1.f, true);
    top.text(font_body_b, prev.x + 136, prev.y + 24, tr("5 kurze Seiten", "5 short pages"), th::ink);
    top.text(font_small, prev.x + 136, prev.y + 44, tr("Sprache \xc2\xb7 souldew \xc2\xb7 im Spiel \xc2\xb7 Regeln", "language \xc2\xb7 souldew \xc2\xb7 in game \xc2\xb7 rules"), th::ink3, 220);
  } else {
    drawSurface(top, prev, 12, rgb(0x2b2a33), rgb(0x222128), th::edge, 3);
    drawLogo(top, prev.x + 16, prev.y + 14);
    top.text(font_small, prev.x + 16, prev.y + 50, std::string("souldew 3DS ") + kVersion + tr(" \xc2\xb7 Launcher + Plugin", " \xc2\xb7 launcher + plugin"), th::ink2);
    top.text(font_small, prev.x + 16, prev.y + 66, "github.com/souldewapp/souldew", th::crsInk);
  }

  // ---- bottom: the settings
  drawBackdrop(bottom, Backdrop::Plain);
  drawSectionLabel(bottom, 14, 8, tr("ALLGEMEIN", "GENERAL"), th::ink3, 306);
  struct Row {
    const char *icon, *label;
    int kind;  // 0 language, 1 switch, 2 link
    bool on;
  } rows[] = {
      {"globe", tr("Sprache", "Language"), 0, false},
      {"eye", tr("Overlays im Spiel", "Overlays in game"), 1, s.overlays},
      {"pin", tr("Lasche unten rechts", "Corner tab"), 1, s.tab},
      {"help", tr("Einf\xc3\xbchrung ansehen", "Show the introduction"), 2, false},
      {"info", tr("\xc3\x9c" "ber souldew", "About souldew"), 2, false},
  };
  int y = 22;
  for (int i = 0; i < 5; i++) {
    if (i == 3) {
      drawSectionLabel(bottom, 14, y + 2, tr("SOULDEW", "SOULDEW"), th::ink3, 306);
      y += 16;
    }
    Rect r{10, y, 300, 28};
    bool f = focus == i;
    drawCard(bottom, r, 9, f);
    drawIcon(bottom, rows[i].icon, r.x + 12, r.y + 14, th::ink2);
    bottom.text(font_body_b, r.x + 30, r.y + 6, rows[i].label, th::ink, 150);
    if (rows[i].kind == 0) {
      // a segmented switch: DE | EN
      Rect seg{r.x + r.w - 98, r.y + 4, 90, 20};
      bottom.roundRect(seg.x, seg.y, seg.w, seg.h, 10, th::well);
      bool en = s.lang == "en";
      Rect on{seg.x + (en ? 45 : 2), seg.y + 2, 43, 16};
      bottom.roundRectVGradient(on.x, on.y, on.w, on.h, 8, th::playHi, th::play);
      bottom.text(font_label, seg.x, seg.y + 4, "DEUTSCH", !en ? th::ink : th::ink3, 45, ALIGN_CENTER);
      bottom.text(font_label, seg.x + 45, seg.y + 4, "ENGLISH", en ? th::ink : th::ink3, 45, ALIGN_CENTER);
      hits_.push_back({{seg.x, r.y, 45, r.h}, SH_LANG, 0});
      hits_.push_back({{seg.x + 45, r.y, 45, r.h}, SH_LANG, 1});
    } else if (rows[i].kind == 1) drawSwitch(bottom, r.x + r.w - 38, r.y + 6, rows[i].on);
    else drawIcon(bottom, "chevron", r.x + r.w - 16, r.y + 14, th::ink3);
    hits_.insert(hits_.begin(), {r, SH_ROW, i});  // below the language halves
    y += 31;
  }
  drawActionBar(bottom, 199);
  Rect back{10, 205, 110, 30};
  drawButton(bottom, back, tr("Men\xc3\xbc", "Menu"), BtnStyle::Ghost, false, "back");
  hits_.push_back({back, SH_BACK, 0});
  bottom.text(font_small, 132, 212, tr("Gespeichert in sdmc:/souldew", "Saved in sdmc:/souldew"), th::ink4, 178, ALIGN_RIGHT);
}

SettingsAction SettingsScreen::handle(const Input &in, RunStore &store) {
  if (in.touchDown) downX_ = in.tx, downY_ = in.ty;
  int id = 0, arg = 0;
  if (const Hit *h = tapped(hits_, in, downX_, downY_)) id = h->id, arg = h->arg;
  int old = focus;
  if (in.pressed & K_DOWN) focus = std::min(4, focus + 1);
  if (in.pressed & K_UP) focus = std::max(0, focus - 1);
  if (in.pressed & K_B) return SettingsAction::Back;
  if ((in.pressed & (K_LEFT | K_RIGHT)) && focus == 0) id = SH_LANG, arg = (in.pressed & K_LEFT) ? 0 : 1;
  else if ((in.pressed & (K_LEFT | K_RIGHT)) && (focus == 1 || focus == 2)) id = SH_ROW, arg = focus;
  if (in.pressed & K_A) {
    id = SH_ROW;
    arg = focus;
    if (focus == 0) id = SH_LANG, arg = store.settings.lang == "en" ? 0 : 1;
  }
  switch (id) {
    case SH_BACK: return SettingsAction::Back;
    case SH_LANG: {
      focus = 0;
      std::string want = arg ? "en" : "de";
      if (store.settings.lang == want) return SettingsAction::Changed;
      store.settings.lang = want;
      g_lang = langOf(store.settings);
      return SettingsAction::Language;
    }
    case SH_ROW:
      focus = arg;
      if (arg == 0) return SettingsAction::Changed;
      if (arg == 1) store.settings.overlays = !store.settings.overlays;
      if (arg == 2) store.settings.tab = !store.settings.tab;
      if (arg == 3) return SettingsAction::Intro;
      return SettingsAction::Changed;
  }
  return focus != old ? SettingsAction::Changed : SettingsAction::None;
}

// ---- message page --------------------------------------------------------------------------------------------------

void drawMessage(Canvas &top, Canvas &bottom, const std::string &title, const std::string &body, const std::string &hint,
                 Tone tone, const char *buttonA, const char *buttonB, std::vector<Hit> *hits) {
  drawBackdrop(top, Backdrop::Hero);
  drawLogo(top, 12, 11);
  Color acc = toneColor(tone);
  Rect card{14, 50, 372, 160};
  top.shadow(card.x, card.y, card.w, card.h, 14, 6, 3, 130);
  top.roundRect(card.x, card.y, card.w, card.h, 14, mix(th::edge, acc, 0.35f));
  top.roundRectVGradient(card.x + 1, card.y + 1, card.w - 2, card.h - 2, 13, mix(rgb(0x2b2a33), acc, 0.12f), rgb(0x222128));
  top.hline(card.x + 14, card.y + 1, card.w - 28, Color(255, 255, 255, 24));
  top.circle(card.x + 34, card.y + 34, 16, mix(th::well, acc, 0.22f));
  top.roundRectStroke(card.x + 18, card.y + 18, 32, 32, 16, 1, mix(th::well, acc, 0.6f));
  const char *ic = tone == Tone::Ok ? "play" : tone == Tone::Danger ? "cross" : tone == Tone::Warn ? "warn" : "info";
  drawIcon(top, ic, card.x + 34 - iconWidth(ic) / 2 + (tone == Tone::Ok ? 1 : 0), card.y + 34, toneInk(tone));
  top.text(font_display, card.x + 62, card.y + 22, title, toneInk(tone), card.w - 76);
  top.textWrapped(font_body, card.x + 20, card.y + 64, card.w - 40, body, th::ink2, 2, 5);
  if (!hint.empty()) top.text(font_small, 14, 220, hint, th::ink3, 372);
  // the bottom screen: a quiet LCD (like the desktop app's play view) with the rings
  drawBackdrop(bottom, Backdrop::Plain);
  bottom.roundRect(10, 10, 300, 180, 14, rgb(0x050407));
  bottom.roundRectVGradient(13, 13, 294, 174, 11, rgb(0x1b2b20), rgb(0x080d0a));
  bottom.glow(160, 96, 150, 90, rgb(0x22362a, 160));
  drawLogoMark(bottom, 160 - 48, 100 - 24, 3, 1.f, true);
  for (int y = 14; y < 186; y += 2) bottom.hline(14, y, 292, Color(0, 0, 0, 36));
  bottom.text(font_label, 10, 150, "SOULDEW", th::lcd3, 300, ALIGN_CENTER);
  if (hits) hits->clear();
  drawActionBar(bottom, 199);
  if (buttonA && buttonB) {
    Rect a{10, 205, 146, 30}, b{164, 205, 146, 30};
    drawButton(bottom, a, buttonA, BtnStyle::Primary);
    drawButton(bottom, b, buttonB, BtnStyle::Ghost);
    if (hits) {
      hits->push_back({a, 1, 0});
      hits->push_back({b, 2, 0});
    }
  } else if (buttonA) {
    Rect a{10, 205, 300, 30};
    drawButton(bottom, a, buttonA, BtnStyle::Primary);
    if (hits) hits->push_back({a, 1, 0});
  }
}

// ---- overlay layer -------------------------------------------------------------------------------------------------

Event trophyEvent(const TrophyDef &d) {
  Event e{EventKind::Trophy, Tone::Ok, trophyName(d),
          fmt(tr("Erfolg freigeschaltet \xc2\xb7 %s \xc2\xb7 +%d", "Achievement unlocked \xc2\xb7 %s \xc2\xb7 +%d"), rarityLabel(d.rarity), trophyPoints(d.rarity))};
  e.rarity = (int)d.rarity;
  e.cat = (int)d.cat;
  return e;
}

void OverlayLayer::push(const Event &e, double now) {
  // the newest moment matters most: what is waiting and not important is dropped, the card on screen makes room
  // quickly (only warnings and deaths wait their turn) – so a card never trails the game by several seconds
  double dur = e.tone == Tone::Danger ? 5.0 : e.tone == Tone::Muted ? 2.6 : e.kind == EventKind::Trophy ? 4.2 : 3.6;
  auto important = [](const Event &x) {
    switch (x.kind) {
      case EventKind::NewArea: case EventKind::AreaUsed: case EventKind::Info: case EventKind::Gift: case EventKind::Missed:
      case EventKind::Evolved:
        return x.tone == Tone::Danger || x.tone == Tone::Warn;
      default: return true;  // encounters, catches, deaths, caps, badges, achievements, the end of a run
    }
  };
  for (size_t i = queue_.size(); i-- > 0;) {
    Item &it = queue_[i];
    if (it.start > now && !important(it.e)) queue_.erase(queue_.begin() + i);
  }
  double start = now;
  if (!queue_.empty()) {
    Item &last = queue_.back();
    if (last.start <= now) {
      double shown = now - last.start;
      double keep = last.e.tone == Tone::Danger ? 2.5 : important(last.e) ? 2.0 : 1.2;  // let it be read a moment
      if (last.start + last.dur > now + 0.3) last.dur = std::max(shown + 0.3, std::min(last.dur, keep));
    }
    start = std::max(now, last.start + last.dur);
  }
  queue_.push_back({e, start, dur});
  while (queue_.size() > 6) queue_.pop_front();
}

bool OverlayLayer::active(double now) const { return !badge_.empty() || (!queue_.empty() && queue_.front().start <= now); }

Rect OverlayLayer::drawTop(Canvas &c, double now, bool *redrawn) {
  while (!queue_.empty() && now > queue_.front().start + queue_.front().dur) queue_.pop_front();
  const Item *it = (!queue_.empty() && queue_.front().start <= now) ? &queue_.front() : nullptr;
  float a = 1;
  if (it) {
    double t = now - it->start;
    if (t < 0.22) a = (float)(t / 0.22);
    else if (t > it->dur - 0.25) a = (float)std::max(0.0, (it->dur - t) / 0.25);
    a = a * a * (3 - 2 * a);  // smoothstep
  }
  // what the picture shows: the card (and its fade step) and the badge
  std::string key = it ? fmt("%p/%.3f/%d", (const void *)it, it->start, (int)(a * 24)) : std::string("-");
  key += "|" + badge_;
  if (redrawn) *redrawn = false;
  if (key == lastKey_) return lastRect_;
  if (lastRect_.w) {
    Rect old = c.clip();
    c.setClip(lastRect_);
    c.clear(Color());
    c.setClip(old);
  }
  Rect dirty{0, 0, 0, 0};
  if (it) {
    int w = badge_.empty() ? 300 : 252;  // room for the battle badge on the right
    drawToast(c, 8, 8, w, it->e, a);
    dirty = {2, 0, w + 14, 62};
  }
  if (!badge_.empty()) {
    drawBattleBadge(c, 392, 10, badge_, badgeTone_);
    Rect b{246, 4, 152, 28};
    if (!dirty.w) dirty = b;
    else dirty = {std::min(dirty.x, b.x), 0, std::max(dirty.x + dirty.w, b.x + b.w) - std::min(dirty.x, b.x), 62};
  }
  // the plugin also has to repaint where the last picture was (the game redraws under it every frame anyway)
  lastKey_ = key;
  lastRect_ = dirty;
  if (redrawn) *redrawn = true;
  return dirty;
}

Rect OverlayLayer::drawBottom(Canvas &c, bool tabVisible, bool tabPressed) {
  if (!tabVisible) return {0, 0, 0, 0};
  Rect r = tabRect();
  drawCornerTab(c, r.x, r.y, tabPressed);
  return r;
}

}  // namespace sd
