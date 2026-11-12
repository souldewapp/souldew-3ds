// souldew – the launcher (.3dsx, libctru): runs, rules, team/graveyard/routes, and "start the game" – it asks Luma3DS'
// plugin loader to load souldew's plugin for the chosen game and jumps to it. All drawing is the shared software
// canvas (common/), blitted into the 3DS framebuffers.
#include <3ds.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

#include "fbblit.h"
#include "i18n.h"
#include "plgldr.h"
#include "screens.h"

using namespace sd;

static const char *kPluginPath = "/3ds/souldew/souldew.3gx";

static RunStore g_store;
static std::vector<InstalledGame> g_installed;

static long long wallSec() { return (long long)(osGetTime() / 1000ull); }

static void saveStore() {
  mkdir("sdmc:/souldew", 0777);
  g_store.save(std::string("sdmc:") + kRunsPath);
}

// installed titles (SD card) and the inserted cartridge that souldew knows
static void scanTitles() {
  g_installed.clear();
  if (R_FAILED(amInit())) return;
  const FS_MediaType media[2] = {MEDIATYPE_SD, MEDIATYPE_GAME_CARD};
  for (int m = 0; m < 2; m++) {
    u32 count = 0;
    if (R_FAILED(AM_GetTitleCount(media[m], &count)) || !count) continue;
    std::vector<u64> ids(count);
    u32 read = 0;
    if (R_FAILED(AM_GetTitleList(&read, media[m], count, ids.data()))) continue;
    for (u32 i = 0; i < read; i++) {
      int v = 0;
      const GameInfo *g = gameByTitleId(ids[i], &v);
      if (!g) continue;
      bool dup = false;
      for (auto &x : g_installed) dup |= x.titleId == ids[i];
      if (dup) continue;
      InstalledGame ig;
      ig.game = g;
      ig.version = v;
      ig.cartridge = media[m] == MEDIATYPE_GAME_CARD;
      ig.titleId = ids[i];
      g_installed.push_back(ig);
    }
  }
  amExit();
}

static const InstalledGame *installedFor(const std::string &key) {
  // the cartridge first (it is what's in the console right now)
  const InstalledGame *best = nullptr;
  for (auto &i : g_installed)
    if (key == i.game->key && (!best || i.cartridge)) best = &i;
  return best;
}

// ---- screen output ----------------------------------------------------------------------------------------------

static Canvas *g_top, *g_bottom;

static void present() {
  u16 w, h;
  u8 *fbTop = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w, &h);
  blit(*g_top, {0, 0, 400, 240}, ctruTarget(fbTop, 400, PixFmt::BGR8), 0, 0, false);
  u8 *fbBot = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &w, &h);
  blit(*g_bottom, {0, 0, 320, 240}, ctruTarget(fbBot, 320, PixFmt::BGR8), 0, 0, false);
  gfxFlushBuffers();
  gfxSwapBuffers();
}

// ---- input ------------------------------------------------------------------------------------------------------

static Input readInput() {
  static bool wasTouch = false;
  static int lastX = 0, lastY = 0;
  hidScanInput();
  Input in;
  in.pressed = hidKeysDown();
  in.held = hidKeysHeld();
  bool touch = in.held & KEY_TOUCH;
  if (touch) {
    touchPosition t;
    hidTouchRead(&t);
    in.touchHeld = true;
    in.touchDown = !wasTouch;
    in.scrollDy = wasTouch ? t.py - lastY : 0;
    in.tx = t.px;
    in.ty = t.py;
    lastX = t.px;
    lastY = t.py;
  } else if (wasTouch) {
    in.touchUp = true;
    in.tx = lastX;
    in.ty = lastY;
  }
  wasTouch = touch;
  return in;
}

