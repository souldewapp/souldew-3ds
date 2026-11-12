// The Nintendo 3DS Pokémon games souldew knows: title IDs and the memory addresses its live data comes from.
// Addresses: the souldew desktop app's native/shared/data/addresses.json (platform "3ds") and research/azahar.md –
// Citra-Tracker v2 (kcblack42), Pokémon Stream Tool (Readek), PokeReader (zaksabeast), PKMN-NTR (zone), checked live
// by souldew in Azahar with retail cartridges.
#pragma once
#include <cstdint>

namespace sd {

enum class GameId { None = 0, XY, ORAS, SM, USUM };

struct PartyAddr {
  const char *version;  // update version ("1.5"), nullptr = any
  uint32_t party;
  uint32_t badges;      // 0 = not known
};

struct BattleAddrs {
  uint32_t wild, trainer;     // battle bases (Gen 6: one per kind; Gen 7: one for both)
  int stride;                 // bytes per battler (580 Gen 6, 816 Gen 7)
  int blocks;                 // battlers read
  uint32_t inBattle, isTrainer;  // Gen 7 flags (0 = none)
};

struct LevelCaps {
  int count;            // milestones (badges / grand trials)
  const int *caps;      // cap before milestone n (n = 0 … count-1)
  const char *const *bosses;  // who gives milestone n
  int finalCap;         // after all milestones (Elite Four + champion)
  const char *finalBoss;
  bool fromBadges;      // true: the badge count comes from memory (Gen 6); false: the player steps it (Gen 7)
};

struct GameInfo {
  GameId id;
  const char *key;          // "xy", "oras", "sm", "usum" (run.json)
  const char *name;         // "Pokémon X / Y"
  const char *shortName;    // "X / Y"
  int gen;
  uint64_t titleIds[2];
  const char *versionNames[2];  // per title ID
  int partyAddrCount;
  PartyAddr partyAddrs[2];
  BattleAddrs battle;
  uint32_t zoneAddr;        // u16 current zone
  LevelCaps caps;
  bool live;                // memory addresses known (overlays work)
};

const GameInfo *gameById(GameId id);
const GameInfo *gameByKey(const char *key);
const GameInfo *gameByTitleId(uint64_t tid, int *versionIndex = nullptr);
int gameCount();
const GameInfo *gameAt(int i);

}  // namespace sd
