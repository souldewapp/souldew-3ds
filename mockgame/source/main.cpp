// souldew mock game – a test title (title ID 000400000F5D0000) that lays out its memory like Pokémon X 1.0 at the
// addresses the plugin reads, and plays a scripted Nuzlocke scene by scene: town → Route 2 → wild battle (first
// encounter) → catch → forest → dupe → over the level cap → trainer battle → a death → a badge. It lets the real
// plugin run end to end in Azahar (its plugin loader loads luma/plugins/000400000F5D0000/*.3gx for it) without any
// game files. Its RomFS holds a zone table laid out like X/Y's (tools/gen_mock_romfs.py).
#include <3ds.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "fbblit.h"
#include "pk6.h"
#include "ui.h"

using namespace sd;

extern "C" Result sdk_gsp_register(Handle *session, Handle event, u32 flags, Handle *outSharedMem, u32 *outThreadId);
extern "C" u32 sdk_present(u32 isBottom, int nextBank, void *leftFb, void *rightFb, int stride, int format, int swap);

// Pokémon X 1.0 (common/games.cpp)
static const u32 PARTY = 0x08CE1CE8, BADGES = 0x08C6A6A0, WILD = 0x08203ED0, TRAINER = 0x082059E0, ZONE = 0x081828EC;

static void put16(u8 *b, int o, u16 v) {
  b[o] = v & 0xff;
  b[o + 1] = v >> 8;
}
static void put32(u8 *b, int o, u32 v) {
  for (int i = 0; i < 4; i++) b[o + i] = (v >> (8 * i)) & 0xff;
}

struct MockMon {
  u32 pid, ec;
  int species, level, hp, maxHp, met;
  const char *nick;
};

// a PK6 with its party stats, encrypted, written into party slot `slot` (box data + stats at +0x158)
static void writeSlot(int slot, const MockMon *m) {
  u8 *at = reinterpret_cast<u8 *>(PARTY + slot * SLOT_STRIDE);
  if (!m) {
    std::memset(at, 0, SLOT_STRIDE);
    return;
  }
  u8 plain[PK_SIZE] = {0}, enc[PK_SIZE];
  put32(plain, 0x00, m->ec);
  put16(plain, 0x08, m->species);
  put16(plain, 0x0C, 12345);  // TID
  put16(plain, 0x0E, 54321);  // SID
  put32(plain, 0x18, m->pid);
  plain[0x1D] = 0 << 1;       // male, form 0
  for (int i = 0; i < 12 && m->nick[i]; i++) put16(plain, 0x40 + i * 2, (u8)m->nick[i]);
  put16(plain, 0x5A, 33);     // a move (Tackle)
  put32(plain, 0x74, 0x1F1F1F1F & 0x3FFFFFFF);
  put16(plain, 0xDA, m->met);
  plain[0xDD] = m->level;
  put32(plain, 0xE8, 0);      // status
  plain[0xEC] = m->level;
  put16(plain, 0xF0, m->hp);
  put16(plain, 0xF2, m->maxHp);
  for (int o = 0xF4; o < 0xFE; o += 2) put16(plain, o, 20 + m->level);
  encryptPK6(plain, PK_SIZE, enc);
  std::memset(at, 0, SLOT_STRIDE);
  std::memcpy(at, enc, BOX_SIZE);
  std::memcpy(at + STATS_OFFSET, enc + BOX_SIZE, STATS_SIZE);
}

// a battler block (Gen 6: 580 bytes) at the battle base
static void writeBattler(u32 base, int block, int species, int level, int hp, int maxHp, int slot) {
  u8 *o = reinterpret_cast<u8 *>(base + block * 580);
  std::memset(o, 0, 580);
  put16(o, 4, species);
  put16(o, 6, maxHp);
  put16(o, 8, hp);
  o[0x10] = level;
  o[0x11] = slot;
}
static void clearBattle(u32 base) { std::memset(reinterpret_cast<u8 *>(base), 0, 580 * 12); }

