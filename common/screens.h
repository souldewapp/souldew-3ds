// The launcher's screens (title menu, onboarding, runs, new run, achievements, settings, messages) and the in-game
// overlay layer – platform-free, so the host renderer draws exactly what the console shows.
#pragma once
#include <deque>
#include <string>
#include <vector>
#include "trophies.h"
#include "ui.h"

namespace sd {

constexpr const char *kVersion = "1.1";

struct InstalledGame {
  const GameInfo *game = nullptr;
  int version = 0;      // index into game->titleIds
  bool cartridge = false;
  uint64_t titleId = 0;
};

// the two versions' colours of a game (the cartridge art: no logos, only colour)
void gameVersionColors(const std::string &game, Color &a, Color &b);
// a small game cartridge in the versions' colours with the game's short name
void drawCartridge(Canvas &c, int x, int y, int w, int h, const GameInfo *g, bool dim = false);

// ---- title menu ----------------------------------------------------------------------------------------------------
enum class MenuAction { None, Changed, Continue, NewRun, Runs, Trophies, Settings, Help, Quit };

class MainMenu {
 public:
  int focus = 0;        // 0 continue, 1 new run, 2 runs, 3 achievements, 4 settings, 5 help, 6 quit
  float intro = 1.f;    // 0..1: the logo's rings link up (the launcher advances it after start)
  void draw(Canvas &top, Canvas &bottom, RunStore &store, const std::vector<InstalledGame> &installed, const std::string &status);
  void drawTop(Canvas &top, RunStore &store, const std::vector<InstalledGame> &installed, const std::string &status);
  MenuAction handle(const Input &in, RunStore &store);

 private:
  std::vector<Hit> hits_;
  int downX_ = 0, downY_ = 0;
};

// ---- first launch: what souldew does, how to open it in game, the Nuzlocke basics, the language -------------------
enum class IntroAction { None, Changed, Language, Done, NewRun };

class Onboarding {
 public:
  static constexpr int kPages = 5;
  int page = 0;
  int focus = 1;         // the button / option under the D-pad (page 0: 0 de, 1 en …)
  bool fromMenu = false;  // opened again from the menu / settings ("help")
  void start(bool again);
  void draw(Canvas &top, Canvas &bottom, const RunStore &store);
  IntroAction handle(const Input &in, RunStore &store);

 private:
  std::vector<Hit> hits_;
  int downX_ = 0, downY_ = 0;
};

// ---- the runs list -------------------------------------------------------------------------------------------------
enum class HomeAction { None, OpenRun, Continue, NewRun, Quit, Changed, Back };

class HomeScreen {
 public:
  int selected = 0;
  void draw(Canvas &top, Canvas &bottom, RunStore &store, const std::vector<InstalledGame> &installed, const std::string &status);
  HomeAction handle(const Input &in, RunStore &store);
  std::string selectedId(const RunStore &store) const;

 private:
  std::vector<Hit> hits_;
  int scroll_ = 0;
  int downX_ = 0, downY_ = 0;
  bool drag_ = false;
};

// ---- new run -------------------------------------------------------------------------------------------------------
enum class WizardAction { None, Changed, Cancel, AskName, Create };

class NewRunWizard {
 public:
  int step = 0;          // 0 game, 1 rules, 2 confirm
  int gameIndex = 0;     // into gameAt()
  int version = 0;
  Rules rules;
  std::string name;
  int focus = 0;
  void start(const std::vector<InstalledGame> &installed);
  void draw(Canvas &top, Canvas &bottom, const std::vector<InstalledGame> &installed);
  WizardAction handle(const Input &in, const std::vector<InstalledGame> &installed);
  std::string defaultName() const;

 private:
  std::vector<Hit> hits_;
  int downX_ = 0, downY_ = 0;
};

// ---- achievements --------------------------------------------------------------------------------------------------
enum class TrophyAction { None, Changed, Back };

class TrophyScreen {
 public:
  int filter = 0;  // 0 all, 1..4 a category
  int focus = 0;   // in the filtered list
  void reset() {
    filter = 0;
    focus = 0;
    scroll_ = 0;
  }
  void draw(Canvas &top, Canvas &bottom, const RunStore &store);
  TrophyAction handle(const Input &in, const RunStore &store);

 private:
  std::vector<Hit> hits_;
  std::vector<int> shown_;  // filtered list → index into the trophy list
  int scroll_ = 0, content_ = 0;
  int downX_ = 0, downY_ = 0;
  bool drag_ = false;
  void keepVisible();
};

// ---- settings ------------------------------------------------------------------------------------------------------
enum class SettingsAction { None, Changed, Back, Language, Intro };

class SettingsScreen {
 public:
  int focus = 0;  // 0 language, 1 overlays, 2 corner tab, 3 introduction, 4 about
  void draw(Canvas &top, Canvas &bottom, const RunStore &store);
  SettingsAction handle(const Input &in, RunStore &store);

 private:
  std::vector<Hit> hits_;
  int downX_ = 0, downY_ = 0;
};

// a status page (launching, errors)
void drawMessage(Canvas &top, Canvas &bottom, const std::string &title, const std::string &body, const std::string &hint,
                 Tone tone, const char *buttonA = nullptr, const char *buttonB = nullptr, std::vector<Hit> *hits = nullptr);

// a screen change: `out` = `to` sliding in by `slide` px and fading over `from` (t 0..1)
void transition(Canvas &out, const Canvas &from, const Canvas &to, float t, int slide);

// the in-game "game picture" stand-in the onboarding and settings use as an illustration (no game graphics here)
void drawSceneArt(Canvas &c, Rect r, bool battle);

// ---- the overlay layer (plugin) ------------------------------------------------------------------------------------
class OverlayLayer {
 public:
  void push(const Event &e, double now);
  // the persistent battle badge (empty: none)
  void setBadge(const std::string &text, Tone tone) {
    badge_ = text;
    badgeTone_ = tone;
  }
  bool active(double now) const;  // something to draw on the top screen
  bool busy() const { return !queue_.empty() || !badge_.empty(); }
  // draws onto a top-screen-sized transparent canvas; returns the dirty rect (w = 0: nothing). The canvas is only
  // redrawn when the picture changes (`redrawn`) – between fades the plugin just blits the last one again.
  Rect drawTop(Canvas &c, double now, bool *redrawn = nullptr);
  Rect drawBottom(Canvas &c, bool tabVisible, bool tabPressed);
  static Rect tabRect() { return {320 - 34, 240 - 22, 30, 18}; }

 private:
  struct Item {
    Event e;
    double start;
    double dur;
  };
  std::deque<Item> queue_;
  std::string badge_;
  Tone badgeTone_ = Tone::Info;
  std::string lastKey_;
  Rect lastRect_{0, 0, 0, 0};
};

// the cards for newly unlocked achievements
Event trophyEvent(const TrophyDef &d);

}  // namespace sd
