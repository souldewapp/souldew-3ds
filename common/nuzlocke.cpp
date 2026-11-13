#include "nuzlocke.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include "i18n.h"

namespace sd {

Lang g_lang = Lang::De;

std::string fmt(const char *f, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, f);
  std::vsnprintf(buf, sizeof buf, f, ap);
  va_end(ap);
  return buf;
}

static std::string spName(int species) { return speciesName(species, g_lang); }

std::string Engine::monName(const Mon &m) const {
  // the nickname when the player gave one, else the species' name in the UI language
  std::string sp = speciesName(m.species, Lang::En), spDe = speciesName(m.species, Lang::De);
  if (!m.nickname.empty() && m.nickname != sp && m.nickname != spDe) return m.nickname;
  return spName(m.species);
}

bool Engine::familyCaught(int species) const {
  if (run_.rules.dupes == "off") return false;
  for (auto &m : run_.mons) {
    if (run_.rules.dupes == "species" ? m.species == species : speciesFamily(m.species) == speciesFamily(species)) return true;
  }
  return false;
}

RunMon &Engine::addMon(const Mon &m, long long now) {
  RunMon r;
  r.key = m.key;
  r.species = m.species;
  r.firstSpecies = m.species;
  r.nickname = m.nickname;
  r.level = m.level;
  r.metLevel = m.metLevel;
  r.hp = m.hp;
  r.maxHp = m.maxHp;
  r.loc = m.metLocation;
  r.area = areaKey(g_.gen, m.metLocation);
  r.gender = m.gender;
  r.shiny = m.shiny;
  (void)now;
  run_.mons.push_back(r);
  dirty_ = true;
  return run_.mons.back();
}

void Engine::adoptStarters(const LiveFrame &f, long long now, std::vector<Event> &ev) {
  // a fresh run: what the party holds when souldew first sees it is the start team (the starter), not an encounter
  for (auto &m : f.party) {
    if (m.isEgg || run_.mon(m.key)) continue;
    RunMon &r = addMon(m, now);
    Encounter e;
    e.area = "Starter";
    e.loc = m.metLocation;
    e.species = m.species;
    e.level = m.metLevel ? m.metLevel : m.level;
    e.status = "caught";
    e.monKey = m.key;
    e.starter = true;
    e.ts = now;
    run_.encounters.push_back(e);
    run_.addLog(now, "caught", fmt(tr("Start: %s", "Start: %s"), monName(m).c_str()));
    (void)r;
  }
  if (!f.party.empty()) {
    Event e{EventKind::Info, Tone::Info, tr("souldew l\xc3\xa4uft", "souldew is running"),
            fmt(tr("%s \xc2\xb7 Start-Team \xc3\xbc" "bernommen", "%s \xc2\xb7 start team adopted"), run_.name.c_str())};
    ev.push_back(e);
  }
}

void Engine::newMembers(const LiveFrame &f, long long now, std::vector<Event> &ev) {
  for (auto &m : f.party) {
    if (m.isEgg || run_.mon(m.key)) continue;
    RunMon &r = addMon(m, now);
    std::string name = monName(m);
    // the open encounter this catch closes: the last wild battle's area and species
    Encounter *open = nullptr;
    for (auto &e : run_.encounters)
      if (e.status == "open" && e.species == m.species) open = &e;
    if (open) {
      open->status = "caught";
      open->monKey = m.key;
      pendingMiss_ = false;
      r.area = open->area;
      r.loc = open->loc;
      run_.addLog(now, "caught", fmt(tr("Gefangen: %s (%s)", "Caught: %s (%s)"), name.c_str(),
                                     areaLabel(g_.gen, open->loc, open->area, g_lang).c_str()));
      ev.push_back({EventKind::Caught, Tone::Ok, fmt(tr("Gefangen: %s", "Caught: %s"), name.c_str()),
                    areaLabel(g_.gen, open->loc, open->area, g_lang), m.species});
      continue;
    }
    // no battle before it: a gift, a static, a trade or an egg – its met location is its area
    std::string area = r.area;
    bool hatched = m.eggLocation != 0;
    Encounter *used = run_.encounterFor(area);
    if (run_.rules.gifts == "area" && !hatched && !used) {
      Encounter e;
      e.area = area;
      e.loc = m.metLocation;
      e.species = m.species;
      e.level = m.metLevel ? m.metLevel : m.level;
      e.status = "gift";
      e.monKey = m.key;
      e.ts = now;
      run_.encounters.push_back(e);
      run_.addLog(now, "caught", fmt(tr("Erhalten: %s (%s)", "Received: %s (%s)"), name.c_str(),
                                     areaLabel(g_.gen, m.metLocation, area, g_lang).c_str()));
      ev.push_back({EventKind::Gift, Tone::Ok, fmt(tr("Neu im Team: %s", "New on the team: %s"), name.c_str()),
                    fmt(tr("z\xc3\xa4hlt als Begegnung: %s", "counts as the encounter of %s"),
                        areaLabel(g_.gen, m.metLocation, area, g_lang).c_str()),
                    m.species});
    } else {
      run_.addLog(now, "caught", fmt(tr("Neu: %s", "New: %s"), name.c_str()));
      std::string sub = used && run_.rules.gifts == "area" && !hatched
                            ? fmt(tr("%s war schon belegt", "%s was already used"), areaLabel(g_.gen, m.metLocation, area, g_lang).c_str())
                            : std::string(hatched ? tr("aus einem Ei", "hatched from an egg") : tr("z\xc3\xa4hlt nicht als Begegnung", "doesn't count as an encounter"));
      ev.push_back({EventKind::Gift, used ? Tone::Warn : Tone::Muted, fmt(tr("Neu im Team: %s", "New on the team: %s"), name.c_str()), sub, m.species});
    }
  }
}

