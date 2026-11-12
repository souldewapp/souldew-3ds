// Gen 6 / 7 party and battle data in the running game's memory – a port of the souldew desktop app's
// native/shared/live/gen67.mjs (PK6/PK7 crypto after PKHeX PokeCrypto; layouts in that file's header).
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "games.h"

namespace sd {

constexpr int SLOT_STRIDE = 0x1E4;
constexpr int BOX_SIZE = 0xE8;       // 232
constexpr int STATS_OFFSET = 0x158;  // party stats inside a RAM slot
constexpr int STATS_SIZE = 0x1C;
constexpr int PK_SIZE = BOX_SIZE + STATS_SIZE;  // 260
constexpr int PARTY_BYTES = 6 * SLOT_STRIDE;

// reads `len` bytes of the game's memory at `addr`; false if unreadable
using MemRead = bool (*)(uint32_t addr, void *out, uint32_t len, void *ctx);

struct Mon {
  std::string key;  // "pid-tid-sid" (hex pid) – the same key the desktop app uses
  uint32_t pid = 0, ec = 0;
  int species = 0, form = 0, item = 0;
  std::string nickname;  // UTF-8
  bool isEgg = false, shiny = false;
  int moves[4] = {0, 0, 0, 0};
  int metLocation = 0, eggLocation = 0, metLevel = 0;
  char gender = '-';  // 'M' 'F' '-'
  int status = 0;     // Gen 3–5 style code (0x40 par, 1 slp, 0x20 frz, 0x10 brn, 0x08 psn)
  int level = 0, hp = 0, maxHp = 0;
};

bool decryptPK6(const uint8_t *src, int len, uint8_t *out);  // returns checksum ok; len 232 or 260
void encryptPK6(const uint8_t *plain, int len, uint8_t *out); // writes the checksum (tests, the mock game)
bool parsePK6(const uint8_t *raw260, int gen, Mon &out);

// the party in a 6-slot region; empty vector = none (title screen, wrong address, slot being rewritten)
struct BadEggState {
  uint8_t sig[6][16];
  int n[6] = {0, 0, 0, 0, 0, 0};
};
std::vector<Mon> partyFromRegion(const uint8_t *region, int gen, BadEggState *bad);

struct PartyRead {
  bool ok = false;
  std::vector<Mon> party;
  int badges = -1;  // -1 unknown
  const char *version = nullptr;
};

// keeps the address that worked and the "shrink only after two reads" rule
struct PartyReaderState {
  int addrIndex = -1;
  int lastLen = 0;
  std::string shrinkSig;
  BadEggState bad[2];
};

PartyRead readParty(const GameInfo &g, MemRead read, void *ctx, PartyReaderState &st);

struct BattleRead {
  bool inBattle = false;
  bool trainer = false;
  int enemySpecies = 0, enemyLevel = 0, enemyHp = 0, enemyMaxHp = 0;
  int activeIndex = -1;  // party slot of the player's battler, -1 unknown
  int activeHp = 0, activeMaxHp = 0;
  int activeSpecies = 0, activeLevel = 0;
};

BattleRead readBattle(const GameInfo &g, MemRead read, void *ctx, const std::vector<Mon> *party);

int readZone(const GameInfo &g, MemRead read, void *ctx);  // -1 unknown

}  // namespace sd
