// Renders souldew-3ds' screens on a computer with the very drawing code the console runs (common/), into PNGs:
// the launcher's screens and the in-game overlays / souldew screens composited onto a placeholder game picture
// through the same framebuffer blitter the plugin uses (BGR8 / RGB565 column-major layouts).
//
//   render <out dir> [de|en] [sample runs.json to write]
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../common/fbblit.h"
#include "../common/i18n.h"
#include "../common/screens.h"
#include "png.h"

using namespace sd;

static const char *OUT = "docs/screenshots";

// a 3DS-like composite: top screen above, bottom screen centred below, dark bezel
static void save3ds(const std::string &name, const Canvas &top, const Canvas &bottom) {
  const int W = 432, H = 520;
  Canvas img(W, H);
  img.clear(rgb(0x0c0b0e));
  img.roundRect(4, 4, W - 8, 252, 14, rgb(0x16151a));
  img.roundRect(4, 264, W - 8, 252, 14, rgb(0x16151a));
  img.draw(top, 16, 10);
  img.draw(bottom, 56, 270);
  std::vector<uint8_t> rgbBuf((size_t)W * H * 3);
  for (int i = 0; i < W * H; i++) {
    uint32_t p = img.pixels()[i];
    rgbBuf[i * 3] = (p >> 16) & 0xff;
    rgbBuf[i * 3 + 1] = (p >> 8) & 0xff;
    rgbBuf[i * 3 + 2] = p & 0xff;
  }
  std::string path = std::string(OUT) + "/" + name + ".png";
  if (!png::write(path, W, H, rgbBuf)) std::fprintf(stderr, "cannot write %s\n", path.c_str());
  else std::printf("%s\n", path.c_str());
}

static int locId(const char *en) {
  for (int i = 0; i < 400; i++) {
    const char *n = locationName(6, i, Lang::En);
    if (n && !std::strcmp(n, en)) return i;
  }
  return 0;
}

static RunMon mk(const char *key, int species, const char *nick, int level, int hp, int maxHp, const char *area, int slot) {
  RunMon m;
  m.key = key;
  m.species = species;
  m.nickname = nick;
  m.level = level;
  m.metLevel = level / 2 + 1;
  m.hp = hp;
  m.maxHp = maxHp;
  m.area = area;
  m.loc = locId(area);
  m.inParty = slot >= 0;
  m.partySlot = slot;
  return m;
}

static Encounter enc(const char *area, int species, int level, const char *status, const char *key = "") {
  Encounter e;
  e.area = area;
  e.loc = locId(area);
  e.species = species;
  e.level = level;
  e.status = status;
  e.monKey = key;
  return e;
}

static RunStore sampleStore() {
  RunStore st;
  Rules r;
  r.applyPreset("classic");
  Run &run = st.create("Kims Nuzlocke", "xy", r, 1700000000);
  run.badges = 3;
  run.lastArea = "Route 7";
  run.mons = {
      mk("a1", 657, "Quaxi", 31, 71, 88, "Aquacorde Town", 0),  // Frogadier
      mk("a2", 662, "", 30, 52, 79, "Route 2", 1),               // Fletchinder
      mk("a3", 667, "Leo", 29, 18, 77, "Route 22", 2),           // Litleo
      mk("a4", 674, "", 27, 66, 70, "Route 5", 3),               // Pancham
      mk("a5", 659, "Buddel", 26, 61, 61, "Route 3", 4),         // Bunnelby
      mk("a6", 682, "", 24, 0, 0, "Route 7", -1),                // Spritzee (box)
      mk("d1", 664, "Flatter", 9, 0, 29, "Santalune Forest", -1), // Scatterbug
      mk("d2", 16, "", 14, 0, 38, "Route 4", -1),                 // Pidgey
  };
  run.mons[0].metLevel = 5;
  for (auto &m : run.mons) m.maxHp = m.maxHp ? m.maxHp : 64;
  run.mons[5].hp = 64;
  run.mons[6].dead = true;
  run.mons[6].cause = "Pikachu Lv 6";
  run.mons[6].deathArea = "Santalune Forest";
  run.mons[6].deathTs = 1700001000;
  run.mons[7].dead = true;
  run.mons[7].cause = "Trainerkampf \xc2\xb7 Grant: Tyrunt Lv 25";
  run.mons[7].deathTs = 1700002000;
  run.encounters = {
      enc("Starter", 656, 5, "caught", "a1"),
      enc("Route 2", 661, 3, "caught", "a2"),
      enc("Santalune Forest", 664, 4, "caught", "d1"),
      enc("Route 3", 659, 6, "caught", "a5"),
      enc("Route 4", 16, 7, "caught", "d2"),
      enc("Route 22", 667, 7, "caught", "a3"),
      enc("Route 5", 674, 9, "caught", "a4"),
      enc("Route 6", 543, 10, "missed"),
      enc("Route 7", 682, 12, "caught", "a6"),
      enc("Connecting Cave", 41, 14, "open"),
  };
  run.encounters[0].starter = true;
  run.log.push_back({0, "caught", "Gefangen: Parfi (Route 7)"});
  std::string firstId = run.id;
  Rules h;
  h.applyPreset("hardcore");
  Run &r2 = st.create("Rubin Hardcore", "oras", h, 1700100000);
  r2.badges = 6;
  r2.encounters = {enc("Route 101", 263, 2, "caught", "x"), enc("Route 102", 280, 4, "missed")};
  r2.mons = {mk("x", 263, "", 12, 0, 30, "Route 101", -1)};
  r2.mons[0].dead = true;
  r2.status = "lost";
  Run &r3 = st.create("Sonne Solo", "sm", r, 1700200000);
  r3.milestone = 2;
  r3.encounters = {enc("Route 1", 731, 3, "caught", "y")};
  r3.mons = {mk("y", 731, "Picki", 20, 50, 50, "Route 1", 0)};
  st.active = firstId;  // (create() moved the runs: `run` is stale here)
  return st;
}