void Engine::syncParty(const LiveFrame &f, long long now, std::vector<Event> &ev) {
  for (auto &rm : run_.mons) {
    rm.inParty = false;
    rm.partySlot = -1;
  }
  for (size_t i = 0; i < f.party.size(); i++) {
    const Mon &m = f.party[i];
    RunMon *r = run_.mon(m.key);
    if (!r) continue;
    if (r->level != m.level || r->hp != m.hp || r->maxHp != m.maxHp || r->nickname != m.nickname || r->species != m.species) dirty_ = true;
    r->inParty = true;
    r->partySlot = (int)i;
    r->level = m.level;
    r->hp = m.hp;
    r->maxHp = m.maxHp;
    r->nickname = m.nickname;
    if (r->species != m.species && r->species > 0 && m.species > 0 && !m.isEgg && !r->dead) {
      // an evolution (the same Pokémon, a new species)
      std::string from = speciesName(r->species, g_lang), to = speciesName(m.species, g_lang);
      run_.stats.evolutions++;
      run_.addLog(now, "evolve", fmt("%s \xe2\x86\x92 %s", from.c_str(), to.c_str()));
      ev.push_back({EventKind::Evolved, Tone::Ok, fmt(tr("%s hat sich entwickelt", "%s evolved"), monName(m).c_str()),
                    fmt("%s \xe2\x86\x92 %s", from.c_str(), to.c_str()), m.species});
    }
    r->species = m.species;
    // moments for the achievements: exactly 1 HP left
    if (m.hp == 1 && m.maxHp > 1) {
      bool before = false;
      for (auto &o : last_.party)
        if (o.key == m.key && o.hp == 1) before = true;
      if (!before && !initial_) run_.stats.oneHp++;
    }
  }
  // a full team of six sharing a type
  if (!run_.stats.monotypeAt && f.party.size() == 6) {
    for (int k = 0; k < 2; k++) {
      int t = speciesType(f.party[0].species, k);
      if (t < 0) continue;
      bool all = true;
      for (auto &m : f.party) all = all && !m.isEgg && (speciesType(m.species, 0) == t || speciesType(m.species, 1) == t);
      if (all) {
        run_.stats.monotypeAt = now;
        dirty_ = true;
        break;
      }
    }
  }
}

void Engine::setDead(Run &run, RunMon &m, bool dead, const std::string &cause, long long now) {
  if (m.dead == dead) return;
  m.dead = dead;
  if (dead) {
    m.cause = cause;
    m.deathTs = now;
    m.deathArea = run.lastArea;
    run.addLog(now, "death", fmt(tr("Tot: %s", "Dead: %s"), (m.nickname.empty() ? speciesName(m.species, g_lang) : m.nickname.c_str())));
  } else {
    m.cause.clear();
    run.addLog(now, "death", fmt(tr("Wiederbelebt (manuell): %s", "Revived (by hand): %s"), m.nickname.c_str()));
  }
}

