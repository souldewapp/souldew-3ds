#include "screens.h"

#include <algorithm>
#include <cstdlib>
#include "i18n.h"

namespace sd {

// ---- home ----------------------------------------------------------------------------------------------------------

enum { HH_RUN = 1, HH_CONTINUE, HH_NEW };

std::string HomeScreen::selectedId(const RunStore &store) const {
  if (selected >= 0 && selected < (int)store.runs.size()) return store.runs[store.runs.size() - 1 - selected].id;
  return "";
}

static const InstalledGame *findInstalled(const std::vector<InstalledGame> &inst, const std::string &key) {
  for (auto &i : inst)
    if (i.game && key == i.game->key) return &i;
  return nullptr;
}

void HomeScreen::draw(Canvas &top, Canvas &bottom, RunStore &store, const std::vector<InstalledGame> &installed,
                      const std::string &status) {
  hits_.clear();
  // ---- top: logo, the selected run, its numbers
  top.clear(th::shell);
  drawLogo(top, 12, 10);
  {
    std::string right = installed.empty() ? tr("kein Spiel gefunden", "no game found")
                                          : fmt(tr("%d Spiel%s gefunden", "%d game%s found"), (int)installed.size(),
                                                installed.size() == 1 ? "" : tr("e", "s"));
    int w = chipWidth(right);
    drawChip(top, 388 - w, 16, right, installed.empty() ? th::warnInk : th::ink2);
  }
  Run *run = nullptr;
  if (!store.runs.empty()) {
    selected = std::max(0, std::min(selected, (int)store.runs.size() - 1));
    run = store.byId(selectedId(store));
  }
  if (!run) {
    top.text(font_display, 14, 50, tr("Willkommen", "Welcome"), th::ink);
    top.textWrapped(font_body, 14, 82, 372,
                    tr("souldew begleitet deinen Nuzlocke direkt auf dem 3DS: Lauf anlegen, Spiel starten \xe2\x80\x93 "
                       "souldew merkt sich Begegnungen, Tode und Level-Caps und meldet sich nur in den wichtigen Momenten.",
                       "souldew follows your Nuzlocke right on the 3DS: create a run, start the game \xe2\x80\x93 souldew "
                       "keeps track of encounters, deaths and level caps and only speaks up when it matters."),
                    th::ink2, 2);
    top.card(14, 150, 372, 58, 10, th::panel, th::edge, th::crs, 4);
    top.text(font_body_b, 28, 158, tr("So geht's", "How it works"), th::crsInk);
    top.text(font_small, 28, 176, tr("1  Neuer Lauf  \xc2\xb7  2  Spiel starten  \xc2\xb7  3  im Spiel: SELECT \xc3\xb6" "ffnet souldew",
                                     "1  New run  \xc2\xb7  2  Start the game  \xc2\xb7  3  in game: SELECT opens souldew"),
             th::ink2, 350);
    top.text(font_small, 28, 191, tr("Luma3DS mit Plugin-Loader n\xc3\xb6tig (siehe README).", "Needs Luma3DS with the plugin loader (see README)."), th::ink3, 350);
  } else {
    const GameInfo *g = gameByKey(run->game.c_str());
    top.text(font_display, 14, 44, run->name, th::ink, 372);
    Color acc = gameColor(run->game);
    int cy = 76;
    top.roundRect(12, cy, 376, 128, 12, th::edge);
    top.roundRectGradient(13, cy + 1, 374, 126, 11, mix(rgb(0x26252d), acc, 0.13f), rgb(0x24232a));
    top.roundRect(13, cy + 1, 4, 126, 2, acc);
    top.text(font_head, 28, cy + 8, g ? g->name : run->game, mix(acc, th::ink, 0.45f), 250);
    std::string st = run->status == "lost" ? tr("VERLOREN", "LOST") : run->status == "won" ? tr("GEWONNEN", "WON") : tr("AKTIV", "ACTIVE");
    drawChip(top, 376 - chipWidth(st), cy + 10, st, run->status == "lost" ? th::badInk : run->status == "won" ? th::warnInk : th::okInk);
    std::string boss;
    int cap = run->levelCap(&boss);
    std::string sub = "Nuzlocke";
    if (g) sub += g->caps.fromBadges ? fmt(tr(" \xc2\xb7 %d Orden", " \xc2\xb7 %d badges"), run->badges)
                                     : fmt(tr(" \xc2\xb7 %d/4 Pr\xc3\xbc" "fungen", " \xc2\xb7 %d/4 trials"), run->milestone);
    if (cap) sub += fmt(" \xc2\xb7 Cap %d", cap);
    const InstalledGame *ig = findInstalled(installed, run->game);
    sub += ig ? (ig->cartridge ? tr(" \xc2\xb7 Karte", " \xc2\xb7 cartridge") : tr(" \xc2\xb7 installiert", " \xc2\xb7 installed")) : tr(" \xc2\xb7 Spiel fehlt", " \xc2\xb7 game missing");
    top.text(font_small, 28, cy + 28, sub, th::ink2, 350);
    int tw = 66, gap = 5, tx = 28;
    drawStatTile(top, tx, cy + 50, tw, 52, run->caught(), tr("gefangen", "caught"), th::ink); tx += tw + gap;
    drawStatTile(top, tx, cy + 50, tw, 52, run->missed(), tr("verpasst", "missed"), th::ink); tx += tw + gap;
    drawStatTile(top, tx, cy + 50, tw, 52, run->open(), tr("offen", "open"), th::warn); tx += tw + gap;
    drawStatTile(top, tx, cy + 50, tw, 52, run->alive(), tr("lebend", "alive"), th::okInk); tx += tw + gap;
    drawStatTile(top, tx, cy + 50, tw, 52, run->dead(), tr("tot", "dead"), th::badInk);
    if (!run->log.empty()) top.text(font_small, 28, cy + 108, run->log.back().text, th::ink3, 350);
  }
  int hx = 14, w;
  drawKeyHint(top, hx, 220, "A", tr("\xc3\xb6" "ffnen", "open"), &w); hx += w + 12;
  drawKeyHint(top, hx, 220, "X", tr("weiterspielen", "continue"), &w); hx += w + 12;
  drawKeyHint(top, hx, 220, "Y", tr("neuer Lauf", "new run"), &w); hx += w + 12;
  drawKeyHint(top, hx, 220, "START", tr("beenden", "quit"), &w);
  if (!status.empty()) top.text(font_small, 200, 219, status, th::warnInk, 188, ALIGN_RIGHT);

  // ---- bottom: all runs + the two big buttons
  bottom.clear(th::shell);
  bottom.text(font_head, 12, 6, tr("Alle L\xc3\xa4ufe", "All runs"), th::ink);
  bottom.text(font_small, 0, 9, fmt(tr("%d insgesamt", "%d in total"), (int)store.runs.size()), th::ink3, 308, ALIGN_RIGHT);
  Rect list{0, 28, 320, 166};
  bottom.setClip(list);
  const int RH = 48;
  int n = (int)store.runs.size();
  if (!n) {
    bottom.textWrapped(font_body, 24, 70, 272, tr("Noch kein Lauf. Tippe auf \xe2\x80\x9eNeuer Lauf\xe2\x80\x9c.", "No run yet. Tap \xe2\x80\x9cNew run\xe2\x80\x9d."), th::ink3);
  }
  int maxScroll = std::max(0, n * RH - list.h + 4);
  scroll_ = std::max(0, std::min(scroll_, maxScroll));
  for (int i = 0; i < n; i++) {
    const Run &r = store.runs[n - 1 - i];  // newest first
    int y = list.y + 2 + i * RH - scroll_;
    if (y + RH < list.y || y > list.y + list.h) continue;
    bool sel = i == selected;
    const GameInfo *g = gameByKey(r.game.c_str());
    Color acc = gameColor(r.game);
    bottom.card(10, y, 300, RH - 6, 10, sel ? th::shell3 : th::shell2, sel ? th::edgeHi : th::shell4);
    bottom.roundRect(11, y + 1, 298, 3, 1, acc.alpha(sel ? 230 : 140));
    bottom.text(font_body_b, 22, y + 6, r.name, th::ink, 190);
    std::string sub = std::string(g ? g->shortName : r.game.c_str()) + (g && g->caps.fromBadges ? fmt(tr(" \xc2\xb7 %d Orden", " \xc2\xb7 %d badges"), r.badges) : "") +
                      fmt(tr(" \xc2\xb7 %d gefangen \xc2\xb7 %d tot", " \xc2\xb7 %d caught \xc2\xb7 %d dead"), r.caught(), r.dead());
    bottom.text(font_small, 22, y + 23, sub, th::ink3, 200);
    if (r.id == store.active && r.status == "active") drawChip(bottom, 300 - chipWidth(tr("AKTIV", "ACTIVE")), y + 15, tr("AKTIV", "ACTIVE"), th::okInk);
    else if (r.status != "active") {
      std::string st = r.status == "lost" ? tr("VERLOREN", "LOST") : tr("GEWONNEN", "WON");
      drawChip(bottom, 300 - chipWidth(st), y + 15, st, r.status == "lost" ? th::badInk : th::warnInk);
    }
    hits_.push_back({{10, std::max(y, list.y), 300, RH - 6}, HH_RUN, i});
  }
  bottom.resetClip();
  drawScrollHint(bottom, {list.x, list.y, list.w - 2, list.h}, scroll_, n * RH);
  bottom.fill(0, 196, 320, 44, th::shell);
  bottom.hline(0, 196, 320, th::shell3);
  Rect cont{10, 202, 170, 32}, nw{188, 202, 122, 32};
  drawButton(bottom, cont, tr("Weiterspielen", "Continue"), BtnStyle::Primary, false, "play");
  drawButton(bottom, nw, tr("Neuer Lauf", "New run"), BtnStyle::Accent, false, "plus");
  hits_.push_back({cont, HH_CONTINUE, 0});
  hits_.push_back({nw, HH_NEW, 0});
}

HomeAction HomeScreen::handle(const Input &in, RunStore &store) {
  int n = (int)store.runs.size();
  if (in.touchDown) {
    downX_ = in.tx;
    downY_ = in.ty;
    drag_ = false;
  }
  if (in.touchHeld && std::abs(in.ty - downY_) > 6 && downY_ > 28 && downY_ < 194) drag_ = true;
  if (in.touchHeld && drag_) {
    scroll_ -= in.scrollDy;
    return HomeAction::Changed;
  }
  if (in.touchUp && !drag_) {
    for (int i = (int)hits_.size() - 1; i >= 0; i--) {
      const Hit &h = hits_[i];
      if (!h.r.contains(in.tx, in.ty) || !h.r.contains(downX_, downY_)) continue;
      if (h.id == HH_RUN) {
        if (selected == h.arg) return HomeAction::OpenRun;
        selected = h.arg;
        return HomeAction::Changed;
      }
      if (h.id == HH_CONTINUE) return n ? HomeAction::Continue : HomeAction::NewRun;
      if (h.id == HH_NEW) return HomeAction::NewRun;
    }
  }
  if ((in.pressed & K_DOWN) && selected < n - 1) {
    selected++;
    if ((selected + 1) * 48 > scroll_ + 166) scroll_ = (selected + 1) * 48 - 166;
    return HomeAction::Changed;
  }
  if ((in.pressed & K_UP) && selected > 0) {
    selected--;
    if (selected * 48 < scroll_) scroll_ = selected * 48;
    return HomeAction::Changed;
  }
  if ((in.pressed & K_A) && n) return HomeAction::OpenRun;
  if (in.pressed & K_X) return n ? HomeAction::Continue : HomeAction::NewRun;
  if (in.pressed & K_Y) return HomeAction::NewRun;
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
    {"Blackout beendet Lauf", "Whiteout ends the run", "F\xc3\xa4llt das ganze Team, ist der Lauf vorbei.", "If the whole team faints, the run is over."},
    {"Geschenke z\xc3\xa4hlen", "Gifts count", "Geschenkte und statische Pok\xc3\xa9mon sind die Begegnung ihres Gebiets.",
     "Gifted and static Pok\xc3\xa9mon are the encounter of their area."},
    {"Deathless", "Deathless", "Der erste Tod beendet den Lauf.", "The first death ends the run."},
    {"Keine Items im Kampf", "No items in battle", "Hardcore: Heiltr\xc3\xa4nke nur au\xc3\x9f" "erhalb von K\xc3\xa4mpfen (Ehrensache).",
     "Hardcore: healing items only outside battles (honour system)."},
    {"Set-Modus", "Set mode", "Hardcore: kein Wechsel, wenn der Gegner ein Pok\xc3\xa9mon besiegt hat (Ehrensache).",
     "Hardcore: no switching after the opponent's Pok\xc3\xa9mon faints (honour system)."},
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
  top.clear(th::shell);
  drawLogo(top, 12, 10);
  const char *steps[] = {tr("Spiel", "Game"), tr("Regeln", "Rules"), tr("Name", "Name")};
  int sx = 230;
  for (int i = 0; i < 3; i++) {
    bool on = i == step, done = i < step;
    top.circle(sx + 7, 22, 7, on ? th::crs : done ? th::tri : th::shell4);
    top.text(font_label, sx, 18, std::to_string(i + 1), on || done ? th::shell : th::ink3, 14, ALIGN_CENTER);
    top.text(font_small, sx + 18, 15, steps[i], on ? th::ink : th::ink3);
    sx += 18 + Canvas::textWidth(font_small, steps[i]) + 10;
  }
  top.text(font_display, 14, 44, tr("Neuer Nuzlocke", "New Nuzlocke"), th::ink);
  const GameInfo *g = gameAt(gameIndex);
  bottom.clear(th::shell);
  if (step == 0) {
    top.textWrapped(font_body, 14, 76, 372, tr("W\xc3\xa4hle das Spiel. souldew sucht installierte Titel und die eingesteckte Karte.",
                                                "Pick the game. souldew looks for installed titles and the inserted cartridge."),
                    th::ink2);
    // details of the focused game
    top.card(14, 112, 372, 92, 12, th::panel, th::edge, gameColor(g->key), 4);
    top.text(font_head, 28, 120, g->name, th::ink, 340);
    const InstalledGame *ig = findInstalled(installed, g->key);
    std::string where = ig ? (ig->cartridge ? tr("Karte eingesteckt", "cartridge inserted") : tr("auf der SD-Karte installiert", "installed on the SD card"))
                           : tr("nicht gefunden \xe2\x80\x93 Lauf trotzdem m\xc3\xb6glich", "not found \xe2\x80\x93 you can still create the run");
    top.text(font_small, 28, 141, where, ig ? th::okInk : th::warnInk, 340);
    std::string live = fmt(tr("Generation %d \xc2\xb7 Live-Daten: Team, K\xc3\xa4mpfe, Gebiet%s", "Generation %d \xc2\xb7 live data: team, battles, area%s"),
                           g->gen, g->caps.fromBadges ? tr(", Orden", ", badges") : "");
    top.text(font_small, 28, 158, live, th::ink3, 340);
    std::string caps = fmt(tr("Level-Caps: %d \xe2\x80\x93 %d", "Level caps: %d \xe2\x80\x93 %d"), g->caps.caps[0], g->caps.finalCap);
    top.text(font_small, 28, 174, caps, th::ink3, 340);
    // the four games
    for (int i = 0; i < gameCount(); i++) {
      const GameInfo *gi = gameAt(i);
      int x = 10 + (i % 2) * 154, y = 10 + (i / 2) * 88;
      bool sel = i == gameIndex;
      Color acc = gameColor(gi->key);
      bottom.card(x, y, 146, 80, 12, sel ? th::shell3 : th::shell2, sel ? acc : th::shell4);
      bottom.roundRect(x + 1, y + 1, 144, 3, 1, acc.alpha(sel ? 255 : 120));
      bottom.text(font_head, x + 12, y + 12, gi->shortName, th::ink, 124);
      bottom.text(font_small, x + 12, y + 32, fmt("Gen %d", gi->gen), th::ink3);
      const InstalledGame *in2 = findInstalled(installed, gi->key);
      std::string chip = in2 ? (in2->cartridge ? tr("KARTE", "CART") : tr("INSTALLIERT", "INSTALLED")) : tr("FEHLT", "MISSING");
      drawChip(bottom, x + 12, y + 54, chip, in2 ? th::okInk : th::ink3);
      hits_.push_back({{x, y, 146, 80}, WH_GAME, i});
    }
    // version (X or Y …) when the game pair is focused: B/A below
  } else if (step == 1) {
    top.text(font_small, 14, 76, fmt(tr("%s \xc2\xb7 Regeln", "%s \xc2\xb7 rules"), g->name), th::ink2);
    int f = std::max(0, std::min(focus, RULE_COUNT));
    std::string help = f == 0 ? tr("Klassisch: Dupes-Klausel, Level-Caps als Warnung. Hardcore: strenge Caps, keine Items im Kampf, Set-Modus.",
                                   "Classic: dupes clause, level caps as a warning. Hardcore: strict caps, no items in battle, set mode.")
                              : tr(RULES[f - 1].helpDe, RULES[f - 1].helpEn);
    top.card(14, 96, 372, 84, 12, th::panel, th::edge, th::crs, 4);
    top.text(font_body_b, 28, 104, f == 0 ? tr("Voreinstellung", "Preset") : tr(RULES[f - 1].de, RULES[f - 1].en), th::crsInk, 340);
    top.textWrapped(font_small, 28, 124, 344, help, th::ink2, 2, 3);
    top.text(font_small, 14, 190, tr("Immer an: nur die erste Begegnung pro Gebiet, gefallen = tot.", "Always on: only the first encounter per area, fainted = dead."), th::ink3, 372);
    // presets
    std::string pn = rules.presetName();
    Rect pc{10, 8, 148, 30}, ph{162, 8, 148, 30};
    drawButton(bottom, pc, tr("Klassisch", "Classic"), pn == "classic" ? BtnStyle::Accent : BtnStyle::Ghost, focus == 0);
    drawButton(bottom, ph, "Hardcore", pn == "hardcore" ? BtnStyle::Danger : BtnStyle::Ghost, false);
    hits_.push_back({pc, WH_PRESET, 0});
    hits_.push_back({ph, WH_PRESET, 1});
    for (int i = 0; i < RULE_COUNT; i++) {
      int y = 44 + i * 21;
      bool foc = focus == i + 1;
      if (foc) bottom.roundRect(8, y - 1, 304, 20, 6, th::shell3);
      bottom.text(font_body, 16, y + 1, tr(RULES[i].de, RULES[i].en), th::ink, 180);
      std::string v = ruleState(rules, i);
      bool on = v != tr("aus", "off");
      drawChip(bottom, 304 - chipWidth(v), y + 3, v, on ? th::okInk : th::ink3);
      hits_.push_back({{8, y - 1, 304, 20}, WH_RULE, i});
    }
  } else {
    top.textWrapped(font_body, 14, 76, 372, tr("Gib dem Lauf einen Namen. Tippe auf das Feld, um ihn zu \xc3\xa4ndern.",
                                                "Name the run. Tap the field to change it."),
                    th::ink2);
    top.card(14, 112, 372, 92, 12, th::panel, th::edge, gameColor(g->key), 4);
    top.text(font_head, 28, 120, name, th::ink, 340);
    top.text(font_small, 28, 142, fmt("%s \xc2\xb7 %s", g->name, g->versionNames[version]), th::ink2, 340);
    std::string pn = rules.presetName();
    top.text(font_small, 28, 160, pn == "hardcore" ? "Nuzlocke \xc2\xb7 Hardcore" : pn == "classic" ? tr("Nuzlocke \xc2\xb7 Klassisch", "Nuzlocke \xc2\xb7 classic") : tr("Nuzlocke \xc2\xb7 eigene Regeln", "Nuzlocke \xc2\xb7 custom"), th::ink3);
    Rect nf{10, 20, 300, 44};
    bottom.card(nf.x, nf.y, nf.w, nf.h, 10, th::well, th::edgeHi);
    bottom.text(font_body_b, 22, 33, name, th::ink, 240);
    bottom.text(font_label, 0, 37, tr("\xc3\x84NDERN", "EDIT"), th::crsInk, 296, ALIGN_RIGHT);
    hits_.push_back({nf, WH_NAME, 0});
    // version choice (X / Y …)
    for (int v = 0; v < 2; v++) {
      Rect r{10 + v * 154, 80, 146, 36};
      drawButton(bottom, r, g->versionNames[v], v == version ? BtnStyle::Accent : BtnStyle::Ghost);
      hits_.push_back({r, WH_GAME, 100 + v});
    }
  }
  // navigation
  bottom.fill(0, 196, 320, 44, th::shell);
  bottom.hline(0, 196, 320, th::shell3);
  Rect back{10, 202, 110, 32}, next{128, 202, 182, 32};
  drawButton(bottom, back, step == 0 ? tr("Abbrechen", "Cancel") : tr("Zur\xc3\xbc" "ck", "Back"), BtnStyle::Ghost, false, "back");
  drawButton(bottom, next, step == 2 ? tr("Lauf anlegen", "Create run") : tr("Weiter", "Next"), step == 2 ? BtnStyle::Primary : BtnStyle::Accent, false, step == 2 ? "check" : nullptr);
  hits_.push_back({back, WH_BACK, 0});
  hits_.push_back({next, WH_NEXT, 0});
  int hx = 14, w;
  drawKeyHint(top, hx, 220, "A", step == 2 ? tr("anlegen", "create") : tr("weiter", "next"), &w); hx += w + 12;
  drawKeyHint(top, hx, 220, "B", tr("zur\xc3\xbc" "ck", "back"), &w); hx += w + 12;
  if (step == 1) drawKeyHint(top, hx, 220, "< >", tr("\xc3\xa4ndern", "change"), &w);
  if (step == 2) drawKeyHint(top, hx, 220, "Y", tr("Name", "name"), &w);
}

WizardAction NewRunWizard::handle(const Input &in, const std::vector<InstalledGame> &) {
  int id = 0, arg = 0;
  if (in.touchDown) {
    downX_ = in.tx;
    downY_ = in.ty;
  }
  if (in.touchUp)
    for (int i = (int)hits_.size() - 1; i >= 0; i--)
      if (hits_[i].r.contains(in.tx, in.ty) && hits_[i].r.contains(downX_, downY_)) {
        id = hits_[i].id;
        arg = hits_[i].arg;
        break;
      }
  if (in.pressed & K_A) id = (step == 1 && focus > 0) ? WH_RULE : WH_NEXT, arg = focus - 1;
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

// ---- message page --------------------------------------------------------------------------------------------------

void drawMessage(Canvas &top, Canvas &bottom, const std::string &title, const std::string &body, const std::string &hint,
                 Tone tone, const char *buttonA, const char *buttonB, std::vector<Hit> *hits) {
  top.clear(th::shell);
  drawLogo(top, 12, 10);
  top.card(14, 56, 372, 150, 14, th::panel, th::edge, toneColor(tone), 4);
  top.text(font_display, 30, 68, title, toneInk(tone), 340);
  top.textWrapped(font_body, 30, 102, 340, body, th::ink2, 2, 5);
  if (!hint.empty()) top.text(font_small, 14, 218, hint, th::ink3, 372);
  bottom.clear(th::shell);
  // a quiet LCD panel on the bottom screen (like the desktop app's play view)
  bottom.card(10, 10, 300, 180, 12, th::lcd0, th::lcd2);
  for (int y = 14; y < 188; y += 12) bottom.hline(14, y, 292, th::lcd1);
  for (int x = 14; x < 308; x += 12) bottom.vline(x, 14, 174, th::lcd1);
  bottom.text(font_label, 10, 94, "SOULDEW", th::lcd3, 300, ALIGN_CENTER);
  if (hits) hits->clear();
  if (buttonA && buttonB) {
    Rect a{10, 202, 146, 32}, b{164, 202, 146, 32};
    drawButton(bottom, a, buttonA, BtnStyle::Primary);
    drawButton(bottom, b, buttonB, BtnStyle::Ghost);
    if (hits) {
      hits->push_back({a, 1, 0});
      hits->push_back({b, 2, 0});
    }
  } else if (buttonA) {
    Rect a{10, 202, 300, 32};
    drawButton(bottom, a, buttonA, BtnStyle::Primary);
    if (hits) hits->push_back({a, 1, 0});
  }
}

// ---- overlay layer -------------------------------------------------------------------------------------------------

void OverlayLayer::push(const Event &e, double now) {
  // the newest moment matters most: what is waiting and not important is dropped, the card on screen makes room
  // quickly (only warnings and deaths wait their turn) – so a card never trails the game by several seconds
  double dur = e.tone == Tone::Danger ? 5.0 : e.tone == Tone::Muted ? 2.6 : 3.6;
  auto important = [](const Event &x) {
    switch (x.kind) {
      case EventKind::NewArea: case EventKind::AreaUsed: case EventKind::Info: case EventKind::Gift: case EventKind::Missed:
        return x.tone == Tone::Danger || x.tone == Tone::Warn;
      default: return true;  // encounters, catches, deaths, caps, badges, the end of a run
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
    dirty = {4, 0, w + 10, 60};
  }
  if (!badge_.empty()) {
    drawBattleBadge(c, 392, 10, badge_, badgeTone_);
    Rect b{250, 6, 146, 24};
    if (!dirty.w) dirty = b;
    else dirty = {std::min(dirty.x, b.x), 0, std::max(dirty.x + dirty.w, b.x + b.w) - std::min(dirty.x, b.x), 60};
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
