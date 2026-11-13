// The Nuzlocke rules engine: turns what the plugin reads from the game (party, battle, zone, badges) into run changes
// (encounters, catches, deaths, level caps) and the overlay messages for the important moments.
#pragma once
#include <string>
#include <vector>
#include "games.h"
#include "pk6.h"
#include "run.h"

namespace sd {

enum class Tone { Info, Ok, Danger, Warn, Muted };

enum class EventKind {
  NewArea,          // entered an area whose encounter is still free
  FirstEncounter,   // a wild battle that is the area's encounter
  DupeEncounter,    // dupes clause: this one doesn't count
  AreaUsed,         // wild battle in an area already used
  Caught,
  Missed,
  Gift,
  Death,
  Whiteout,
  OverCap,
  Badge,
  RunLost,
  Evolved,
  Trophy,           // an achievement unlocked (common/trophies.cpp)
  Info,
};

struct Event {
  EventKind kind;
  Tone tone;
  std::string title, sub;
  int species = 0;  // shown as a tile
  int rarity = 0;   // Trophy: 0 common … 4 legendary (the coin's metal)
  int cat = 0;      // Trophy: category (the coin's emblem)
};

struct LiveFrame {
  bool partyOk = false;
  std::vector<Mon> party;
  int badges = -1;
  BattleRead battle;
  int place = 0;  // met location id of the current zone, 0 = unknown
};

class Engine {
 public:
  Engine(Run &run, const GameInfo &game) : run_(run), g_(game) {}
  // `now`: seconds (any epoch); returns the moments worth an overlay
  std::vector<Event> update(const LiveFrame &f, long long now);
  bool dirty() const { return dirty_; }
  void clearDirty() { dirty_ = false; }
  std::string currentArea() const { return area_; }
  int currentLoc() const { return loc_; }
  const LiveFrame &last() const { return last_; }

  // the player resolves an open encounter by hand (party full: the catch went to the box)
  static void resolve(Run &run, Encounter &e, const std::string &status, long long now);
  static void setDead(Run &run, RunMon &m, bool dead, const std::string &cause, long long now);

 private:
  Run &run_;
  const GameInfo &g_;
  LiveFrame last_;
  bool first_ = true;
  bool initial_ = false;  // this update is the first one (what the game already shows isn't a new moment)
  bool dirty_ = false;
  bool wasInBattle_ = false;
  std::string area_;
  int loc_ = 0;
  std::string battleArea_;
  int battleLoc_ = 0;
  int battleSpecies_ = 0;
  long long battleEnd_ = 0;
  bool pendingMiss_ = false;
  std::vector<std::string> capWarned_;  // "key@level"
  bool whiteout_ = false;

  void adoptStarters(const LiveFrame &f, long long now, std::vector<Event> &ev);
  void newMembers(const LiveFrame &f, long long now, std::vector<Event> &ev);
  void syncParty(const LiveFrame &f, long long now, std::vector<Event> &ev);
  void deaths(const LiveFrame &f, long long now, std::vector<Event> &ev);
  void areaChange(const LiveFrame &f, long long now, std::vector<Event> &ev);
  void battle(const LiveFrame &f, long long now, std::vector<Event> &ev);
  void caps(const LiveFrame &f, long long now, std::vector<Event> &ev, bool trainerStart);
  bool familyCaught(int species) const;
  std::string monName(const Mon &m) const;
  RunMon &addMon(const Mon &m, long long now);
};

}  // namespace sd
