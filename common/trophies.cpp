#include "trophies.h"

#include <algorithm>
#include <cstdio>
#include "games.h"
#include "i18n.h"

namespace sd {

// clang-format off
static const TrophyDef DEFS[] = {
  // ---- journey -------------------------------------------------------------------------------------------------
  {"firstRun", TCat::Journey, Rarity::Common, 1, false, "Drück Start", "Press Start", "Starte deinen ersten Lauf.", "Start your first run."},
  {"firstCatch", TCat::Journey, Rarity::Common, 1, false, "Gefangen!", "Gotcha!", "Fang dein erstes Pokémon.", "Catch your first Pokémon."},
  {"catches50", TCat::Journey, Rarity::Uncommon, 50, false, "Ball-Hamster", "Ball Hoarder", "Fang %d Pokémon über alle Läufe hinweg.", "Catch %d Pokémon across all your runs."},
  {"catches250", TCat::Journey, Rarity::Epic, 250, false, "Box voll", "PC Box Overflow", "Fang %d Pokémon über alle Läufe hinweg.", "Catch %d Pokémon across all your runs."},
  {"firstBadge", TCat::Journey, Rarity::Common, 1, false, "Blech am Revers", "Shiny Pin", "Hol dir deinen ersten Orden (Alola: die erste große Prüfung).", "Earn your first badge (Alola: the first grand trial)."},
  {"badges8", TCat::Journey, Rarity::Rare, 8, false, "Acht von acht", "Eight for Eight", "Alle 8 Orden in einem Lauf (Alola: alle vier großen Prüfungen).", "All 8 badges in one run (Alola: all four grand trials)."},
  {"badges50", TCat::Journey, Rarity::Rare, 50, false, "Ordenssammler", "Badge Collector", "Hol dir %d Orden über alle Läufe hinweg.", "Earn %d badges across all your runs."},
  {"firstWin", TCat::Journey, Rarity::Rare, 1, false, "Ruhmeshalle", "Hall of Fame", "Beende einen Lauf als gewonnen.", "Finish a run as won."},
  {"wins5", TCat::Journey, Rarity::Epic, 5, false, "Alter Hase", "Seasoned Champion", "Gewinne %d Läufe.", "Win %d runs."},
  {"runs10", TCat::Journey, Rarity::Uncommon, 10, false, "Nochmal! Nochmal!", "Again! Again!", "Starte %d Läufe.", "Start %d runs."},
  {"bothGens", TCat::Journey, Rarity::Uncommon, 2, false, "Kalos und Alola", "Kalos and Alola", "Spiel einen Lauf in Gen 6 und einen in Gen 7.", "Play a run in Gen 6 and one in Gen 7."},
  {"level100", TCat::Journey, Rarity::Epic, 1, false, "Level 100", "Level 100", "Bring eines deiner Pokémon auf Level 100.", "Raise one of your Pokémon to level 100."},
  {"firstEvolution", TCat::Journey, Rarity::Common, 1, false, "Nanu? Es entwickelt sich!", "What? It's Evolving!", "Entwickle ein Pokémon.", "Evolve a Pokémon."},
  {"evolutions25", TCat::Journey, Rarity::Uncommon, 25, false, "Metamorphose", "Metamorphosis", "Entwickle %d Pokémon.", "Evolve %d Pokémon."},
  {"hardcoreWin", TCat::Journey, Rarity::Legendary, 1, false, "Hardcore-Legende", "Hardcore Legend", "Gewinne einen Nuzlocke mit den Hardcore-Regeln.", "Win a Nuzlocke with the Hardcore rules."},
  // ---- collection ----------------------------------------------------------------------------------------------
  {"dex10", TCat::Collection, Rarity::Common, 10, false, "Feldnotizen", "Field Notes", "Besitze %d verschiedene Arten.", "Own %d different species."},
  {"dex50", TCat::Collection, Rarity::Uncommon, 50, false, "Feldforscher", "Field Researcher", "Besitze %d verschiedene Arten.", "Own %d different species."},
  {"dex100", TCat::Collection, Rarity::Rare, 100, false, "Hundertschaft", "Centurion", "Besitze %d verschiedene Arten.", "Own %d different species."},
  {"dex250", TCat::Collection, Rarity::Epic, 250, false, "Stolz des Professors", "Professor's Pride", "Besitze %d verschiedene Arten.", "Own %d different species."},
  {"dexKalos", TCat::Collection, Rarity::Epic, 69, false, "Kalos komplett", "Kalos Complete", "Besitze jede Art von #650 bis #721 – ohne Event-Pokémon.", "Own every species from #650 to #721 – event-only Pokémon excluded."},
  {"dexAlola", TCat::Collection, Rarity::Epic, 83, false, "Alola komplett", "Alola Complete", "Besitze jede Art von #722 bis #807 – ohne Event-Pokémon.", "Own every species from #722 to #807 – event-only Pokémon excluded."},
  {"firstShiny", TCat::Collection, Rarity::Rare, 1, false, "Da glitzert was", "Something Sparkly", "Fang ein schillerndes Pokémon.", "Catch a shiny Pokémon."},
  {"shiny5", TCat::Collection, Rarity::Legendary, 5, false, "Shiny-Jäger", "Shiny Hunter", "Fang %d schillernde Pokémon.", "Catch %d shiny Pokémon."},
  {"legendary", TCat::Collection, Rarity::Rare, 1, false, "Wie die Legende sagt", "Legend Has It", "Besitze ein legendäres oder mysteriöses Pokémon.", "Own a legendary or mythical Pokémon."},
  {"legendary5", TCat::Collection, Rarity::Epic, 5, false, "Mythenjäger", "Myth Buster", "Besitze %d verschiedene legendäre Pokémon.", "Own %d different legendary Pokémon."},
  {"starterTrio", TCat::Collection, Rarity::Uncommon, 1, false, "Starterpaket", "Starter Pack", "Besitze alle drei Starter einer Generation.", "Own all three starters of one generation."},
  {"eevee", TCat::Collection, Rarity::Rare, 5, false, "Evoli-Fan", "Eevee Enthusiast", "Besitze %d verschiedene Entwicklungen von Evoli.", "Own %d different Eevee evolutions."},
  {"fossil", TCat::Collection, Rarity::Uncommon, 1, false, "Jurassic Pokémon", "Jurassic Pokémon", "Besitze ein Fossil-Pokémon.", "Own a fossil Pokémon."},
  {"allTypes", TCat::Collection, Rarity::Rare, 17, false, "Typensammler", "Type Collector", "Besitze Pokémon aller %d klassischen Typen.", "Own Pokémon of all %d classic types."},
  {"zubat", TCat::Collection, Rarity::Uncommon, 5, false, "Nicht schon wieder Zubat", "Not Another Zubat", "Fang in %d verschiedenen Läufen ein Zubat.", "Catch a Zubat in %d different runs."},
  {"gyarados", TCat::Collection, Rarity::Uncommon, 1, false, "Das Platschern hat sich gelohnt", "Worth the Splash", "Entwickle ein Karpador zu Garados.", "Evolve a Magikarp into Gyarados."},
  {"feebas", TCat::Collection, Rarity::Rare, 1, false, "Sechs Felder Geduld", "Six Tiles of Patience", "Fang ein Barschwa.", "Catch a Feebas."},
  {"helix", TCat::Collection, Rarity::Uncommon, 1, true, "Lobet den Helix", "Praise Helix", "Besitze ein Amonitas oder Amoroso.", "Own an Omanyte or Omastar."},
  // ---- skill ---------------------------------------------------------------------------------------------------
  {"cleanBadge", TCat::Skill, Rarity::Uncommon, 1, false, "Makelloser Sieg", "Flawless Victory", "Hol dir einen Orden ohne einen Tod seit dem letzten.", "Earn a badge without a single death since the one before."},
  {"cleanFour", TCat::Skill, Rarity::Rare, 1, false, "Weiße Weste", "Clean Sheet", "Hol dir die ersten vier Orden eines Laufs ohne einen Tod.", "Earn the first four badges of a run without a death."},
  {"deathlessWin", TCat::Skill, Rarity::Legendary, 1, false, "Unantastbar", "Untouchable", "Gewinne einen Lauf ohne einen einzigen Tod.", "Win a run without a single death."},
  {"lowDeathWin", TCat::Skill, Rarity::Epic, 1, false, "Knapp war's", "Close Call Club", "Gewinne einen Lauf mit höchstens drei Toden.", "Win a run with three deaths or fewer."},
  {"underdog", TCat::Skill, Rarity::Rare, 1, false, "Außenseiter", "Underdog", "Hol dir einen Orden, während dein stärkstes Pokémon unter dem Level-Cap liegt.", "Earn a badge while your strongest Pokémon is below the level cap."},
  {"underdog3", TCat::Skill, Rarity::Epic, 3, false, "David gegen Goliath", "David vs. Goliath", "Hol dir %d Orden unterlevelt.", "Earn %d badges underleveled."},
  {"oneHp", TCat::Skill, Rarity::Rare, 1, false, "Am seidenen Faden", "Hanging by a Thread", "Lass eines deiner Pokémon mit genau 1 KP überleben.", "Have a Pokémon survive at exactly 1 HP."},
  {"veteran", TCat::Skill, Rarity::Rare, 1, false, "Treue Seele", "Old Reliable", "Lass ein Pokémon nach seinem Fang 40 Level aufsteigen.", "Have a Pokémon gain 40 levels after you caught it."},
  {"starterSurvives", TCat::Skill, Rarity::Rare, 1, false, "Von Anfang an", "Day One", "Gewinne einen Lauf, in dem dein Starter noch lebt.", "Win a run with your starter still alive."},
  {"monotype", TCat::Skill, Rarity::Epic, 1, false, "Ein Typ, ein Team", "One-Type Wonder", "Stell ein volles Team aus sechs auf, das sich einen Typ teilt.", "Field a full team of six that shares one type."},
  {"byTheBook", TCat::Skill, Rarity::Epic, 1, false, "Nach Vorschrift", "By the Book", "Gewinne mit Level-Caps – ohne einen einzigen zu brechen.", "Win with level caps on – and never break one."},
  {"fullTeamWin", TCat::Skill, Rarity::Uncommon, 1, false, "Volle Truppe", "Squad Goals", "Gewinne einen Lauf mit einem vollen Team aus sechs.", "Win a run with a full team of six."},
  {"phoenix", TCat::Skill, Rarity::Epic, 1, false, "Phönix aus der Asche", "Phoenix", "Gewinne einen Lauf trotz zehn oder mehr Toden.", "Win a run despite losing ten or more Pokémon."},
  // ---- fun -----------------------------------------------------------------------------------------------------
  {"magikarp", TCat::Fun, Rarity::Epic, 1, true, "Karpador-Moment", "It's a Magikarp Moment", "Verliere ein Pokémon an ein Karpador.", "Lose a Pokémon to a Magikarp."},
  {"rattata", TCat::Fun, Rarity::Uncommon, 1, false, "Top-Prozent-Rattfratz", "Top Percentage", "Verliere ein Pokémon an ein Rattfratz oder Rattikarl.", "Lose a Pokémon to a Rattata or Raticate."},
  {"rival", TCat::Fun, Rarity::Uncommon, 1, false, "Rivale 1, du 0", "Rival Supremacy", "Verliere vor dem ersten Orden ein Pokémon an den Starter eines Trainers.", "Lose a Pokémon to a trainer's starter before the first badge."},
  {"starterDown", TCat::Fun, Rarity::Uncommon, 1, false, "Starter nicht enthalten", "Starter Not Included", "Verliere deinen Starter vor dem ersten Orden.", "Lose your starter before the first badge."},
  {"nuzlose", TCat::Fun, Rarity::Rare, 5, false, "Nuzlocke? Eher Nuz-lost", "Nuzlocke? More Like Nuzlose", "Verliere %d Läufe.", "Lose %d runs."},
  {"skillIssue", TCat::Fun, Rarity::Rare, 1, true, "Skill Issue", "Skill Issue", "Lass einen Lauf an einem Trainer-Pokémon unter Level 10 enden.", "Have a run end to a trainer's Pokémon below level 10."},
  {"speedrun", TCat::Fun, Rarity::Rare, 1, true, "Speedrun (Any%)", "Speedrun (Any%)", "Verliere einen Lauf in den ersten 30 Minuten.", "Lose a run within 30 minutes of starting it."},
  {"cemetery", TCat::Fun, Rarity::Uncommon, 50, false, "Pokémon-Turm", "Pokémon Tower", "Verliere insgesamt %d Pokémon.", "Lose %d Pokémon in total."},
  {"lavender", TCat::Fun, Rarity::Epic, 100, false, "Wohnhaft in Lavandia", "Lavender Town Resident", "Verliere insgesamt %d Pokémon.", "Lose %d Pokémon in total."},
  {"butterfingers", TCat::Fun, Rarity::Uncommon, 10, false, "Butterfinger", "Butterfingers", "Lass %d Begegnungen entkommen.", "Let %d encounters get away."},
  {"kaboom", TCat::Fun, Rarity::Uncommon, 1, false, "Kawumm", "Kaboom", "Verliere ein Pokémon an ein wildes Kleinstein, Georok, Voltobal oder Lektrobal.", "Lose a Pokémon to a wild Geodude, Graveler, Voltorb or Electrode."},
  {"teleport", TCat::Fun, Rarity::Common, 1, false, "Abra setzt Teleport ein", "Abra Used Teleport", "Lass ein wildes Abra entkommen.", "Let a wild Abra get away."},
  {"garchomp", TCat::Fun, Rarity::Uncommon, 1, false, "Im Schatten des Champs", "Champion's Shadow", "Verliere ein Pokémon an das Knakrack eines Trainers.", "Lose a Pokémon to a trainer's Garchomp."},
  {"shinyDown", TCat::Fun, Rarity::Rare, 1, true, "Erst Glanz, dann Grab", "Sparkle, Then Darkness", "Verliere ein schillerndes Pokémon.", "Lose a shiny Pokémon."},
  {"tooMuchWater", TCat::Fun, Rarity::Rare, 6, false, "Zu viel Wasser", "Too Much Water", "Fang %d Wasser-Pokémon in einem Hoenn-Lauf (OR/AS).", "Catch %d Water-type Pokémon in one Hoenn run (OR/AS)."},
  {"birdJesus", TCat::Fun, Rarity::Rare, 1, false, "Bird Jesus", "Bird Jesus", "Gewinne einen Lauf mit Tauboss im finalen Team.", "Win a run with Pidgeot in your final team."},
  {"nightOwl", TCat::Fun, Rarity::Uncommon, 1, true, "Nachteule", "Night Owl", "Fang ein Pokémon zwischen 3 und 5 Uhr morgens.", "Catch a Pokémon between 3 and 5 a.m."},
};
// clang-format on
constexpr int N_DEFS = sizeof DEFS / sizeof DEFS[0];

int trophyCount() { return N_DEFS; }
const TrophyDef &trophyAt(int i) { return DEFS[i]; }
const TrophyDef *trophyById(const std::string &id) {
  for (auto &d : DEFS)
    if (id == d.id) return &d;
  return nullptr;
}
std::string trophyName(const TrophyDef &d) { return tr(d.nameDe, d.nameEn); }
std::string trophyDesc(const TrophyDef &d) { return fmt(tr(d.descDe, d.descEn), d.target); }
int trophyPoints(Rarity r) {
  static const int P[] = {10, 25, 50, 100, 250};
  return P[(int)r];
}
const char *rarityLabel(Rarity r) {
  switch (r) {
    case Rarity::Common: return tr("Gewöhnlich", "Common");
    case Rarity::Uncommon: return tr("Ungewöhnlich", "Uncommon");
    case Rarity::Rare: return tr("Selten", "Rare");
    case Rarity::Epic: return tr("Episch", "Epic");
    default: return tr("Legendär", "Legendary");
  }
}
const char *catLabel(TCat c) {
  switch (c) {
    case TCat::Journey: return tr("Reise", "Journey");
    case TCat::Collection: return tr("Sammlung", "Collection");
    case TCat::Skill: return tr("Können", "Skill");
    default: return tr("Spaß", "Fun");
  }
}
const char *tierLabel(int t) {
  switch (t) {
    case 0: return "Bronze";
    case 1: return tr("Silber", "Silver");
    case 2: return "Gold";
    case 3: return tr("Platin", "Platinum");
    case 4: return tr("Meister", "Master");
    default: return tr("noch keiner", "none yet");
  }
}
const char *medalName(int kind) {
  switch (kind) {
    case 0: return tr("Reise-Orden", "Journey Medal");
    case 1: return tr("Sammler-Orden", "Collector's Medal");
    case 2: return tr("Ass-Orden", "Ace Medal");
    case 3: return tr("Narren-Orden", "Jester's Medal");
    default: return tr("Champ-Orden", "Champion's Medal");
  }
}

// ---- the facts the rules read ------------------------------------------------------------------------------------

static bool isLegendary(int id) {
  static const int L[] = {144, 145, 146, 150, 151, 243, 244, 245, 249, 250, 251, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386,
                          480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 638, 639, 640, 641, 642, 643,
                          644, 645, 646, 647, 648, 649, 716, 717, 718, 719, 720, 721, 772, 773, 785, 786, 787, 788, 789, 790, 791,
                          792, 793, 794, 795, 796, 797, 798, 799, 800, 801, 802, 803, 804, 805, 806, 807};
  return std::binary_search(std::begin(L), std::end(L), id);
}
static bool eventOnly(int id) {
  return id == 151 || id == 251 || id == 385 || id == 386 || (id >= 489 && id <= 494) || (id >= 647 && id <= 649) ||
         (id >= 719 && id <= 721) || id == 801 || id == 802 || id == 807;
}
static bool isFossil(int id) {
  static const int F[] = {138, 139, 140, 141, 142, 345, 346, 347, 348, 408, 409, 410, 411, 564, 565, 566, 567, 696, 697, 698, 699};
  return std::find(std::begin(F), std::end(F), id) != std::end(F);
}
static bool starterLine(int id) {
  static const int S[] = {1, 4, 7, 152, 155, 158, 252, 255, 258, 387, 390, 393, 495, 498, 501, 650, 653, 656, 722, 725, 728};
  for (int s : S)
    if (id >= s && id <= s + 2) return true;
  return false;
}

struct Facts {
  const RunStore *st;
  std::vector<int> dex;  // owned species (profile + the runs), sorted
  int catches = 0, deaths = 0, missed = 0, wins = 0, lost = 0, badgesSum = 0, bestBadges = 0, evolutions = 0;
  int shinies = 0, underdog = 0;
  bool own(int id) const { return std::binary_search(dex.begin(), dex.end(), id); }
};

static int gameGen(const Run &r) {
  const GameInfo *g = gameByKey(r.game.c_str());
  return g ? g->gen : 6;
}
// badges in "8-badge units": Gen 7 has four grand trials
static int badgeUnits(const Run &r) { return gameGen(r) >= 7 ? r.milestone * 2 : r.badges; }
static bool counts(const Encounter &e) { return !e.starter && (e.status == "caught" || e.status == "gift"); }
static int deathsOf(const Run &r) {
  int n = 0;
  for (auto &m : r.mons) n += m.dead ? 1 : 0;
  return n;
}
static int finalParty(const Run &r, int species) {  // how many of the last party (alive) are `species` (0: all)
  int n = 0;
  for (auto &m : r.mons)
    if (m.inParty && !m.dead && (!species || m.species == species)) n++;
  return n;
}
static const RunMon *starterOf(const Run &r) {
  for (auto &e : r.encounters)
    if (e.starter) return r.mon(e.monKey);
  return nullptr;
}
static bool beforeFirstBadge(const Run &r, long long ts) { return r.stats.badgeAt.empty() || (ts && ts < r.stats.badgeAt[0]); }

static Facts facts(const RunStore &st) {
  Facts f;
  f.st = &st;
  f.dex = st.profile.dex;
  for (auto &r : st.runs) {
    for (auto &m : r.mons) {
      for (int s : {m.species, m.firstSpecies})
        if (s > 0 && !std::binary_search(f.dex.begin(), f.dex.end(), s)) f.dex.insert(std::lower_bound(f.dex.begin(), f.dex.end(), s), s);
      if (m.dead) f.deaths++;
      if (m.shiny) f.shinies++;
    }
    for (auto &e : r.encounters) {
      if (counts(e)) f.catches++;
      if (e.status == "missed") f.missed++;
    }
    if (r.status == "won") f.wins++;
    if (r.status == "lost") f.lost++;
    f.badgesSum += gameGen(r) >= 7 ? r.milestone : r.badges;
    f.bestBadges = std::max(f.bestBadges, badgeUnits(r));
    f.evolutions += r.stats.evolutions;
    f.underdog += r.stats.underdog;
  }
  return f;
}

// a badge with no death of the run since the one before (or the start, with a team to keep alive first)
static int cleanBadges(const Run &r, int upTo) {
  int clean = 0;
  for (size_t n = 0; n < r.stats.badgeAt.size() && (int)n < upTo; n++) {
    long long end = r.stats.badgeAt[n], start = n == 0 ? r.created : r.stats.badgeAt[n - 1];
    bool died = false;
    for (auto &m : r.mons)
      if (m.dead && m.deathTs > start && m.deathTs <= end) died = true;
    if (!died) clean++;
  }
  return clean;
}

static int progress(const TrophyDef &d, const Facts &f) {
  const RunStore &st = *f.st;
  std::string id = d.id;
  auto anyRun = [&](auto pred) {
    for (auto &r : st.runs)
      if (pred(r)) return 1;
    return 0;
  };
  auto anyMon = [&](auto pred) {
    for (auto &r : st.runs)
      for (auto &m : r.mons)
        if (pred(r, m)) return 1;
    return 0;
  };
  auto won = [](const Run &r) { return r.status == "won"; };
  auto killedBy = [&](auto pred) { return anyMon([&](const Run &, const RunMon &m) { return m.dead && m.killer && pred(m); }); };
  if (id == "firstRun" || id == "runs10") return std::max(st.profile.runsCreated, (int)st.runs.size());
  if (id == "firstCatch" || id == "catches50" || id == "catches250") return f.catches;
  if (id == "firstBadge") return f.badgesSum > 0 ? 1 : 0;
  if (id == "badges8") return f.bestBadges;
  if (id == "badges50") return f.badgesSum;
  if (id == "firstWin" || id == "wins5") return f.wins;
  if (id == "bothGens") return anyRun([](const Run &r) { return gameGen(r) == 6; }) + anyRun([](const Run &r) { return gameGen(r) == 7; });
  if (id == "level100") return anyMon([](const Run &, const RunMon &m) { return m.level >= 100; });
  if (id == "firstEvolution" || id == "evolutions25") return f.evolutions;
  if (id == "hardcoreWin") return anyRun([&](const Run &r) { return won(r) && r.rules.presetName() == "hardcore"; });
  if (id.rfind("dex", 0) == 0 && id != "dexKalos" && id != "dexAlola") return (int)f.dex.size();
  if (id == "dexKalos" || id == "dexAlola") {
    int lo = id == "dexKalos" ? 650 : 722, hi = id == "dexKalos" ? 721 : 807, n = 0;
    for (int s : f.dex) n += (s >= lo && s <= hi && !eventOnly(s)) ? 1 : 0;
    return n;
  }
  if (id == "firstShiny" || id == "shiny5") return f.shinies;
  if (id == "legendary" || id == "legendary5") {
    int n = 0;
    for (int s : f.dex) n += isLegendary(s) ? 1 : 0;
    return n;
  }
  if (id == "starterTrio") {
    static const int S[] = {1, 152, 252, 387, 495, 650, 722};
    for (int s : S) {
      bool all = true;
      for (int k = 0; k < 3; k++) all = all && f.own(s + 3 * k);
      if (all) return 1;
    }
    return 0;
  }
  if (id == "eevee") {
    int n = 0;
    for (int s : {134, 135, 136, 196, 197, 470, 471, 700}) n += f.own(s) ? 1 : 0;
    return n;
  }
  if (id == "fossil") {
    for (int s : f.dex)
      if (isFossil(s)) return 1;
    return 0;
  }
  if (id == "allTypes") {
    bool seen[17] = {};
    for (int s : f.dex)
      for (int k = 0; k < 2; k++) {
        int t = speciesType(s, k);
        if (t >= 0 && t < 17) seen[t] = true;
      }
    return (int)std::count(std::begin(seen), std::end(seen), true);
  }
  if (id == "zubat") {
    int n = 0;
    for (auto &r : st.runs)
      for (auto &e : r.encounters)
        if (counts(e) && e.species == 41) {
          n++;
          break;
        }
    return n;
  }
  if (id == "gyarados") return anyMon([](const Run &, const RunMon &m) { return m.firstSpecies == 129 && m.species == 130; });
  if (id == "feebas") return anyMon([](const Run &, const RunMon &m) { return m.firstSpecies == 349; });
  if (id == "helix") return f.own(138) || f.own(139) ? 1 : 0;
  if (id == "cleanBadge") return anyRun([](const Run &r) { return cleanBadges(r, 99) > 0; });
  if (id == "cleanFour")
    return anyRun([](const Run &r) {
      int n = gameGen(r) >= 7 ? 2 : 4;  // Alola: the first two grand trials
      return (int)r.stats.badgeAt.size() >= n && cleanBadges(r, n) == n;
    });
  if (id == "deathlessWin") return anyRun([&](const Run &r) { return won(r) && deathsOf(r) == 0 && !r.mons.empty(); });
  if (id == "lowDeathWin") return anyRun([&](const Run &r) { return won(r) && deathsOf(r) <= 3 && !r.mons.empty(); });
  if (id == "underdog" || id == "underdog3") return f.underdog;
  if (id == "oneHp") return anyRun([](const Run &r) { return r.stats.oneHp > 0; });
  if (id == "veteran") return anyMon([](const Run &, const RunMon &m) { return m.metLevel > 0 && m.level - m.metLevel >= 40; });
  if (id == "starterSurvives")
    return anyRun([&](const Run &r) {
      const RunMon *s = starterOf(r);
      return won(r) && s && !s->dead;
    });
  if (id == "monotype") return anyRun([](const Run &r) { return r.stats.monotypeAt > 0; });
  if (id == "byTheBook") return anyRun([&](const Run &r) { return won(r) && r.rules.levelCaps != "off" && r.stats.capBreaks == 0; });
  if (id == "fullTeamWin") return anyRun([&](const Run &r) { return won(r) && finalParty(r, 0) >= 6; });
  if (id == "phoenix") return anyRun([&](const Run &r) { return won(r) && deathsOf(r) >= 10; });
  if (id == "magikarp") return killedBy([](const RunMon &m) { return m.killer == 129; });
  if (id == "rattata") return killedBy([](const RunMon &m) { return m.killer == 19 || m.killer == 20; });
  if (id == "rival")
    return anyMon([](const Run &r, const RunMon &m) {
      return m.dead && m.killerTrainer && starterLine(m.killer) && m.killerLevel <= 12 && beforeFirstBadge(r, m.deathTs);
    });
  if (id == "starterDown")
    return anyRun([](const Run &r) {
      const RunMon *s = starterOf(r);
      return s && s->dead && beforeFirstBadge(r, s->deathTs);
    });
  if (id == "nuzlose") return f.lost;
  if (id == "skillIssue")
    return anyRun([](const Run &r) {
      if (r.status != "lost") return false;
      const RunMon *last = nullptr;
      for (auto &m : r.mons)
        if (m.dead && (!last || m.deathTs > last->deathTs)) last = &m;
      return last && last->killerTrainer && last->killer && last->killerLevel < 10;
    });
  if (id == "speedrun")
    return anyRun([](const Run &r) { return r.status == "lost" && r.stats.endedAt && r.created && r.stats.endedAt - r.created < 30 * 60; });
  if (id == "cemetery" || id == "lavender") return f.deaths;
  if (id == "butterfingers") return f.missed;
  if (id == "kaboom") return killedBy([](const RunMon &m) { return !m.killerTrainer && (m.killer == 74 || m.killer == 75 || m.killer == 100 || m.killer == 101); });
  if (id == "teleport")
    return anyRun([](const Run &r) {
      for (auto &e : r.encounters)
        if (e.status == "missed" && e.species == 63) return true;
      return false;
    });
  if (id == "garchomp") return killedBy([](const RunMon &m) { return m.killerTrainer && m.killer == 445; });
  if (id == "shinyDown") return anyMon([](const Run &, const RunMon &m) { return m.dead && m.shiny; });
  if (id == "tooMuchWater") {
    int best = 0;
    for (auto &r : st.runs) {
      if (r.game != "oras") continue;
      int n = 0;
      for (auto &e : r.encounters)
        if (counts(e) && (speciesType(e.species, 0) == 10 || speciesType(e.species, 1) == 10)) n++;
      best = std::max(best, n);
    }
    return best;
  }
  if (id == "birdJesus") return anyRun([&](const Run &r) { return won(r) && finalParty(r, 18) > 0; });
  if (id == "nightOwl")
    return anyRun([](const Run &r) {
      for (auto &e : r.encounters) {
        long long h = (e.ts % 86400) / 3600;  // the console's clock is local time
        if (counts(e) && e.ts > 0 && (h == 3 || h == 4)) return true;
      }
      return false;
    });
  return 0;
}

std::vector<TrophyState> evaluateTrophies(const RunStore &st) {
  Facts f = facts(st);
  std::vector<TrophyState> out;
  out.reserve(N_DEFS);
  for (auto &d : DEFS) {
    TrophyState s;
    s.def = &d;
    int n = progress(d, f);
    s.at = st.profile.when(d.id);
    s.ok = n >= d.target || st.profile.has(d.id);
    s.n = s.ok ? d.target : std::max(0, std::min(n, d.target));
    out.push_back(s);
  }
  return out;
}

std::vector<const TrophyDef *> unlockTrophies(RunStore &st, long long now) {
  for (auto &r : st.runs)
    for (auto &m : r.mons) {
      st.profile.addDex(m.species);
      st.profile.addDex(m.firstSpecies);
    }
  std::vector<const TrophyDef *> fresh;
  Facts f = facts(st);
  for (auto &d : DEFS) {
    if (st.profile.has(d.id)) continue;
    if (progress(d, f) >= d.target) {
      st.profile.trophies.push_back({d.id, now});
      fresh.push_back(&d);
    }
  }
  return fresh;
}

std::vector<MedalState> medalStates(const std::vector<TrophyState> &states) {
  std::vector<MedalState> out;
  auto tierOf = [](int v, const int *steps) {
    int t = -1;
    for (int i = 0; i < 5; i++)
      if (v >= steps[i]) t = i;
    return t;
  };
  auto nextOf = [](int tier, const int *steps) { return tier < 4 ? steps[tier + 1] : 0; };
  for (int c = 0; c < kTCats; c++) {
    int all = 0, open = 0, got = 0, gotOpen = 0;
    for (auto &s : states) {
      if ((int)s.def->cat != c) continue;
      all++;
      if (!s.def->hidden) open++;
      if (s.ok) got++;
      if (s.ok && !s.def->hidden) gotOpen++;
    }
    // platinum needs every open one, master the secret ones too (the desktop app's ladder)
    int value = gotOpen < open ? std::min(got, open - 1) : got;
    int gold = (open * 7 + 9) / 10;
    int platinum = all > open ? open : std::min(all - 1, std::max(gold + 1, (open * 9 + 9) / 10));
    int steps[5] = {1, (open * 35 + 99) / 100, gold, platinum, all};
    MedalState m;
    m.kind = c;
    m.tier = tierOf(value, steps);
    m.value = value;
    m.total = all;
    m.next = nextOf(m.tier, steps);
    out.push_back(m);
  }
  int got = 0;
  for (auto &s : states) got += s.ok ? 1 : 0;
  int n = (int)states.size();
  int steps[5] = {(n * 2 + 9) / 10, (n * 4 + 9) / 10, (n * 6 + 9) / 10, (n * 8 + 9) / 10, n};
  MedalState m;
  m.kind = 4;
  m.tier = tierOf(got, steps);
  m.value = got;
  m.total = n;
  m.next = nextOf(m.tier, steps);
  out.push_back(m);
  return out;
}

TrophyTotals trophyTotals(const RunStore &st, const std::vector<TrophyState> &states) {
  TrophyTotals t;
  t.total = (int)states.size();
  for (auto &s : states) {
    t.maxPoints += trophyPoints(s.def->rarity);
    if (s.ok) {
      t.unlocked++;
      t.points += trophyPoints(s.def->rarity);
    }
  }
  t.species = (int)facts(st).dex.size();
  return t;
}

}  // namespace sd