static std::vector<InstalledGame> sampleInstalled() {
  std::vector<InstalledGame> v;
  InstalledGame a;
  a.game = gameByKey("xy");
  a.version = 1;
  a.titleId = a.game->titleIds[1];
  v.push_back(a);
  InstalledGame b;
  b.game = gameByKey("sm");
  b.version = 0;
  b.cartridge = true;
  b.titleId = b.game->titleIds[0];
  v.push_back(b);
  return v;
}

// a placeholder "game picture" (no game graphics in this repository): sky, hills, a path, grass – only to show where
// the overlays sit on a busy picture
static void fakeGameTop(const FbTarget &t, bool battle) {
  Canvas c(400, 240);
  for (int y = 0; y < 240; y++) {
    Color col = battle ? mix(rgb(0x9fc6e8), rgb(0xd9eef7), y / 140.f) : mix(rgb(0x7fb5e0), rgb(0xcfe7f3), y / 120.f);
    c.fill(0, y, 400, 1, col);
  }
  if (battle) {
    c.fill(0, 150, 400, 90, rgb(0x8cbf6a));
    c.roundRect(40, 168, 140, 34, 17, rgb(0x6fa452));
    c.roundRect(230, 92, 130, 30, 15, rgb(0x6fa452));
    c.roundRect(268, 46, 56, 56, 18, rgb(0xd0b26a));  // the foe (a blob)
    c.roundRect(78, 116, 64, 64, 20, rgb(0x5e88c4));  // ours (a blob)
    c.roundRect(10, 196, 170, 34, 8, Color(250, 250, 250, 230));
    c.text(font_body_b, 18, 204, "Lv 31   71/88", rgb(0x333333));
  } else {
    c.circle(90, 190, 120, rgb(0x7cae5e));
    c.circle(320, 200, 140, rgb(0x6c9f50));
    c.fill(0, 168, 400, 72, rgb(0x86b864));
    for (int y = 172; y < 240; y += 8)
      for (int x = (y / 8 % 2) * 8; x < 400; x += 16) c.fill(x, y, 6, 3, rgb(0x76a855));
    c.roundRect(170, 150, 60, 90, 8, rgb(0xd8c08a));
    c.roundRect(188, 128, 24, 34, 8, rgb(0xd25a4a));  // the player (a figure)
    c.circle(200, 124, 9, rgb(0xf0d0b0));
  }
  c.text(font_label, 6, 228, "PLATZHALTER \xe2\x80\x93 KEIN SPIELBILD", Color(0, 0, 0, 120));
  blit(c, {0, 0, 400, 240}, t, 0, 0, false);
}

static void fakeGameBottom(const FbTarget &t) {
  Canvas c(320, 240);
  c.clear(rgb(0x3a6fb0));
  for (int y = 0; y < 240; y += 20) c.fill(0, y, 320, 1, rgb(0x447bbd));
  c.roundRect(20, 30, 280, 140, 14, rgb(0x2d5a92));
  c.text(font_body_b, 20, 92, "(Touchscreen des Spiels)", rgb(0xbcd4f0), 280, ALIGN_CENTER);
  blit(c, {0, 0, 320, 240}, t, 0, 0, false);
}

// a 3DS framebuffer in memory (column-major like the real one) and back to a canvas
struct FakeFb {
  std::vector<uint8_t> mem;
  FbTarget t;
  FakeFb(int w, PixFmt f) : mem((size_t)w * 240 * bytesPerPixel(f)) { t = ctruTarget(mem.data(), w, f); }
  void toCanvas(Canvas &c) const {
    for (int y = 0; y < 240; y++)
      for (int x = 0; x < t.width; x++) c.fill(x, y, 1, 1, readPixel(t, x, y));
  }
};