int main() {
  u64 t0 = osGetTime();
  gfxInit(GSP_RGB565_OES, GSP_BGR8_OES, false);  // RGB565 on top: the plugin's blitter handles every format
  u8 *zone = reinterpret_cast<u8 *>(ZONE);
  u8 *badges = reinterpret_cast<u8 *>(BADGES);
  // the addresses must lie in the heap and above what malloc hands out
  Canvas *top = new Canvas(400, 240);
  Canvas *bot = new Canvas(320, 240);
  bool layoutOk = (u32)top < ZONE && (u32)bot + 320 * 240 * 4 < WILD && (u32)bot + 320 * 240 * 4 < ZONE;
  for (u32 a : {PARTY, BADGES, WILD, TRAINER, ZONE, (u32)0x08100000}) {
    MemInfo mi;
    PageInfo pi;
    layoutOk &= R_SUCCEEDED(svcQueryMemory(&mi, &pi, a)) && mi.state != MEMSTATE_FREE && (mi.perm & MEMPERM_WRITE);
  }
  std::memset(reinterpret_cast<u8 *>(0x08100000), 0, 64);  // the test plugin's trace counters
  std::memset(reinterpret_cast<u8 *>(PARTY), 0, 6 * SLOT_STRIDE);
  clearBattle(WILD);
  clearBattle(TRAINER);
  *badges = 0;

  MockMon froakie{0x1234ABCD, 0x5A5A1111, 656, 5, 20, 20, 10, "Froxy"};
  MockMon fletch{0x0BADF00D, 0x77E40001, 661, 3, 12, 12, 12, "Dartiri"};
  int scene = -1;
  std::string label;
  bool gspHooked = false;
  while (aptMainLoop()) {
    // once the plugin is up (it hooks this SDK-style call), register an interrupt queue through it – from then on
    // the plugin owns the GSP interrupts and the mock paces itself with sleeps instead of libctru's VBlank wait
    if (!gspHooked && osGetTime() - t0 > 1500) {
      gspHooked = true;
      Handle ev = 0, mem = 0;
      u32 tid = 0;
      svcCreateEvent(&ev, RESET_ONESHOT);
      sdk_gsp_register(gspGetSessionHandle(), ev, 1, &mem, &tid);
    }
    hidScanInput();
    if (hidKeysDown() & KEY_START) break;
    double t = (osGetTime() - t0) / 1000.0;
    int s = t < 6 ? 0 : t < 12 ? 1 : t < 18 ? 2 : t < 22 ? 3 : t < 28 ? 4 : t < 34 ? 5 : t < 38 ? 6 : t < 42 ? 7 : t < 48 ? 8 : t < 54 ? 9 : 10;
    if (s != scene) {
      scene = s;
      switch (s) {
        case 0:  // the town, the starter
          writeSlot(0, &froakie);
          put16(zone, 0, 1);
          label = "Aquarellia (Zone 1) - Starter Froxy";
          break;
        case 1:
          put16(zone, 0, 2);
          label = "Route 2 (Zone 2)";
          break;
        case 2:  // a wild Fletchling
          writeBattler(WILD, 0, 656, 5, 20, 20, 0);
          writeBattler(WILD, 1, 661, 3, 12, 12, 12);
          label = "Wildes Dartiri Lv 3 (erste Begegnung)";
          break;
        case 3:  // caught
          clearBattle(WILD);
          writeSlot(1, &fletch);
          label = "Gefangen: Dartiri";
          break;
        case 4:
          put16(zone, 0, 3);
          label = "Nouvaria-Wald (Zone 3)";
          break;
        case 5:  // another Fletchling: a dupe
          writeBattler(WILD, 0, 656, 5, 20, 20, 0);
          writeBattler(WILD, 1, 661, 4, 13, 13, 12);
          label = "Wildes Dartiri Lv 4 (Dupe)";
          break;
        case 6:  // levelled past the cap (12 before the first badge)
          clearBattle(WILD);
          froakie.level = 13;
          froakie.hp = froakie.maxHp = 38;
          writeSlot(0, &froakie);
          label = "Froxy Lv 13 (ueber dem Cap 12)";
          break;
        case 7:  // a trainer battle, Fletchling in front
          writeBattler(TRAINER, 0, 661, 3, 12, 12, 1);
          writeBattler(TRAINER, 1, 664, 6, 18, 18, 12);
          label = "Trainerkampf (Cap-Warnung)";
          break;
        case 8:  // Fletchling faints
          writeBattler(TRAINER, 0, 661, 3, 0, 12, 1);
          fletch.hp = 0;
          writeSlot(1, &fletch);
          label = "Dartiri faellt";
          break;
        case 9:  // the battle ends, the first badge
          clearBattle(TRAINER);
          *badges = 1;
          label = "Orden 1";
          break;
        default:
          put16(zone, 0, 5);
          label = "Nouvaria City (Zone 5) - Ende des Szenarios";
          break;
      }
    }
    // the "game picture": plain shapes, the scene's label, the clock (redrawn twice a second – the CPU is the
    // emulated ARM11's, and the plugin needs its share)
    static int lastStamp = -1;
    int stamp = (int)(t * 2);
    if (stamp != lastStamp) {
      lastStamp = stamp;
      top->clear(rgb(0x6fa7d8));
      for (int y = 0; y < 240; y += 2) top->fill(0, y, 400, 2, mix(rgb(0x6fa7d8), rgb(0xcfe7f3), y / 240.f));
      top->fill(0, 160, 400, 80, rgb(0x7fb35c));
      for (int y = 166; y < 240; y += 8)
        for (int x = (y / 8 % 2) * 8; x < 400; x += 16) top->fill(x, y, 6, 3, rgb(0x6fa24f));
      int px = 190 + (int)(20 * ((int)(t * 2) % 2));
      top->roundRect(px, 128, 22, 32, 8, rgb(0xd25a4a));
      top->roundRect(10, 196, 380, 36, 8, Color(250, 250, 250, 235));
      top->text(font_body_b, 20, 204, label, rgb(0x2b2b2b), 300);
      char clock[32];
      std::snprintf(clock, sizeof clock, "t=%02d s", (int)t);
      top->text(font_body_b, 320, 204, clock, rgb(0x2b2b2b), 60, ALIGN_RIGHT);
      top->text(font_label, 8, 228, layoutOk ? "SOULDEW MOCK GAME (X 1.0 LAYOUT)" : "MOCK GAME: HEAP LAYOUT FAILED", rgb(0x203040));
      bot->clear(rgb(0x3a6fb0));
      bot->roundRect(20, 30, 280, 140, 14, rgb(0x2d5a92));
      bot->text(font_body_b, 20, 92, "Mock-Spiel: Touchscreen", rgb(0xbcd4f0), 280, ALIGN_CENTER);
    }
    u16 w, h;
    u8 *fbTop = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w, &h);
    u8 *fbBot = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &w, &h);
    blit(*top, {0, 0, 400, 240}, ctruTarget(fbTop, 400, PixFmt::RGB565), 0, 0, false);
    blit(*bot, {0, 0, 320, 240}, ctruTarget(fbBot, 320, PixFmt::BGR8), 0, 0, false);
    // like a retail game: the frame goes through the SDK-style present call (where the plugin's OSD draws)
    sdk_present(0, 0, fbTop, nullptr, 240 * 2, GSP_RGB565_OES, 0);
    sdk_present(1, 0, fbBot, nullptr, 240 * 3, GSP_BGR8_OES, 0);
    gfxFlushBuffers();
    gfxSwapBuffers();
    if (gspHooked) svcSleepThread(16 * 1000 * 1000);
    else gspWaitForVBlank();
  }
  gfxExit();
  return 0;
}
