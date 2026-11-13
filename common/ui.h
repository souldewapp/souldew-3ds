// souldew's look on the 3DS ("handheld at night": dark console plastic, muted LCD greens, the two link colours of
// the logo, pixel display type) – theme, components, and the run screens the launcher and the in-game plugin share.
#pragma once
#include <string>
#include <vector>
#include "gfx.h"
#include "nuzlocke.h"
#include "run.h"

namespace sd {

namespace th {
constexpr Color stage = rgb(0x121116);
constexpr Color shell = rgb(0x1b1a1f), shell2 = rgb(0x222128), shell3 = rgb(0x2a2931), shell4 = rgb(0x33323b);
constexpr Color edge = rgb(0x393842), edgeHi = rgb(0x4a4954);
constexpr Color ink = rgb(0xebe6da), ink2 = rgb(0xa9a59b), ink3 = rgb(0x938f87), ink4 = rgb(0x6b6862);
constexpr Color lcd0 = rgb(0x0e130f), lcd1 = rgb(0x18211a), lcd2 = rgb(0x2c3b2d), lcd3 = rgb(0x6f8a62), lcd4 = rgb(0xb9cb98);
constexpr Color tri = rgb(0x6fb39b), cir = rgb(0xdc7268), crs = rgb(0x7f9ddb), sqr = rgb(0xc98bbf), warn = rgb(0xe1b05a);
constexpr Color okInk = rgb(0x9ed3bf), badInk = rgb(0xef9a90), warnInk = rgb(0xecc57f), crsInk = rgb(0xaac0f0);
constexpr Color well = rgb(0x15141a), wellEdge = rgb(0x0d0c10);
constexpr Color panel = rgb(0x26252d);
// the logo's link colours (player 1 blue, player 2 pink, the link between them) and the website's play button
constexpr Color p1 = rgb(0x8cb0ff), p2 = rgb(0xff8fdc), link = rgb(0xf2ffdc);
constexpr Color play = rgb(0x3f6b55), playHi = rgb(0x4f8a6c);
}  // namespace th

Color typeColor(int t);
Color toneColor(Tone t);
Color toneInk(Tone t);
Color gameColor(const std::string &game);
Color rarityColor(int rarity);  // 0 common … 4 legendary

// keys (libctru / CTRPF values)
enum : uint32_t {
  K_A = 1u << 0, K_B = 1u << 1, K_SELECT = 1u << 2, K_START = 1u << 3, K_RIGHT = 1u << 4, K_LEFT = 1u << 5,
  K_UP = 1u << 6, K_DOWN = 1u << 7, K_R = 1u << 8, K_L = 1u << 9, K_X = 1u << 10, K_Y = 1u << 11,
  K_ZL = 1u << 14, K_ZR = 1u << 15, K_TOUCH = 1u << 20,
  K_CRIGHT = 1u << 28, K_CLEFT = 1u << 29, K_CUP = 1u << 30, K_CDOWN = 1u << 31,
};

struct Input {
  uint32_t pressed = 0, held = 0;
  bool touchDown = false;   // touch started this frame
  bool touchHeld = false;
  bool touchUp = false;     // released this frame (a tap ends here)
  int tx = 0, ty = 0;       // last touch position
  int scrollDy = 0;         // drag distance this frame
};

struct Hit {
  Rect r;
  int id, arg;
};

// ---- components ----------------------------------------------------------------------------------------------------
enum class Backdrop { Plain, Hero, Paused };
extern bool g_cacheBackdrops;  // keep each backdrop once drawn (the launcher; the plugin saves the memory)
void drawBackdrop(Canvas &c, Backdrop kind = Backdrop::Plain);
// the logo: two linked pixel rings (32×16 cells × scale); `link` 0..1 animates the rings sliding together
void drawLogoMark(Canvas &c, int x, int y, int scale, float link = 1.f, bool glow = false);
void drawLogo(Canvas &c, int x, int y, bool small = false);  // the handheld badge + "souldew"
void drawMonTile(Canvas &c, int x, int y, int size, int species, bool dead = false, bool shiny = false);
int drawChip(Canvas &c, int x, int y, const std::string &text, Color fg, Color bg = Color(), Color border = Color());
int chipWidth(const std::string &text);
void drawHpBar(Canvas &c, int x, int y, int w, int h, int hp, int maxHp);
void drawStatTile(Canvas &c, int x, int y, int w, int h, int value, const char *label, Color valueColor);
enum class BtnStyle { Primary, Accent, Ghost, Danger };
void drawButton(Canvas &c, Rect r, const std::string &label, BtnStyle s, bool focused = false, const char *icon = nullptr);
// pixel icons: play back plus minus check chevron trophy gear list help power globe eye lock cross star pin info warn
// pause tab flag ball bolt grin crown sparkle — x = left edge, cy = vertical centre; returns the width
int drawIcon(Canvas &c, const char *name, int x, int cy, Color col, int scale = 1);
int iconWidth(const char *name, int scale = 1);
void drawKeyHint(Canvas &c, int x, int y, const char *key, const char *label, int *outW = nullptr);
void drawTabBar(Canvas &c, int y, const std::vector<std::string> &tabs, int active, std::vector<Hit> *hits, int hitBase,
                const std::vector<int> *counts = nullptr);
void drawToast(Canvas &c, int x, int y, int w, const Event &e, float t /* 0..1 appear */);
void drawBattleBadge(Canvas &c, int rightX, int y, const std::string &text, Tone tone);
void drawCornerTab(Canvas &c, int x, int y, bool pressed);
void drawScrollHint(Canvas &c, Rect area, int scroll, int content);
// a raised surface: vertical gradient, 1px edge, a light top rim, optional soft shadow
void drawSurface(Canvas &c, Rect r, int rad, Color top, Color bottom, Color edge, int shadow = 0);
void drawCard(Canvas &c, Rect r, int rad = 10, bool focused = false);
void drawSectionLabel(Canvas &c, int x, int y, const std::string &text, Color col = th::ink3, int lineTo = 0);
void drawSwitch(Canvas &c, int x, int y, bool on);  // 28×16
void drawProgress(Canvas &c, int x, int y, int w, int h, int n, int total, Color col);
void drawPips(Canvas &c, int x, int y, int n, int total, Color on);  // badges: small diamonds
void drawPageDots(Canvas &c, int cx, int y, int n, int active);
// achievements: the desktop app's computed pixel medals (kind 0..3 category, 4 champion; tier -1 locked … 4 master)
void drawMedal(Canvas &c, int x, int y, int kind, int tier, int scale = 2);  // 24×31 cells
// an achievement's coin (13×13 cells): metal by rarity, the category's emblem; locked: dark with a lock / "?"
void drawCoin(Canvas &c, int x, int y, int scale, int cat, int rarity, bool locked, bool hidden);
// a dark translucent band behind the action buttons at the bottom of a screen
void drawActionBar(Canvas &c, int y);

std::string monLabel(const RunMon &m);
std::string shortName(int species);

// ---- the run screens -----------------------------------------------------------------------------------------------
struct ViewCtx {
  Run *run = nullptr;
  const GameInfo *game = nullptr;
  bool inGame = false;              // drawn by the plugin over the paused game
  const std::vector<Mon> *party = nullptr;  // live party (plugin), else the run's last known
  std::string area;                 // current place key
  int loc = 0;
  bool overlays = true;
};

enum class ViewAction { None, Back, StartGame, ResumeGame, Changed, DeleteRun, ToggleOverlays };

class RunView {
 public:
  int tab = 0;  // 0 team, 1 routes, 2 graveyard, 3 rules
  void reset() {
    tab = 0;
    for (int &s : scroll_) s = 0;
    sheet_ = 0;
    focus_ = 0;
  }
  void drawTop(Canvas &c, const ViewCtx &ctx);
  void drawBottom(Canvas &c, const ViewCtx &ctx);
  ViewAction handle(const Input &in, ViewCtx &ctx, long long now);

 private:
  int scroll_[4] = {0, 0, 0, 0};
  int content_[4] = {0, 0, 0, 0};
  int focus_ = 0;           // keyboard focus in the list
  int sheet_ = 0;           // 0 none, 1 mon actions, 2 encounter actions, 3 confirm delete, 4 end the run
  int sheetArg_ = -1;
  std::vector<Hit> hits_;
  std::vector<int> rows_;   // list row → index (mons / encounters)

  void listTeam(Canvas &c, const ViewCtx &ctx, Rect area);
  void listRoutes(Canvas &c, const ViewCtx &ctx, Rect area);
  void listGrave(Canvas &c, const ViewCtx &ctx, Rect area);
  void listRules(Canvas &c, const ViewCtx &ctx, Rect area);
  void drawSheet(Canvas &c, const ViewCtx &ctx);
};

// the team cards on the top screen (both views)
void drawTeamGrid(Canvas &c, int x, int y, const Run &run, const std::vector<Mon> *party);
void drawRunHeader(Canvas &c, int x, int y, int w, const ViewCtx &ctx);

}  // namespace sd
