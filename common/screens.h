// The launcher's screens (home, new run, launch) and the in-game overlay layer – platform-free, so the host renderer
// draws exactly what the console shows.
#pragma once
#include <deque>
#include <string>
#include <vector>
#include "ui.h"

namespace sd {

struct InstalledGame {
  const GameInfo *game = nullptr;
  int version = 0;      // index into game->titleIds
  bool cartridge = false;
  uint64_t titleId = 0;
};

enum class HomeAction { None, OpenRun, Continue, NewRun, Quit, Changed };

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

// a status page (launching, errors)
void drawMessage(Canvas &top, Canvas &bottom, const std::string &title, const std::string &body, const std::string &hint,
                 Tone tone, const char *buttonA = nullptr, const char *buttonB = nullptr, std::vector<Hit> *hits = nullptr);

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

}  // namespace sd