void Engine::deaths(const LiveFrame &f, long long now, std::vector<Event> &ev) {
  std::string cause;
  if (f.battle.inBattle)
    cause = fmt("%s Lv %d", spName(f.battle.enemySpecies).c_str(), f.battle.enemyLevel) +
            (f.battle.trainer ? tr(" (Trainer)", " (trainer)") : "");
  else if (last_.battle.inBattle)
    cause = fmt("%s Lv %d", spName(last_.battle.enemySpecies).c_str(), last_.battle.enemyLevel);
  const BattleRead &foe = f.battle.inBattle ? f.battle : last_.battle;
  auto kill = [&](RunMon &r, const Mon *m) {
    if (r.dead) return;
    setDead(run_, r, true, cause, now);
    r.deathArea = area_;
    if (!cause.empty() && foe.enemySpecies > 0) {
      r.killer = foe.enemySpecies;
      r.killerLevel = foe.enemyLevel;
      r.killerTrainer = foe.trainer;
    }
    dirty_ = true;
    std::string name = m ? monName(*m) : r.nickname;
    ev.push_back({EventKind::Death, Tone::Danger, fmt(tr("%s ist gefallen", "%s has fainted"), name.c_str()),
                  cause.empty() ? std::string(tr("Nuzlocke: f\xc3\xa4llt = tot", "Nuzlocke: fainted = dead"))
                                : fmt(tr("gegen %s \xc2\xb7 in die Box", "vs %s \xc2\xb7 box it"), cause.c_str()),
                  r.species});
    if (run_.rules.deathless && run_.status == "active") {
      run_.end("lost", now);
      run_.addLog(now, "run", tr("Deathless: der Lauf ist vorbei", "Deathless: the run is over"));
      ev.push_back({EventKind::RunLost, Tone::Danger, tr("Lauf verloren", "Run lost"), tr("Deathless-Regel: der erste Tod beendet ihn", "Deathless rule: the first death ends it")});
    }
  };
  // the battle copy shows a faint before the party data does (Gen 6 writes the party back after the battle)
  if (f.battle.inBattle && f.battle.activeIndex >= 0 && f.battle.activeHp == 0 && f.battle.activeMaxHp > 0 &&
      f.battle.activeIndex < (int)f.party.size()) {
    const Mon &m = f.party[f.battle.activeIndex];
    if (RunMon *r = run_.mon(m.key)) kill(*r, &m);
  }
  int alive = 0, total = 0;
  for (auto &m : f.party) {
    if (m.isEgg) continue;
    total++;
    if (m.hp > 0) alive++;
    if (m.hp == 0 && m.maxHp > 0)
      if (RunMon *r = run_.mon(m.key)) kill(*r, &m);
  }
  // whiteout: every Pokémon in the party down (only counted once, after it stood again)
  bool allDown = total > 0 && alive == 0;
  if (allDown && !whiteout_) {
    whiteout_ = true;
    if (run_.rules.blackout == "party" && run_.status == "active") {
      run_.end("lost", now);
      run_.addLog(now, "run", tr("Blackout: der Lauf ist vorbei", "Whiteout: the run is over"));
      ev.push_back({EventKind::Whiteout, Tone::Danger, tr("Blackout \xe2\x80\x93 Lauf verloren", "Whiteout \xe2\x80\x93 run lost"),
                    tr("Das ganze Team ist gefallen", "The whole team fainted")});
    }
  } else if (!allDown) whiteout_ = false;
}

void Engine::areaChange(const LiveFrame &f, long long now, std::vector<Event> &ev) {
  if (f.place <= 0 || f.battle.inBattle) return;
  std::string key = areaKey(g_.gen, f.place);
  if (key == area_) return;
  area_ = key;
  loc_ = f.place;
  if (run_.lastArea != key) {
    run_.lastArea = key;
    dirty_ = true;
  }
  // a missed encounter is final once the player walks on
  if (pendingMiss_) {
    for (auto &e : run_.encounters)
      if (e.status == "open" && e.area == battleArea_ && f.party.size() < 6) {
        resolve(run_, e, "missed", now);
        ev.push_back({EventKind::Missed, Tone::Muted, fmt(tr("Verpasst: %s", "Missed: %s"), spName(e.species).c_str()),
                      areaLabel(g_.gen, e.loc, e.area, g_lang), e.species});
      }
    pendingMiss_ = false;
  }
  if (!run_.encounterFor(key) && run_.status == "active") {
    ev.push_back({EventKind::NewArea, Tone::Info, areaLabel(g_.gen, f.place, key, g_lang),
                  tr("Neues Gebiet \xc2\xb7 die erste Begegnung z\xc3\xa4hlt", "New area \xc2\xb7 the first encounter counts")});
    run_.addLog(now, "area", fmt(tr("Neues Gebiet: %s", "New area: %s"), areaLabel(g_.gen, f.place, key, g_lang).c_str()));
  }
}

