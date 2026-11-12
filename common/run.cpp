#include "run.h"

#include <cstdio>
#include <cstring>
#include "games.h"

namespace sd {

void Rules::applyPreset(const std::string &preset) {
  dupes = "line";
  blackout = "party";
  gifts = "area";
  deathless = false;
  shinyClause = true;
  nicknames = false;
  if (preset == "hardcore") {
    levelCaps = "strict";
    noBattleItems = true;
    setMode = true;
  } else {
    levelCaps = "warn";
    noBattleItems = false;
    setMode = false;
  }
}

std::string Rules::presetName() const {
  if (dupes != "line" || blackout != "party" || gifts != "area" || deathless || !shinyClause || nicknames) return "custom";
  if (levelCaps == "strict" && noBattleItems && setMode) return "hardcore";
  if (levelCaps == "warn" && !noBattleItems && !setMode) return "classic";
  return "custom";
}

const Encounter *Run::encounterFor(const std::string &area) const {
  for (auto &e : encounters)
    if (!e.starter && e.area == area && e.status != "dupe") return &e;
  return nullptr;
}
Encounter *Run::encounterFor(const std::string &area) {
  for (auto &e : encounters)
    if (!e.starter && e.area == area && e.status != "dupe") return &e;
  return nullptr;
}
RunMon *Run::mon(const std::string &key) {
  for (auto &m : mons)
    if (m.key == key) return &m;
  return nullptr;
}
const RunMon *Run::mon(const std::string &key) const {
  for (auto &m : mons)
    if (m.key == key) return &m;
  return nullptr;
}
int Run::caught() const {
  int n = 0;
  for (auto &e : encounters) n += (e.status == "caught" || e.status == "gift") ? 1 : 0;
  return n;
}
int Run::missed() const {
  int n = 0;
  for (auto &e : encounters) n += e.status == "missed" ? 1 : 0;
  return n;
}
int Run::open() const {
  int n = 0;
  for (auto &e : encounters) n += e.status == "open" ? 1 : 0;
  return n;
}
int Run::alive() const {
  int n = 0;
  for (auto &m : mons) n += m.dead ? 0 : 1;
  return n;
}
int Run::dead() const {
  int n = 0;
  for (auto &m : mons) n += m.dead ? 1 : 0;
  return n;
}
int Run::progress() const {
  const GameInfo *g = gameByKey(game.c_str());
  return (g && !g->caps.fromBadges) ? milestone : badges;
}
int Run::levelCap(std::string *boss) const {
  if (rules.levelCaps == "off") return 0;
  const GameInfo *g = gameByKey(game.c_str());
  if (!g) return 0;
  int n = progress();
  int count = g->caps.count;
  if (n < count) {
    if (boss) *boss = g->caps.bosses[n];
    if ((int)rules.caps.size() > n && rules.caps[n] > 0) return rules.caps[n];
    return g->caps.caps[n];
  }
  if (boss) *boss = g->caps.finalBoss;
  if ((int)rules.caps.size() > count && rules.caps[count] > 0) return rules.caps[count];
  return g->caps.finalCap;
}
void Run::addLog(long long ts, const char *kind, const std::string &text) {
  log.push_back({ts, kind, text});
  if (log.size() > 80) log.erase(log.begin(), log.begin() + (log.size() - 80));
}

Run *RunStore::byId(const std::string &id) {
  for (auto &r : runs)
    if (r.id == id) return &r;
  return nullptr;
}
Run *RunStore::activeRun() { return byId(active); }

Run &RunStore::create(const std::string &name, const std::string &game, const Rules &rules, long long now) {
  Run r;
  char id[32];
  std::snprintf(id, sizeof id, "r%llx", (unsigned long long)now);
  r.id = id;
  while (byId(r.id)) r.id += "x";
  r.name = name;
  r.game = game;
  r.rules = rules;
  r.created = r.updated = now;
  runs.push_back(r);
  active = runs.back().id;
  return runs.back();
}

void RunStore::remove(const std::string &id) {
  for (size_t i = 0; i < runs.size(); i++)
    if (runs[i].id == id) {
      runs.erase(runs.begin() + i);
      break;
    }
  if (active == id) active = runs.empty() ? "" : runs.back().id;
}

static Json rulesJson(const Rules &r) {
  Json j = Json::object();
  j.set("dupes", r.dupes);
  j.set("levelCaps", r.levelCaps);
  j.set("blackout", r.blackout);
  j.set("gifts", r.gifts);
  j.set("deathless", r.deathless);
  j.set("shinyClause", r.shinyClause);
  j.set("nicknames", r.nicknames);
  j.set("noBattleItems", r.noBattleItems);
  j.set("setMode", r.setMode);
  Json caps = Json::array();
  for (int c : r.caps) caps.push(c);
  j.set("caps", caps);
  return j;
}

static Rules rulesFrom(const Json &j) {
  Rules r;
  r.dupes = j["dupes"].asString(r.dupes);
  // the desktop app's rule can also be a boolean
  if (j["levelCaps"].type() == Json::Bool) r.levelCaps = j["levelCaps"].asBool() ? "warn" : "off";
  else r.levelCaps = j["levelCaps"].asString(r.levelCaps);
  r.blackout = j["blackout"].asString(r.blackout);
  r.gifts = j["gifts"].asString(r.gifts);
  r.deathless = j["deathless"].asBool(r.deathless);
  r.shinyClause = j["shinyClause"].asBool(r.shinyClause);
  r.nicknames = j["nicknames"].asBool(r.nicknames);
  r.noBattleItems = j["noBattleItems"].asBool(r.noBattleItems);
  r.setMode = j["setMode"].asBool(r.setMode);
  const Json &caps = j["caps"];
  for (size_t i = 0; i < caps.size(); i++) r.caps.push_back(caps.at(i).asInt());
  return r;
}

Json RunStore::toJson() const {
  Json root = Json::object();
  root.set("souldew3ds", version);
  root.set("active", active);
  Json s = Json::object();
  s.set("lang", settings.lang);
  s.set("overlays", settings.overlays);
  s.set("tab", settings.tab);
  root.set("settings", s);
  Json arr = Json::array();
  for (auto &r : runs) {
    Json j = Json::object();
    j.set("id", r.id);
    j.set("name", r.name);
    j.set("game", r.game);
    j.set("mode", r.mode);
    j.set("status", r.status);
    j.set("created", r.created);
    j.set("updated", r.updated);
    j.set("rules", rulesJson(r.rules));
    j.set("badges", r.badges);
    j.set("milestone", r.milestone);
    j.set("playMinutes", r.playMinutes);
    j.set("lastArea", r.lastArea);
    Json enc = Json::array();
    for (auto &e : r.encounters) {
      Json x = Json::object();
      x.set("area", e.area);
      x.set("loc", e.loc);
      x.set("species", e.species);
      x.set("level", e.level);
      x.set("status", e.status);
      if (!e.monKey.empty()) x.set("mon", e.monKey);
      if (e.starter) x.set("starter", true);
      x.set("ts", e.ts);
      enc.push(x);
    }
    j.set("encounters", enc);
    Json mons = Json::array();
    for (auto &m : r.mons) {
      Json x = Json::object();
      x.set("key", m.key);
      x.set("species", m.species);
      x.set("nickname", m.nickname);
      x.set("level", m.level);
      x.set("metLevel", m.metLevel);
      x.set("hp", m.hp);
      x.set("maxHp", m.maxHp);
      x.set("loc", m.loc);
      x.set("area", m.area);
      x.set("gender", std::string(1, m.gender));
      x.set("shiny", m.shiny);
      x.set("dead", m.dead);
      if (m.dead) {
        x.set("cause", m.cause);
        x.set("deathArea", m.deathArea);
        x.set("deathTs", m.deathTs);
      }
      x.set("inParty", m.inParty);
      x.set("partySlot", m.partySlot);
      mons.push(x);
    }
    j.set("mons", mons);
    Json log = Json::array();
    for (auto &l : r.log) {
      Json x = Json::object();
      x.set("ts", l.ts);
      x.set("kind", l.kind);
      x.set("text", l.text);
      log.push(x);
    }
    j.set("log", log);
    arr.push(j);
  }
  root.set("runs", arr);
  return root;
}

RunStore RunStore::fromJson(const Json &root) {
  RunStore st;
  st.version = root["souldew3ds"].asInt(1);
  st.active = root["active"].asString();
  const Json &s = root["settings"];
  st.settings.lang = s["lang"].asString("de");
  st.settings.overlays = s["overlays"].asBool(true);
  st.settings.tab = s["tab"].asBool(true);
  const Json &runs = root["runs"];
  for (size_t i = 0; i < runs.size(); i++) {
    const Json &j = runs.at(i);
    Run r;
    r.id = j["id"].asString();
    if (r.id.empty()) continue;
    r.name = j["name"].asString("Nuzlocke");
    r.game = j["game"].asString();
    r.mode = j["mode"].asString("nuzlocke");
    r.status = j["status"].asString("active");
    r.created = j["created"].asInt64();
    r.updated = j["updated"].asInt64();
    r.rules = rulesFrom(j["rules"]);
    r.badges = j["badges"].asInt();
    r.milestone = j["milestone"].asInt();
    r.playMinutes = j["playMinutes"].asInt();
    r.lastArea = j["lastArea"].asString();
    const Json &enc = j["encounters"];
    for (size_t k = 0; k < enc.size(); k++) {
      const Json &x = enc.at(k);
      Encounter e;
      e.area = x["area"].asString();
      e.loc = x["loc"].asInt();
      e.species = x["species"].asInt();
      e.level = x["level"].asInt();
      e.status = x["status"].asString("open");
      e.monKey = x["mon"].asString();
      e.starter = x["starter"].asBool();
      e.ts = x["ts"].asInt64();
      r.encounters.push_back(e);
    }
    const Json &mons = j["mons"];
    for (size_t k = 0; k < mons.size(); k++) {
      const Json &x = mons.at(k);
      RunMon m;
      m.key = x["key"].asString();
      m.species = x["species"].asInt();
      m.nickname = x["nickname"].asString();
      m.level = x["level"].asInt();
      m.metLevel = x["metLevel"].asInt();
      m.hp = x["hp"].asInt();
      m.maxHp = x["maxHp"].asInt();
      m.loc = x["loc"].asInt();
      m.area = x["area"].asString();
      std::string g = x["gender"].asString("-");
      m.gender = g.empty() ? '-' : g[0];
      m.shiny = x["shiny"].asBool();
      m.dead = x["dead"].asBool();
      m.cause = x["cause"].asString();
      m.deathArea = x["deathArea"].asString();
      m.deathTs = x["deathTs"].asInt64();
      m.inParty = x["inParty"].asBool();
      m.partySlot = x["partySlot"].asInt(-1);
      r.mons.push_back(m);
    }
    const Json &log = j["log"];
    for (size_t k = 0; k < log.size(); k++) {
      const Json &x = log.at(k);
      r.log.push_back({x["ts"].asInt64(), x["kind"].asString(), x["text"].asString()});
    }
    st.runs.push_back(r);
  }
  if (!st.byId(st.active)) st.active = st.runs.empty() ? "" : st.runs.back().id;
  return st;
}

bool RunStore::load(const std::string &path) {
  FILE *f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::string text;
  char buf[4096];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
  std::fclose(f);
  return parseText(text);
}

bool RunStore::parseText(const std::string &text) {
  Json j;
  if (!Json::parse(text, j) || !j.isObject()) return false;
  *this = fromJson(j);
  return true;
}

std::string RunStore::toText() const { return toJson().dump(1); }

bool RunStore::save(const std::string &path) const {
  std::string tmp = path + ".tmp";
  FILE *f = std::fopen(tmp.c_str(), "wb");
  if (!f) return false;
  std::string text = toText();
  bool ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
  ok = (std::fclose(f) == 0) && ok;
  if (!ok) return false;
  std::remove(path.c_str());
  return std::rename(tmp.c_str(), path.c_str()) == 0;
}

Lang langOf(const Settings &s) { return s.lang == "en" ? Lang::En : Lang::De; }

std::string areaKey(int gen, int loc) {
  const char *n = locationName(gen, loc, Lang::En);
  if (n) return n;
  char b[24];
  std::snprintf(b, sizeof b, "#%d", loc);
  return b;
}

std::string areaLabel(int gen, int loc, const std::string &key, Lang lang) {
  if (loc > 0) {
    const char *n = locationName(gen, loc, lang);
    if (n) return n;
  }
  return key;
}

}  // namespace sd
