// A souldew run on the 3DS: the Nuzlocke state the launcher and the in-game plugin share through one JSON file on the
// SD card (sdmc:/souldew/runs.json). Field names follow the desktop app's run (rules: dupes, levelCaps, blackout,
// gifts, deathless …) so a later import/export stays straightforward.
#pragma once
#include <string>
#include <vector>
#include "json.h"
#include "data.h"

namespace sd {

constexpr const char *kRunsPath = "/souldew/runs.json";

struct Rules {
  std::string dupes = "line";      // off | species | line (dead ones count too)
  std::string levelCaps = "warn";  // off | warn | strict
  std::string blackout = "party";  // party (a whiteout ends the run) | off
  std::string gifts = "area";      // area (a gift is that area's encounter) | free
  bool deathless = false;          // the first death ends the run
  bool shinyClause = true;         // shinies may always be caught (checked by hand: the battle copy has no PID)
  bool nicknames = false;
  bool noBattleItems = false;      // hardcore
  bool setMode = false;            // hardcore
  std::vector<int> caps;           // overrides the game's caps (empty: the game's)
  void applyPreset(const std::string &preset);  // "classic" | "hardcore"
  std::string presetName() const;               // "classic" | "hardcore" | "custom"
};

struct Encounter {
  std::string area;       // place key: the English location name (stable across languages)
  int loc = 0;            // met location id of that place
  int species = 0, level = 0;
  std::string status;     // open | caught | missed | dupe | gift
  std::string monKey;     // the caught Pokémon
  bool starter = false;
  long long ts = 0;
};

struct RunMon {
  std::string key;
  int species = 0;
  int firstSpecies = 0;   // what it was caught as (evolutions change `species`)
  std::string nickname;
  int level = 0, metLevel = 0, hp = 0, maxHp = 0;
  int loc = 0;
  std::string area;
  char gender = '-';
  bool shiny = false;
  bool dead = false;
  std::string cause;      // "Wurmple Lv 3" / "Kampf gegen Trainer" / "manuell"
  int killer = 0, killerLevel = 0;  // the Pokémon that knocked it out (0: unknown / by hand)
  bool killerTrainer = false;
  std::string deathArea;
  long long deathTs = 0;
  bool inParty = false;
  int partySlot = -1;
};

struct LogEntry {
  long long ts = 0;
  std::string kind;  // area | encounter | caught | missed | dupe | death | cap | badge | run
  std::string text;
};

// what the achievements need beyond encounters and Pokémon: moments the engine saw while the game ran
struct RunStats {
  std::vector<long long> badgeAt;  // when badge (Gen 7: grand trial) n+1 was earned
  int evolutions = 0;
  int oneHp = 0;          // a Pokémon of the team stood at exactly 1 HP
  int underdog = 0;       // a badge earned while the strongest Pokémon was below the leader's ace
  int capBreaks = 0;      // a trainer battle started with a Pokémon over the level cap
  long long monotypeAt = 0;  // a full team of six sharing one type
  long long endedAt = 0;     // won / lost
};

struct Run {
  std::string id, name, game;  // game: "xy" | "oras" | "sm" | "usum"
  std::string mode = "nuzlocke";
  std::string status = "active";  // active | lost | won
  long long created = 0, updated = 0;
  Rules rules;
  int badges = 0;      // Gen 6: read from memory
  int milestone = 0;   // Gen 7: grand trials done (stepped by the player)
  int playMinutes = 0;
  std::string lastArea;
  std::vector<Encounter> encounters;
  std::vector<RunMon> mons;
  std::vector<LogEntry> log;
  RunStats stats;

  // derived
  const Encounter *encounterFor(const std::string &area) const;  // the area's counting encounter (not dupe)
  Encounter *encounterFor(const std::string &area);
  RunMon *mon(const std::string &key);
  const RunMon *mon(const std::string &key) const;
  int caught() const, missed() const, open() const, alive() const, dead() const;
  int levelCap(std::string *boss = nullptr) const;  // 0 = no cap (rule off / unknown game)
  int progress() const;  // badges or milestone
  void addLog(long long ts, const char *kind, const std::string &text);
  void end(const std::string &status, long long now);  // "won" | "lost" | "active" (reopen)
};

struct Settings {
  std::string lang = "de";
  bool overlays = true;
  bool tab = true;  // the small souldew tab on the bottom screen
  bool onboarded = false;  // the first-launch introduction was shown
};

// across runs: the achievements reached (kept even when a run is deleted), the species ever owned, runs started
struct Profile {
  std::vector<std::pair<std::string, long long>> trophies;  // id → when
  std::vector<int> dex;  // sorted species ids
  int runsCreated = 0;
  bool has(const std::string &id) const;
  long long when(const std::string &id) const;
  void addDex(int species);
};

struct RunStore {
  int version = 1;
  std::string active;
  std::vector<Run> runs;
  Settings settings;
  Profile profile;

  Run *activeRun();
  Run *byId(const std::string &id);
  Run &create(const std::string &name, const std::string &game, const Rules &rules, long long now);
  void remove(const std::string &id);

  Json toJson() const;
  static RunStore fromJson(const Json &j);
  bool parseText(const std::string &text);         // false: not a souldew-3ds run file (store unchanged)
  std::string toText() const;
  bool load(const std::string &path);              // false: no file / unreadable (store stays empty)
  bool save(const std::string &path) const;        // writes path.tmp, then renames
};

Lang langOf(const Settings &s);
// the place key of a met location: English name; the shown name follows the language
std::string areaKey(int gen, int loc);
std::string areaLabel(int gen, int loc, const std::string &key, Lang lang);

}  // namespace sd