void Engine::battle(const LiveFrame &f, long long now, std::vector<Event> &ev) {
  bool in = f.battle.inBattle;
  if (in && !wasInBattle_) {
    battleArea_ = area_;
    battleLoc_ = loc_;
    battleSpecies_ = f.battle.enemySpecies;
    if (!f.battle.trainer && !area_.empty() && run_.status == "active") {
      Encounter *used = run_.encounterFor(area_);
      std::string place = areaLabel(g_.gen, loc_, area_, g_lang);
      std::string foe = fmt("%s Lv %d", spName(f.battle.enemySpecies).c_str(), f.battle.enemyLevel);
      if (!used) {
        if (familyCaught(f.battle.enemySpecies)) {
          Encounter e{area_, loc_, f.battle.enemySpecies, f.battle.enemyLevel, "dupe", "", false, now};
          run_.encounters.push_back(e);
          run_.addLog(now, "dupe", fmt(tr("Dupe: %s (%s)", "Dupe: %s (%s)"), foe.c_str(), place.c_str()));
          ev.push_back({EventKind::DupeEncounter, Tone::Warn, fmt(tr("Dupe: %s", "Dupe: %s"), foe.c_str()),
                        tr("Schon im Lauf \xc2\xb7 z\xc3\xa4hlt nicht, n\xc3\xa4" "chste Begegnung z\xc3\xa4hlt",
                           "Already in the run \xc2\xb7 doesn't count, the next one does"),
                        f.battle.enemySpecies});
        } else {
          Encounter e{area_, loc_, f.battle.enemySpecies, f.battle.enemyLevel, "open", "", false, now};
          run_.encounters.push_back(e);
          run_.addLog(now, "encounter", fmt(tr("Begegnung: %s (%s)", "Encounter: %s (%s)"), foe.c_str(), place.c_str()));
          ev.push_back({EventKind::FirstEncounter, Tone::Ok, fmt(tr("Erste Begegnung: %s", "First encounter: %s"), foe.c_str()),
                        fmt(tr("%s \xc2\xb7 das ist dein Fang", "%s \xc2\xb7 this is your catch"), place.c_str()), f.battle.enemySpecies});
        }
        dirty_ = true;
      } else if (used->status != "open") {
        ev.push_back({EventKind::AreaUsed, Tone::Muted, fmt(tr("%s \xc2\xb7 kein Fang", "%s \xc2\xb7 no catch"), place.c_str()),
                      fmt(tr("Begegnung hier: %s", "Encounter here: %s"), spName(used->species).c_str()), used->species});
      }
    }
    if (f.battle.trainer) caps(f, now, ev, true);
  }
  if (!in && wasInBattle_) {
    battleEnd_ = now;
    // the encounter stays open until the catch shows up in the party or the player walks on (party full: the
    // catch goes to the box and is confirmed in souldew's screen)
    for (auto &e : run_.encounters)
      if (e.status == "open" && e.area == battleArea_) {
        pendingMiss_ = true;
        if (f.party.size() >= 6)
          ev.push_back({EventKind::Info, Tone::Info, tr("Team voll \xe2\x80\x93 gefangen?", "Party full \xe2\x80\x93 caught?"),
                        tr("Im souldew-Men\xc3\xbc best\xc3\xa4tigen (SELECT)", "Confirm in souldew's menu (SELECT)"), e.species});
      }
  }
  wasInBattle_ = in;
}

