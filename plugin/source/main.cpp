// souldew – the in-game part: a Luma3DS 3GX plugin (CTRPluginFramework) that runs inside the Pokémon game.
//
//  * reads the party, the running battle, the zone and the badges from the game's memory (common/pk6.cpp – the
//    addresses of the souldew desktop app) four times a second,
//  * feeds the Nuzlocke rules engine (common/nuzlocke.cpp) and shows its moments as short overlays on the game's top
//    screen (common/screens.cpp OverlayLayer – drawn into the game's framebuffer every frame by an OSD callback),
//  * SELECT (CTRPF's menu hotkey) or holding the small tab in the bottom screen's corner pauses the game and opens
//    souldew's screens (common/ui.cpp RunView – the same ones as the launcher); "Back to the game" resumes it,
//  * keeps the run in sdmc:/souldew/runs.json, shared with the launcher.
#include <3ds.h>
#ifndef NORETURN  // (libctru 2.7 dropped the macro CTRPF's headers still use)
#define NORETURN __attribute__((noreturn))
#endif
#include <CTRPluginFramework.hpp>

#include <cstdio>
#include <cstring>
#include <string>

#include "ctrzones.h"
#include "fbblit.h"
#include "i18n.h"
#include "nuzlocke.h"
#include "pk6.h"
#include "screens.h"
#include "trophies.h"