static void overlayShot(const char *name, const std::vector<Event> &events, double at, const std::string &badge, Tone badgeTone,
                        bool battle, PixFmt fmt) {
  FakeFb top(400, fmt), bot(320, fmt);
  fakeGameTop(top.t, battle);
  fakeGameBottom(bot.t);
  OverlayLayer layer;
  for (auto &e : events) layer.push(e, 0);
  layer.setBadge(badge, badgeTone);
  Canvas ov(400, 240), ovb(320, 240);
  Rect d = layer.drawTop(ov, at);
  if (d.w) blit(ov, d, top.t, d.x, d.y, true);  // exactly what the plugin's OSD callback does
  Rect db = layer.drawBottom(ovb, true, false);
  if (db.w) blit(ovb, db, bot.t, db.x, db.y, true);
  Canvas a(400, 240), b(320, 240);
  top.toCanvas(a);
  bot.toCanvas(b);
  save3ds(name, a, b);
}

int main(int argc, char **argv) {
  if (argc > 1) OUT = argv[1];
  if (argc > 2 && !std::strcmp(argv[2], "en")) g_lang = Lang::En;
  std::string sfx = g_lang == Lang::En ? "_en" : "";
  RunStore store = sampleStore();
  if (argc > 3) store.save(argv[3]);  // the sample run file for the emulator test (tools/emu_test.py launcher)
  std::vector<InstalledGame> inst = sampleInstalled();
  Canvas top(400, 240), bottom(320, 240);

  // launcher: home
  HomeScreen home;
  home.selected = 2;  // newest first: index 2 = the X/Y run
  home.draw(top, bottom, store, inst, "");
  save3ds("launcher_home" + sfx, top, bottom);
  RunStore empty;
  HomeScreen home2;
  home2.draw(top, bottom, empty, inst, "");
  save3ds("launcher_welcome" + sfx, top, bottom);

  // launcher: new run
  NewRunWizard wiz;
  wiz.start(inst);
  wiz.draw(top, bottom, inst);
  save3ds("launcher_new_1_game" + sfx, top, bottom);
  wiz.step = 1;
  wiz.focus = 2;
  wiz.draw(top, bottom, inst);
  save3ds("launcher_new_2_rules" + sfx, top, bottom);
  wiz.step = 2;
  wiz.name = "Kims Nuzlocke";
  wiz.draw(top, bottom, inst);
  save3ds("launcher_new_3_name" + sfx, top, bottom);

  // launcher: a run
  Run *run = store.byId(store.active);
  ViewCtx ctx;
  ctx.run = run;
  ctx.game = gameByKey(run->game.c_str());
  RunView view;
  const char *tabs[] = {"team", "routes", "graves", "rules"};
  for (int t = 0; t < 4; t++) {
    view.tab = t;
    view.drawTop(top, ctx);
    view.drawBottom(bottom, ctx);
    save3ds(std::string("launcher_run_") + tabs[t] + sfx, top, bottom);
  }
  // launching message
  drawMessage(top, bottom, tr("Starte Pok\xc3\xa9mon Y \xe2\x80\xa6", "Starting Pok\xc3\xa9mon Y \xe2\x80\xa6"),
              tr("Das souldew-Plugin wird f\xc3\xbcr dieses Spiel geladen. Im Spiel: SELECT oder die Ecke unten rechts \xc3\xb6" "ffnet souldew.",
                 "The souldew plugin is loaded for this game. In game: SELECT or the bottom-right corner opens souldew."),
              "", Tone::Ok);
  save3ds("launcher_launching" + sfx, top, bottom);
  drawMessage(top, bottom, tr("Plugin-Loader ist aus", "Plugin loader is off"),
              tr("Ohne den Plugin-Loader von Luma3DS gibt es im Spiel keine Overlays. Einschalten? (Rosalina-Men\xc3\xbc \xe2\x80\xba Plugin Loader)",
                 "Without Luma3DS' plugin loader there are no overlays in game. Switch it on? (Rosalina menu \xe2\x80\xba Plugin loader)"),
              "", Tone::Warn, tr("Einschalten & starten", "Switch on & start"), tr("Ohne starten", "Start without"));
  save3ds("launcher_pluginloader_off" + sfx, top, bottom);

  // in game: overlays on the (placeholder) game picture
  int r7 = locId("Route 7"), r8 = locId("Route 8");
  (void)r7;
  Event newArea{EventKind::NewArea, Tone::Info, areaLabel(6, r8, "Route 8", g_lang), tr("Neues Gebiet \xc2\xb7 die erste Begegnung z\xc3\xa4hlt", "New area \xc2\xb7 the first encounter counts")};
  Event first{EventKind::FirstEncounter, Tone::Ok, fmt(tr("Erste Begegnung: %s", "First encounter: %s"), (std::string(speciesName(278, g_lang)) + " Lv 13").c_str()),
              fmt(tr("%s \xc2\xb7 das ist dein Fang", "%s \xc2\xb7 this is your catch"), areaLabel(6, r8, "Route 8", g_lang).c_str()), 278};
  Event dupe{EventKind::DupeEncounter, Tone::Warn, fmt(tr("Dupe: %s", "Dupe: %s"), (std::string(speciesName(661, g_lang)) + " Lv 12").c_str()),
             tr("Schon im Lauf \xc2\xb7 z\xc3\xa4hlt nicht, n\xc3\xa4" "chste Begegnung z\xc3\xa4hlt", "Already in the run \xc2\xb7 doesn't count, the next one does"), 661};
  Event death{EventKind::Death, Tone::Danger, fmt(tr("%s ist gefallen", "%s has fainted"), "Leo"),
              fmt(tr("gegen %s \xc2\xb7 in die Box", "vs %s \xc2\xb7 box it"), (std::string(speciesName(698, g_lang)) + " Lv 25").c_str()), 667};
  Event cap{EventKind::OverCap, Tone::Warn, fmt(tr("%s Lv %d \xc3\xbc" "ber dem Cap", "%s Lv %d is over the cap"), "Quaxi", 33),
            fmt(tr("Level-Cap %d (%s)%s", "Level cap %d (%s)%s"), 32, "Korrina", ""), 657};
  Event caught{EventKind::Caught, Tone::Ok, fmt(tr("Gefangen: %s", "Caught: %s"), speciesName(278, g_lang)), areaLabel(6, r8, "Route 8", g_lang), 278};
  overlayShot(("ingame_new_area" + sfx).c_str(), {newArea}, 1.0, "", Tone::Info, false, PixFmt::BGR8);
  overlayShot(("ingame_first_encounter" + sfx).c_str(), {first}, 1.2, tr("ERSTE BEGEGNUNG", "FIRST ENCOUNTER"), Tone::Ok, true, PixFmt::RGB565);
  overlayShot(("ingame_dupe" + sfx).c_str(), {dupe}, 1.2, "DUPE", Tone::Warn, true, PixFmt::BGR8);
  overlayShot(("ingame_death" + sfx).c_str(), {death}, 1.2, "", Tone::Info, true, PixFmt::RGB565);
  overlayShot(("ingame_level_cap" + sfx).c_str(), {cap}, 1.2, "CAP 32", Tone::Warn, true, PixFmt::BGR8);
  overlayShot(("ingame_caught" + sfx).c_str(), {caught}, 1.0, "", Tone::Info, false, PixFmt::BGR8);
  overlayShot(("ingame_quiet" + sfx).c_str(), {}, 0, "", Tone::Info, false, PixFmt::BGR8);
  // the fade-in, frame by frame
  overlayShot(("ingame_toast_fadein" + sfx).c_str(), {newArea}, 0.11, "", Tone::Info, false, PixFmt::BGR8);

  // in game: souldew's screens over the paused game (top: dimmed game + header + team; bottom: the tabs)
  {
    ctx.inGame = true;
    std::vector<Mon> party;
    for (auto &m : run->mons)
      if (m.inParty) {
        Mon x;
        x.key = m.key;
        x.species = m.species;
        x.nickname = m.nickname;
        x.level = m.level;
        x.hp = m.hp;
        x.maxHp = m.maxHp;
        party.push_back(x);
      }
    party[2].status = 0x10;  // burned
    ctx.party = &party;
    ctx.area = "Route 7";
    ctx.loc = locId("Route 7");
    for (int t = 0; t < 4; t++) {
      view.tab = t;
      view.drawTop(top, ctx);
      view.drawBottom(bottom, ctx);
      save3ds(std::string("ingame_souldew_") + tabs[t] + sfx, top, bottom);
    }
    // an open encounter being confirmed (party was full)
    view.tab = 1;
    Input in;
    view.drawBottom(bottom, ctx);
    in.touchDown = true;
    in.tx = 150;
    in.ty = 36;
    view.handle(in, ctx, 0);
    in = Input();
    in.touchUp = true;
    in.tx = 150;
    in.ty = 36;
    view.handle(in, ctx, 0);
    view.drawTop(top, ctx);
    view.drawBottom(bottom, ctx);
    save3ds("ingame_souldew_confirm" + sfx, top, bottom);
  }
  return 0;
}