void Engine::caps(const LiveFrame &f, long long now, std::vector<Event> &ev, bool trainerStart) {
  std::string boss;
  int cap = run_.levelCap(&boss);
  if (!cap) return;
  bool broke = false;
  for (auto &m : f.party) {
    if (m.isEgg || m.level <= cap) continue;
    RunMon *r = run_.mon(m.key);
    if (r && r->dead) continue;
    broke = true;
    std::string tag = m.key + "@" + std::to_string(m.level) + (trainerStart ? "t" : "");
    if (std::find(capWarned_.begin(), capWarned_.end(), tag) != capWarned_.end()) continue;
    capWarned_.push_back(tag);
    bool strict = run_.rules.levelCaps == "strict";
    ev.push_back({EventKind::OverCap, strict ? Tone::Danger : Tone::Warn,
                  fmt(tr("%s Lv %d \xc3\xbc" "ber dem Cap", "%s Lv %d is over the cap"), monName(m).c_str(), m.level),
                  fmt(tr("Level-Cap %d (%s)%s", "Level cap %d (%s)%s"), cap, boss.c_str(),
                      strict ? tr(" \xc2\xb7 Hardcore: nicht einsetzen", " \xc2\xb7 hardcore: don't use it") : ""),
                  m.species});
    run_.addLog(now, "cap", fmt("%s Lv %d > %d", monName(m).c_str(), m.level, cap));
  }
  if (broke && trainerStart) run_.stats.capBreaks++;
}

void Engine::resolve(Run &run, Encounter &e, const std::string &status, long long now) {
  if (e.status == status) return;
  e.status = status;
  run.addLog(now, status == "missed" ? "missed" : "caught",
             fmt(status == "missed" ? tr("Verpasst: %s", "Missed: %s") : tr("Gefangen (Box): %s", "Caught (box): %s"),
                 speciesName(e.species, g_lang)));
  if (status == "caught" && e.monKey.empty()) {
    // a catch the party never showed (the box): a run Pokémon without memory data
    RunMon m;
    m.key = fmt("box-%lld-%d", now, e.species);
    m.species = m.firstSpecies = e.species;
    m.level = m.metLevel = e.level;
    m.loc = e.loc;
    m.area = e.area;
    run.mons.push_back(m);
    e.monKey = m.key;
  }
}

std::vector<Event> Engine::update(const LiveFrame &f, long long now) {
  std::vector<Event> ev;
  if (!f.partyOk || f.party.empty()) {
    last_.battle = f.battle;
    return ev;
  }
  initial_ = first_;
  if (first_) {
    first_ = false;
    if (run_.mons.empty() && run_.encounters.empty()) adoptStarters(f, now, ev);
    area_ = run_.lastArea;
    // don't warn about what was already over the cap when the run was opened
  }
  if (f.badges >= 0 && g_.caps.fromBadges && f.badges != run_.badges) {
    bool up = f.badges > run_.badges;
    if (up && !initial_) {
      // underdog: the strongest Pokémon still below the ace of the leader just beaten
      int capBefore = run_.levelCap(), top = 0;
      for (auto &m : f.party)
        if (!m.isEgg && m.hp > 0) top = std::max(top, m.level);
      if (capBefore && top && top < capBefore) run_.stats.underdog++;
    }
    while ((int)run_.stats.badgeAt.size() < f.badges) run_.stats.badgeAt.push_back(now);
    if ((int)run_.stats.badgeAt.size() > f.badges) run_.stats.badgeAt.resize(f.badges);
    run_.badges = f.badges;
    dirty_ = true;
    if (up) {
      std::string boss;
      int cap = run_.levelCap(&boss);
      run_.addLog(now, "badge", fmt(tr("Orden %d", "Badge %d"), f.badges));
      ev.push_back({EventKind::Badge, Tone::Ok, fmt(tr("Orden %d!", "Badge %d!"), f.badges),
                    cap ? fmt(tr("Neuer Level-Cap: %d (%s)", "New level cap: %d (%s)"), cap, boss.c_str()) : std::string()});
    }
  }
  areaChange(f, now, ev);
  battle(f, now, ev);
  newMembers(f, now, ev);
  syncParty(f, now, ev);
  deaths(f, now, ev);
  if (!f.battle.inBattle) caps(f, now, ev, false);
  if (pendingMiss_ && battleEnd_ && now - battleEnd_ > 90 && f.party.size() < 6) {
    // long after the battle and nothing joined the party: missed
    for (auto &e : run_.encounters)
      if (e.status == "open" && e.area == battleArea_) {
        resolve(run_, e, "missed", now);
        ev.push_back({EventKind::Missed, Tone::Muted, fmt(tr("Verpasst: %s", "Missed: %s"), spName(e.species).c_str()),
                      areaLabel(g_.gen, e.loc, e.area, g_lang), e.species});
      }
    pendingMiss_ = false;
  }
  if (!ev.empty()) dirty_ = true;
  if (dirty_) run_.updated = now;
  last_ = f;
  return ev;
}

}  // namespace sd
