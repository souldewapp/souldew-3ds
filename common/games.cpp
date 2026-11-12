#include "games.h"

#include <cstring>

namespace sd {

// Level caps: the ace of the boss who gives the next milestone. X/Y and Omega Ruby/Alpha Sapphire: the gym leaders'
// aces (first fight) and the champion's ace. Sun/Moon and Ultra Sun/Ultra Moon have no badges: the four grand trials
// (kahunas), then the Elite Four and the champion – the player steps the milestone in souldew's screen, and every cap
// can be changed in the run's rules (the numbers are the vanilla games' and are a starting point).
static const int capsXY[] = {12, 25, 32, 34, 37, 42, 48, 59};
static const char *const bossXY[] = {"Viola", "Grant", "Korrina", "Ramos", "Clemont", "Valerie", "Olympia", "Wulfric"};
static const int capsORAS[] = {14, 16, 20, 28, 30, 35, 45, 46};
static const char *const bossORAS[] = {"Roxanne", "Brawly", "Wattson", "Flannery", "Norman", "Winona", "Tate & Liza", "Wallace"};
static const int capsSM[] = {15, 27, 39, 48};
static const char *const bossSM[] = {"Hala", "Olivia", "Nanu", "Hapu"};
static const int capsUSUM[] = {16, 28, 43, 54};
static const char *const bossUSUM[] = {"Hala", "Olivia", "Nanu", "Hapu"};

static const GameInfo GAMES[] = {
    {GameId::XY, "xy", "Pok\xc3\xa9mon X / Y", "X / Y", 6,
     {0x0004000000055D00ull, 0x0004000000055E00ull}, {"X", "Y"},
     2, {{"1.5", 0x08CE1CF8, 0x08C6A6B0}, {"1.0", 0x08CE1CE8, 0x08C6A6A0}},
     {0x08203ED0, 0x082059E0, 580, 12, 0, 0}, 0x081828EC,
     {8, capsXY, bossXY, 68, "Champion", true}, true},
    {GameId::ORAS, "oras", "Pok\xc3\xa9mon Omega Ruby / Alpha Sapphire", "OR / AS", 6,
     {0x000400000011C400ull, 0x000400000011C500ull}, {"Omega Ruby", "Alpha Sapphire"},
     2, {{"1.4", 0x08CFB26C, 0x08C71DC4}, {"1.0", 0x08CF727C, 0x08C6DDD4}},
     {0x082041FC, 0x08205D0C, 580, 12, 0, 0}, 0x08187BD4,
     {8, capsORAS, bossORAS, 59, "Champion", true}, true},
    {GameId::SM, "sm", "Pok\xc3\xa9mon Sun / Moon", "Sun / Moon", 7,
     {0x0004000000164800ull, 0x0004000000175E00ull}, {"Sun", "Moon"},
     1, {{nullptr, 0x34195E10, 0}, {nullptr, 0, 0}},
     {0x30002770, 0, 816, 24, 0x341C8CCD, 0x341C8CAC}, 0x34183CC6,
     {4, capsSM, bossSM, 57, "Champion", false}, true},
    {GameId::USUM, "usum", "Pok\xc3\xa9mon Ultra Sun / Ultra Moon", "US / UM", 7,
     {0x00040000001B5000ull, 0x00040000001B5100ull}, {"Ultra Sun", "Ultra Moon"},
     1, {{nullptr, 0x33F7FA44, 0}, {nullptr, 0, 0}},
     {0x30002770, 0, 816, 24, 0x33FC1F65, 0x33FC1F44}, 0x33F6D74A,
     {4, capsUSUM, bossUSUM, 60, "Champion", false}, true},
};

int gameCount() { return (int)(sizeof(GAMES) / sizeof(GAMES[0])); }
const GameInfo *gameAt(int i) { return (i >= 0 && i < gameCount()) ? &GAMES[i] : nullptr; }

const GameInfo *gameById(GameId id) {
  for (auto &g : GAMES)
    if (g.id == id) return &g;
  return nullptr;
}

const GameInfo *gameByKey(const char *key) {
  if (!key) return nullptr;
  for (auto &g : GAMES)
    if (!std::strcmp(g.key, key)) return &g;
  return nullptr;
}

const GameInfo *gameByTitleId(uint64_t tid, int *versionIndex) {
  for (auto &g : GAMES)
    for (int i = 0; i < 2; i++)
      if (g.titleIds[i] == tid) {
        if (versionIndex) *versionIndex = i;
        return &g;
      }
  return nullptr;
}

}  // namespace sd
