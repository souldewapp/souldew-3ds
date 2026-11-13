// Achievements ("trophies") and medals – the desktop app's (shared/trophies.mjs), cut down to what a solo Nuzlocke in
// the 3DS games can show and the plugin can actually observe: catches, deaths with their killer, badges with their
// time, evolutions, the run's end, the last party. Names and descriptions follow the desktop app's texts (de/en).
//
// Every achievement is computed from the run file (all runs + the profile): `evaluateTrophies` gives the progress,
// `unlockTrophies` records the newly reached ones in the profile (kept even when a run is deleted later) and returns
// them – the plugin shows a card for each, the launcher a banner.
#pragma once
#include <string>
#include <vector>
#include "run.h"

namespace sd {

enum class TCat { Journey = 0, Collection, Skill, Fun };
constexpr int kTCats = 4;
enum class Rarity { Common = 0, Uncommon, Rare, Epic, Legendary };

struct TrophyDef {
  const char *id;
  TCat cat;
  Rarity rarity;
  int target;   // counted achievements: the goal (1 = a single moment)
  bool hidden;  // secret until unlocked
  const char *nameDe, *nameEn, *descDe, *descEn;  // descriptions take %d = target
};

struct TrophyState {
  const TrophyDef *def;
  int n = 0;           // progress (≤ target)
  bool ok = false;     // unlocked
  long long at = 0;    // when (profile)
};

int trophyCount();
const TrophyDef &trophyAt(int i);
const TrophyDef *trophyById(const std::string &id);
std::string trophyName(const TrophyDef &d);
std::string trophyDesc(const TrophyDef &d);
int trophyPoints(Rarity r);
const char *rarityLabel(Rarity r);
const char *catLabel(TCat c);

std::vector<TrophyState> evaluateTrophies(const RunStore &st);
// records the newly reached achievements (and the owned species) in st.profile; returns them in list order
std::vector<const TrophyDef *> unlockTrophies(RunStore &st, long long now);

// medals: one per category (by the share unlocked) + the champion's medal over everything.
// tier: -1 none, 0 bronze, 1 silver, 2 gold, 3 platinum, 4 master
struct MedalState {
  int kind;  // 0..3 = TCat, 4 = champion
  int tier = -1;
  int value = 0, total = 0;
  int next = 0;  // the value of the next tier (0: highest reached)
};
std::vector<MedalState> medalStates(const std::vector<TrophyState> &states);
const char *tierLabel(int tier);
const char *medalName(int kind);

struct TrophyTotals {
  int unlocked = 0, total = 0, points = 0, maxPoints = 0, species = 0;
};
TrophyTotals trophyTotals(const RunStore &st, const std::vector<TrophyState> &states);

}  // namespace sd