static bool askName(std::string &name) {
  SwkbdState kb;
  swkbdInit(&kb, SWKBD_TYPE_NORMAL, 2, 30);
  swkbdSetHintText(&kb, tr("Name des Laufs", "Name of the run"));
  swkbdSetInitialText(&kb, name.c_str());
  swkbdSetValidation(&kb, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
  char buf[96] = {0};
  SwkbdButton b = swkbdInputText(&kb, buf, sizeof buf);
  if (b != SWKBD_BUTTON_CONFIRM) return false;
  name = buf;
  return true;
}

// ---- starting the game ------------------------------------------------------------------------------------------

enum class Launch { Started, NoGame, NoPlugin, LoaderOff, Error };

static bool fileExists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

static Launch launchGame(const Run &run, bool enableLoader, bool withoutPlugin, std::string &detail) {
  const InstalledGame *ig = installedFor(run.game);
  if (!ig) return Launch::NoGame;
  if (!withoutPlugin) {
    if (!fileExists((std::string("sdmc:") + kPluginPath).c_str())) return Launch::NoPlugin;
    if (R_FAILED(plgLdrInit())) return Launch::LoaderOff;  // no Luma3DS plugin loader service
    bool on = false;
    PLGLDR__IsPluginLoaderEnabled(&on);
    if (!on && !enableLoader) {
      plgLdrExit();
      return Launch::LoaderOff;
    }
    if (!on) PLGLDR__SetPluginLoaderState(true);
    PluginLoadParameters p;
    std::memset(&p, 0, sizeof p);
    p.noFlash = true;
    p.lowTitleId = (u32)(ig->titleId & 0xffffffffu);
    std::strncpy(p.path, kPluginPath, sizeof p.path - 1);
    Result r = PLGLDR__SetPluginLoadParameters(&p);
    plgLdrExit();
    if (R_FAILED(r)) {
      char b[48];
      std::snprintf(b, sizeof b, "plg:ldr 0x%08lX", (unsigned long)r);
      detail = b;
      return Launch::Error;
    }
  }
  g_store.active = run.id;
  saveStore();
  Result r = APT_PrepareToDoApplicationJump(0, ig->titleId, ig->cartridge ? MEDIATYPE_GAME_CARD : MEDIATYPE_SD);
  if (R_SUCCEEDED(r)) {
    u8 hmac[0x20] = {0};
    u8 param[0x300] = {0};
    r = APT_DoApplicationJump(param, sizeof param, hmac);
  }
  if (R_FAILED(r)) {
    char b[48];
    std::snprintf(b, sizeof b, "APT 0x%08lX", (unsigned long)r);
    detail = b;
    return Launch::Error;
  }
  return Launch::Started;
}

// ---- main -------------------------------------------------------------------------------------------------------

enum class Screen { Home, Wizard, Run, Message };

int main(int argc, char **argv) {
  gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
  gfxSetDoubleBuffering(GFX_TOP, true);
  gfxSetDoubleBuffering(GFX_BOTTOM, true);
  // (no sound: souldew's launcher never initialises audio)
  g_top = new Canvas(400, 240);
  g_bottom = new Canvas(320, 240);
  g_store.load(std::string("sdmc:") + kRunsPath);
  g_lang = langOf(g_store.settings);
  scanTitles();

  HomeScreen home;
  // start on the active run (the list shows the newest first)
  for (size_t i = 0; i < g_store.runs.size(); i++)
    if (g_store.runs[i].id == g_store.active) home.selected = (int)(g_store.runs.size() - 1 - i);
  NewRunWizard wiz;
  RunView view;
  ViewCtx ctx;
  Screen scr = Screen::Home;
  std::string status, msgTitle, msgBody;
  Tone msgTone = Tone::Info;
  const char *msgA = nullptr, *msgB = nullptr;
  std::vector<Hit> msgHits;
  int msgKind = 0;  // 1 loader off (A: switch on + start, B: start without), 2 info (A: ok)
  bool redraw = true;
  bool quit = false;
  int downX = 0, downY = 0;

  auto openRun = [&](const std::string &id) {
    Run *r = g_store.byId(id);
    if (!r) return;
    g_store.active = id;
    ctx = ViewCtx();
    ctx.run = r;
    ctx.game = gameByKey(r->game.c_str());
    ctx.overlays = g_store.settings.overlays;
    view.reset();
    scr = Screen::Run;
  };
  auto message = [&](const std::string &t, const std::string &b, Tone tone, int kind, const char *a, const char *bb) {
    msgTitle = t;
    msgBody = b;
    msgTone = tone;
    msgKind = kind;
    msgA = a;
    msgB = bb;
    scr = Screen::Message;
  };
  auto start = [&](bool enableLoader, bool withoutPlugin) {
    Run *r = ctx.run;
    if (!r) return;
    std::string detail;
    const GameInfo *g = gameByKey(r->game.c_str());
    // the launching page is shown while the system switches over
    drawMessage(*g_top, *g_bottom, fmt(tr("Starte %s \xe2\x80\xa6", "Starting %s \xe2\x80\xa6"), g ? g->name : "?"),
                tr("Das souldew-Plugin wird f\xc3\xbcr dieses Spiel geladen. Im Spiel: SELECT oder die Ecke unten rechts \xc3\xb6" "ffnet souldew.",
                   "The souldew plugin is loaded for this game. In game: SELECT or the bottom-right corner opens souldew."),
                "", Tone::Ok);
    present();
    Launch l = launchGame(*r, enableLoader, withoutPlugin, detail);
    switch (l) {
      case Launch::Started: quit = true; break;
      case Launch::NoGame:
        message(tr("Spiel nicht gefunden", "Game not found"),
                fmt(tr("%s ist weder installiert noch eingesteckt. Stecke die Karte ein oder installiere das Spiel \xe2\x80\x93 der Lauf bleibt gespeichert.",
                       "%s is neither installed nor inserted. Insert the cartridge or install the game \xe2\x80\x93 the run stays saved."),
                    g ? g->name : "?"),
                Tone::Warn, 2, "OK", nullptr);
        break;
      case Launch::NoPlugin:
        message(tr("Plugin fehlt", "Plugin missing"),
                tr("sdmc:/3ds/souldew/souldew.3gx fehlt. Kopiere es dorthin (siehe README) \xe2\x80\x93 oder starte ohne Overlays.",
                   "sdmc:/3ds/souldew/souldew.3gx is missing. Copy it there (see README) \xe2\x80\x93 or start without overlays."),
                Tone::Warn, 3, tr("Ohne Overlays starten", "Start without overlays"), tr("Zur\xc3\xbc" "ck", "Back"));
        break;
      case Launch::LoaderOff:
        message(tr("Plugin-Loader ist aus", "Plugin loader is off"),
                tr("Ohne den Plugin-Loader von Luma3DS gibt es im Spiel keine Overlays. Einschalten und starten?",
                   "Without Luma3DS' plugin loader there are no overlays in game. Switch it on and start?"),
                Tone::Warn, 1, tr("Einschalten & starten", "Switch on & start"), tr("Ohne starten", "Start without"));
        break;
      case Launch::Error:
        message(tr("Start fehlgeschlagen", "Couldn't start"), detail, Tone::Danger, 2, "OK", nullptr);
        break;
    }
    redraw = true;
  };

#ifdef SOULDEW_DEMO
  // demo build: presses its own buttons on a schedule, so the emulator can take screenshots of every screen
  struct Step {
    int frame;
    uint32_t key;
  };
  static const Step demo[] = {{240, K_A}, {420, K_R}, {600, K_R}, {780, K_R}, {960, K_B}, {1140, K_Y},
                              {1320, K_A}, {1380, K_DOWN}, {1440, K_DOWN}, {1500, K_A}, {1680, K_B}, {1740, K_B}, {1800, K_B}};
  int frameNo = 0;
#endif
  while (aptMainLoop() && !quit) {
    Input in = readInput();
#ifdef SOULDEW_DEMO
    frameNo++;
    for (auto &st : demo)
      if (st.frame == frameNo) in.pressed |= st.key;
    if (frameNo > 1900) frameNo = 0;  // loop
#endif
    if (in.touchDown) {
      downX = in.tx;
      downY = in.ty;
    }
    switch (scr) {
      case Screen::Home: {
        HomeAction a = home.handle(in, g_store);
        if (a == HomeAction::Quit) quit = true;
        else if (a == HomeAction::OpenRun) openRun(home.selectedId(g_store));
        else if (a == HomeAction::Continue) {
          openRun(home.selectedId(g_store));
          start(false, false);
        } else if (a == HomeAction::NewRun) {
          wiz.start(g_installed);
          scr = Screen::Wizard;
        }
        if (a != HomeAction::None) redraw = true;
        break;
      }
      case Screen::Wizard: {
        WizardAction a = wiz.handle(in, g_installed);
        if (a == WizardAction::Cancel) scr = Screen::Home;
        else if (a == WizardAction::AskName) askName(wiz.name);
        else if (a == WizardAction::Create) {
          const GameInfo *g = gameAt(wiz.gameIndex);
          Run &r = g_store.create(wiz.name, g->key, wiz.rules, wallSec());
          std::string id = r.id;
          saveStore();
          home.selected = 0;
          openRun(id);
        }
        if (a != WizardAction::None) redraw = true;
        break;
      }
      case Screen::Run: {
        ViewAction a = view.handle(in, ctx, wallSec());
        if (a == ViewAction::Back) {
          saveStore();
          scr = Screen::Home;
        } else if (a == ViewAction::StartGame) start(false, false);
        else if (a == ViewAction::Changed) saveStore();
        else if (a == ViewAction::ToggleOverlays) {
          g_store.settings.overlays = !g_store.settings.overlays;
          ctx.overlays = g_store.settings.overlays;
          saveStore();
        } else if (a == ViewAction::DeleteRun) {
          g_store.remove(ctx.run->id);
          saveStore();
          home.selected = 0;
          scr = Screen::Home;
        }
        if (a != ViewAction::None) redraw = true;
        break;
      }
      case Screen::Message: {
        int tap = 0;
        if (in.touchUp)
          for (auto &h : msgHits)
            if (h.r.contains(in.tx, in.ty) && h.r.contains(downX, downY)) tap = h.id;
        if (in.pressed & KEY_A) tap = 1;
        if (in.pressed & KEY_B) tap = msgB ? 2 : 1;
        if (tap) {
          scr = ctx.run ? Screen::Run : Screen::Home;
          redraw = true;
          if (msgKind == 1) start(tap == 1, tap == 2);
          else if (msgKind == 3 && tap == 1) start(false, true);
        }
        break;
      }
    }
    if (in.pressed || in.touchDown || in.touchUp || in.touchHeld) redraw = true;
    if (redraw && !quit) {
      switch (scr) {
        case Screen::Home: home.draw(*g_top, *g_bottom, g_store, g_installed, status); break;
        case Screen::Wizard: wiz.draw(*g_top, *g_bottom, g_installed); break;
        case Screen::Run:
          view.drawTop(*g_top, ctx);
          view.drawBottom(*g_bottom, ctx);
          break;
        case Screen::Message: drawMessage(*g_top, *g_bottom, msgTitle, msgBody, "", msgTone, msgA, msgB, &msgHits); break;
      }
      present();
      present();  // both buffers
      redraw = false;
    }
    gspWaitForVBlank();
  }
  delete g_top;
  delete g_bottom;
  gfxExit();
  return 0;
}