namespace CTRPluginFramework {

// (CTRPF has its own Rect and Color: souldew's are spelled sd::Rect / sd::Color here)
using sd::Canvas; using sd::Encounter; using sd::Engine; using sd::Event; using sd::EventKind; using sd::FbTarget;
using sd::GameInfo; using sd::ImageKind; using sd::Input; using sd::LiveFrame; using sd::Mon; using sd::OverlayLayer;
using sd::PartyRead; using sd::PartyReaderState; using sd::PixFmt; using sd::Rules; using sd::Run; using sd::RunStore;
using sd::RunView; using sd::Tone; using sd::ViewAction; using sd::ViewCtx; using sd::kRunsPath; using sd::blit;
using sd::fmt; using sd::gameByKey; using sd::gameByTitleId; using sd::langOf; using sd::readBattle; using sd::readParty;
using sd::readZone; using sd::tr; using sd::zonePlaces; using sd::g_lang; using sd::K_B; using sd::TrophyDef; using sd::trophyEvent;
using sd::unlockTrophies;

static RunStore g_store;
static Run *g_run = nullptr;
static const GameInfo *g_game = nullptr;
static Engine *g_engine = nullptr;
static std::vector<uint16_t> g_places;
static OverlayLayer g_layer;
static Mutex g_layerLock;
static LiveFrame g_frame;
static bool g_open = false;      // souldew's screens are up (game paused)
static bool g_tabPressed = false;
static u64 g_bootTick = 0;
static u64 g_lastSave = 0;
static bool g_dirty = false;

#ifdef SOULDEW_TEST
// test build: progress counters in the mock game's memory (free heap at 0x08100000), read back over Azahar's RPC
static void trace(int slot, u32 v) { *reinterpret_cast<volatile u32 *>(0x08100000 + slot * 4) = v; }
static void traceInc(int slot) { *reinterpret_cast<volatile u32 *>(0x08100000 + slot * 4) += 1; }
#else
static void trace(int, u32) {}
static void traceInc(int) {}
#endif

static double nowSec() { return (double)(svcGetSystemTick() - g_bootTick) / (double)SYSCLOCK_ARM11; }
static long long wallSec() { return (long long)(osGetTime() / 1000ull); }  // ms since 1900 → s (only differences matter)

// ---- files (CTRPF's File: the plugin has no newlib stdio on the SD card) ----------------------------------------

static bool readFile(const std::string &path, std::string &out) {
  File f;
  if (File::Open(f, path, File::READ) != File::SUCCESS) return false;
  u64 size = f.GetSize();
  if (size > (4u << 20)) {
    f.Close();
    return false;
  }
  out.assign((size_t)size, '\0');
  bool ok = !size || f.Read(&out[0], (u32)size) == File::SUCCESS;
  f.Close();
  return ok;
}

static bool writeFile(const std::string &path, const std::string &text) {
  std::string tmp = path + ".tmp";
  File::Remove(tmp);
  File f;
  if (File::Open(f, tmp, File::WRITE | File::CREATE | File::TRUNCATE) != File::SUCCESS) return false;
  bool ok = f.Write(text.data(), (u32)text.size()) == File::SUCCESS;
  f.Flush();
  f.Close();
  if (!ok) return false;
  File::Remove(path);
  return File::Rename(tmp, path) == File::SUCCESS;
}

static void saveStore() {
  Directory::Create("/souldew");
  traceInc(5);
  if (writeFile(kRunsPath, g_store.toText())) {
    traceInc(6);
    g_dirty = false;
    if (g_engine) g_engine->clearDirty();
  }
  g_lastSave = svcGetSystemTick();
}

// ---- the game's memory (the plugin runs inside the game: plain reads, guarded against unmapped pages) ------------

static bool memRead(uint32_t addr, void *out, uint32_t len, void *) {
  if (!len) return true;
  if (!Process::CheckAddress(addr, MEMPERM_READ) || !Process::CheckAddress(addr + len - 1, MEMPERM_READ)) return false;
  std::memcpy(out, reinterpret_cast<const void *>(addr), len);
  return true;
}

// ---- the zone table from the game's RomFS (cached on the SD card) -------------------------------------------------

static Handle g_romfs = 0;
static bool romfsRead(uint64_t off, void *out, uint32_t len, void *) {
  u32 got = 0;
  return R_SUCCEEDED(FSFILE_Read(g_romfs, &got, off, out, len)) && got == len;
}

static void loadZonePlaces(u64 tid) {
  char cache[64];
  std::snprintf(cache, sizeof cache, "/souldew/cache/zones_%016llX.bin", (unsigned long long)tid);
  std::string data;
  if (readFile(cache, data) && data.size() >= 2 && data.size() % 2 == 0) {
    g_places.resize(data.size() / 2);
    std::memcpy(g_places.data(), data.data(), data.size());
    return;
  }
  // the RomFS of the process the plugin runs in (as libctru's romfsMountFromCurrentProcess opens it)
  fsInit();
  static const u32 zeros[3] = {0, 0, 0};
  FS_Path archPath = {PATH_EMPTY, 1, ""};
  FS_Path filePath = {PATH_BINARY, sizeof zeros, zeros};
  Result rr = FSUSER_OpenFileDirectly(&g_romfs, ARCHIVE_ROMFS, archPath, filePath, FS_OPEN_READ, 0);
  trace(2, (u32)rr);
  if (R_SUCCEEDED(rr)) {
    g_places = zonePlaces(*g_game, romfsRead, nullptr, ImageKind::Auto);
    FSFILE_Close(g_romfs);
    g_romfs = 0;
  }
  if (!g_places.empty()) {
    Directory::Create("/souldew");
    Directory::Create("/souldew/cache");
    writeFile(cache, std::string(reinterpret_cast<const char *>(g_places.data()), g_places.size() * 2));
  }
}

// ---- reading + rules (the plugin's menu thread, every frame; reads 4× a second) ----------------------------------

static PartyReaderState g_partyState;

static void poll() {
  static u32 frame = 0;
  frame++;
  traceInc(3);
  // the corner tab: hold it ~0.2 s to open souldew (a quick tap still reaches the game)
  static u32 held = 0;
  if (g_store.settings.tab && Touch::IsDown()) {
    UIntVector p = Touch::GetPosition();
    sd::Rect r = OverlayLayer::tabRect();
    if (r.contains((int)p.x, (int)p.y)) {
      held++;
      g_tabPressed = held > 3;
      if (held == 12) PluginMenu::ForceOpen();
    } else held = 0;
  } else {
    held = 0;
    g_tabPressed = false;
  }
  if (frame % 15) return;
  if (!g_game || !g_engine) return;
  LiveFrame f;
  PartyRead pr = readParty(*g_game, memRead, nullptr, g_partyState);
  f.partyOk = pr.ok;
  f.party = pr.party;
  f.badges = pr.badges;
  f.battle = readBattle(*g_game, memRead, nullptr, pr.ok ? &f.party : nullptr);
  int zone = readZone(*g_game, memRead, nullptr);
  if (zone >= 0 && zone < (int)g_places.size()) f.place = g_places[zone];
  std::vector<Event> ev = g_engine->update(f, wallSec());
  // achievements: checked after every moment and every ~10 s (the engine also counts quiet things: 1 HP, evolutions)
  static u32 sinceTrophyCheck = 0;
  if (!ev.empty() || ++sinceTrophyCheck >= 40) {
    sinceTrophyCheck = 0;
    for (const TrophyDef *d : unlockTrophies(g_store, wallSec())) {
      ev.push_back(trophyEvent(*d));
      g_dirty = true;
    }
  }
  // the persistent battle badge: what this wild battle means for the run
  std::string badge;
  Tone tone = Tone::Info;
  if (f.battle.inBattle && g_run->status == "active") {
    const Encounter *e = g_run->encounterFor(g_engine->currentArea());
    if (!f.battle.trainer) {
      if (e && e->status == "open" && e->species == f.battle.enemySpecies) badge = tr("ERSTE BEGEGNUNG", "FIRST ENCOUNTER"), tone = Tone::Ok;
      else if (!e && !g_engine->currentArea().empty()) badge = "DUPE", tone = Tone::Warn;
      else if (e) badge = tr("KEIN FANG", "NO CATCH"), tone = Tone::Muted;
    } else {
      std::string boss;
      int cap = g_run->levelCap(&boss);
      if (cap) badge = fmt("CAP %d", cap), tone = Tone::Warn;
    }
  }
  g_layerLock.Lock();
  g_frame = f;
  if (g_store.settings.overlays) {
    double t = nowSec();
    for (auto &e : ev) g_layer.push(e, t);
    g_layer.setBadge(badge, tone);
  } else g_layer.setBadge("", Tone::Info);
  g_layerLock.Unlock();
  if (g_engine->dirty()) g_dirty = true;
  // save at most every 3 s (and right away when something important happened)
  bool important = false;
  for (auto &e : ev) important |= e.kind == EventKind::Death || e.kind == EventKind::Caught || e.kind == EventKind::RunLost || e.kind == EventKind::Trophy;
  if (g_dirty && (important || svcGetSystemTick() - g_lastSave > 3ull * SYSCLOCK_ARM11)) saveStore();
}

// ---- drawing over the game -------------------------------------------------------------------------------------

static FbTarget targetOf(const Screen &s, bool right) {
  FbTarget t;
  t.origin = s.GetFramebuffer(0, 0, right);
  t.stepX = (int)(s.GetFramebuffer(1, 0, right) - t.origin);
  t.stepY = (int)(s.GetFramebuffer(0, 1, right) - t.origin);
  t.width = s.IsTop ? 400 : 320;
  t.height = 240;
  t.fmt = static_cast<PixFmt>(s.Format & 7);
  return t;
}

static Canvas *g_ovTop = nullptr;
static Canvas *g_ovBottom = nullptr;

static bool drawOsd(const Screen &s) {
  traceInc(4);
  if (g_open || !g_ovTop) return false;
  if (s.IsTop) {
    g_layerLock.Lock();
    double t = nowSec();
    sd::Rect d{0, 0, 0, 0};
    if (g_layer.active(t)) d = g_layer.drawTop(*g_ovTop, t);
    g_layerLock.Unlock();
    if (!d.w) return false;
    // the game draws a fresh frame every time: the (cached) overlay goes on top of it again
    blit(*g_ovTop, d, targetOf(s, false), d.x, d.y, true);
    if (s.Is3DEnabled) blit(*g_ovTop, d, targetOf(s, true), d.x, d.y, true);
    return true;
  }
  if (!g_store.settings.tab) return false;
  sd::Rect r = OverlayLayer::tabRect();
  static int lastPressed = -1;
  if (lastPressed != (int)g_tabPressed) {  // only redrawn when it changes
    lastPressed = g_tabPressed;
    g_ovBottom->setClip(r);
    g_ovBottom->clear(sd::Color());
    g_ovBottom->resetClip();
    g_layer.drawBottom(*g_ovBottom, true, g_tabPressed);
  }
  blit(*g_ovBottom, r, targetOf(s, false), r.x, r.y, true);
  return true;
}

// ---- souldew's screens (game paused) -------------------------------------------------------------------------------

static bool openSouldew() {
  if (!g_run || g_open) return false;
  g_open = true;
  Process::Pause(true);
  static RunView view;
  view.reset();
  Canvas top(400, 240), bottom(320, 240);
  ViewCtx ctx;
  ctx.run = g_run;
  ctx.game = g_game;
  ctx.inGame = true;
  g_layerLock.Lock();
  std::vector<Mon> party = g_frame.party;
  g_layerLock.Unlock();
  ctx.party = party.empty() ? nullptr : &party;
  ctx.area = g_engine ? g_engine->currentArea() : g_run->lastArea;
  ctx.loc = g_engine ? g_engine->currentLoc() : 0;
  ctx.overlays = g_store.settings.overlays;
  bool redraw = true, running = true;
  int lastTx = 0, lastTy = 0;
  bool wasTouch = true;  // the finger that opened it is still down: ignore until lifted
  int framesSinceDraw = 0;
#ifdef SOULDEW_TEST
  u64 openedAt = svcGetSystemTick();
  int testTab = -1;
#endif
  while (running) {
    Controller::Update();
    Input in;
    in.pressed = Controller::GetKeysPressed();
    in.held = Controller::GetKeysDown();
    bool touch = Touch::IsDown();
    UIntVector p = Touch::GetPosition();
    if (touch) {
      in.touchHeld = true;
      in.touchDown = !wasTouch;
      in.scrollDy = wasTouch ? (int)p.y - lastTy : 0;
      in.tx = (int)p.x;
      in.ty = (int)p.y;
      lastTx = in.tx;
      lastTy = in.ty;
    } else if (wasTouch) {
      in.touchUp = true;
      in.tx = lastTx;
      in.ty = lastTy;
    }
    wasTouch = touch;
#ifdef SOULDEW_TEST
    // test build: walk through the tabs by itself, then return to the game (screenshots in the emulator)
    double open = (double)(svcGetSystemTick() - openedAt) / SYSCLOCK_ARM11;
    int want = (int)(open / 2.5);
    if (want != testTab && want < 4) {
      testTab = want;
      view.tab = want;
      redraw = true;
    }
    if (open > 10.5) in.pressed |= K_B;
#endif
    ViewAction a = view.handle(in, ctx, wallSec());
    trace(7, (u32)view.tab + 1);  // test build: which of souldew's tabs is up
    switch (a) {
      case ViewAction::ResumeGame:
      case ViewAction::Back: running = false; break;
      case ViewAction::ToggleOverlays:
        g_store.settings.overlays = !g_store.settings.overlays;
        ctx.overlays = g_store.settings.overlays;
        g_dirty = true;
        redraw = true;
        break;
      case ViewAction::Changed:
        g_dirty = true;
        redraw = true;
        // (an achievement reached by hand – the run ended, a catch confirmed – shows once the game runs again)
        g_layerLock.Lock();
        for (const TrophyDef *d : unlockTrophies(g_store, wallSec()))
          if (g_store.settings.overlays) g_layer.push(trophyEvent(*d), nowSec() + 0.5);
        g_layerLock.Unlock();
        break;
      default: break;
    }
    if (in.pressed || in.touchDown || in.touchUp) redraw = true;
    if (redraw) {
      view.drawTop(top, ctx);
      view.drawBottom(bottom, ctx);
      redraw = false;
      framesSinceDraw = 0;
    }
    // both buffers of each screen get the picture (double buffering)
    if (framesSinceDraw < 2) {
      const Screen &ts = OSD::GetTopScreen();
      const Screen &bs = OSD::GetBottomScreen();
      blit(top, {0, 0, 400, 240}, targetOf(ts, false), 0, 0, false);
      if (ts.Is3DEnabled) blit(top, {0, 0, 400, 240}, targetOf(ts, true), 0, 0, false);
      blit(bottom, {0, 0, 320, 240}, targetOf(bs, false), 0, 0, false);
      OSD::SwapBuffers();
      framesSinceDraw++;
    }
    svcSleepThread(16 * 1000 * 1000);
  }
  if (g_dirty) saveStore();
  g_open = false;
  Process::Play();
  trace(7, 0x100);
  return false;  // never open CTRPF's own menu
}

// ---- plugin entry points ---------------------------------------------------------------------------------------

void PatchProcess(FwkSettings &settings) {
  settings.AllowActionReplay = false;
  settings.AllowSearchEngine = false;
  settings.WaitTimeToBoot = Seconds(3);
  settings.TryLoadSDSounds = false;  // souldew never plays sounds
#ifdef SOULDEW_TEST
  // the mock game is libctru homebrew: CTRPF can't find a retail game's HID shared-memory mapping in it (it would
  // abort), so the test build reads the buttons through its own hid session
  settings.UseGameHidMemory = false;
#endif
}

// the game closing, the HOME menu, sleep: the run is saved before (the SD card may go away with the power)
static void onProcessEvent(Process::Event e) {
  if (e == Process::Event::EXIT || e == Process::Event::HOME_ENTER || e == Process::Event::SLEEP_ENTER ||
      e == Process::Event::SWAP_ENTER)
    if (g_dirty) saveStore();
}

static const GameInfo *gameForTitle(u64 tid) {
  const GameInfo *g = gameByTitleId(tid);
#ifdef SOULDEW_TEST
  if (!g && tid == 0x000400000F5D0000ull) g = gameByKey("xy");  // the mock game pretends to be X/Y
#endif
  return g;
}

int main(void) {
  trace(0, 1);
  g_bootTick = svcGetSystemTick();
  u64 tid = Process::GetTitleID();
  g_game = gameForTitle(tid);
  std::string text;
  if (readFile(kRunsPath, text)) g_store.parseText(text);
  g_lang = langOf(g_store.settings);
  if (g_game) {
    // the active run when it is for this game, else the newest active run for it, else a new one
    Run *r = g_store.activeRun();
    if (!r || r->game != g_game->key) {
      r = nullptr;
      for (auto it = g_store.runs.rbegin(); it != g_store.runs.rend(); ++it)
        if (it->game == g_game->key && it->status == "active") {
          r = &*it;
          break;
        }
    }
    if (!r) {
      Rules rules;
      rules.applyPreset("classic");
      int vi = 0;
      gameByTitleId(tid, &vi);
      r = &g_store.create(std::string("Nuzlocke ") + g_game->versionNames[vi], g_game->key, rules, wallSec());
      g_dirty = true;
    }
    g_store.active = r->id;
    g_run = r;
    // what the runs reached before this session counts, but isn't announced again
    if (!unlockTrophies(g_store, wallSec()).empty()) g_dirty = true;
    g_engine = new Engine(*g_run, *g_game);
    trace(0, 2);
    loadZonePlaces(tid);
    trace(1, 0x10000 | (u32)g_places.size());
  }
  g_ovTop = new Canvas(400, 240);
  g_ovBottom = new Canvas(320, 240);

  PluginMenu *menu = new PluginMenu("souldew", 0, 1, 0, "souldew \xe2\x80\x93 Nuzlocke tracker");
  menu->SynchronizeWithFrame(true);
  menu->ShowWelcomeMessage(false);
  menu->SetHexEditorState(false);
  menu->OnOpening = openSouldew;
  menu->Callback(poll);
  OSD::Run(drawOsd);
  Process::SetProcessEventCallback(onProcessEvent);
  if (g_game && g_run) {
    Event hello{EventKind::Info, Tone::Info, tr("souldew ist bereit", "souldew is ready"),
                fmt(tr("%s \xc2\xb7 SELECT \xc3\xb6" "ffnet souldew", "%s \xc2\xb7 SELECT opens souldew"), g_run->name.c_str())};
    g_layer.push(hello, nowSec() + 1.0);
  }
#ifdef SOULDEW_TEST
  // the mock game's scenario opens souldew's screens by itself once (emulator screenshots)
  struct Opener {
    static void tick() {
      static bool done = false;
      if (!done && nowSec() > 56.0) {
        done = true;
        PluginMenu::ForceOpen();
      }
    }
  };
  menu->Callback(Opener::tick);
#endif
  trace(0, 3);
  menu->Run();
  delete menu;
  if (g_dirty) saveStore();
  return 0;
}

}  // namespace CTRPluginFramework
