// Unit tests for the platform-free part (common/) – run on the computer with the same sources the console builds use.
//   tests [path to test/fixtures]
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../common/ctrzones.h"
#include "../common/fbblit.h"
#include "../common/i18n.h"
#include "../common/json.h"
#include "../common/nuzlocke.h"
#include "../common/pk6.h"
#include "../common/run.h"
#include "../common/screens.h"
#include "../common/trophies.h"

using namespace sd;

static int g_fail = 0, g_pass = 0;
#define CHECK(c)                                                         \
  do {                                                                   \
    if (c) g_pass++;                                                     \
    else {                                                               \
      g_fail++;                                                          \
      std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);         \
    }                                                                    \
  } while (0)
#define SECTION(n) std::printf("%s\n", n)

static void put16(uint8_t *b, int o, int v) { b[o] = v & 0xff; b[o + 1] = (v >> 8) & 0xff; }
static void put32(uint8_t *b, int o, uint32_t v) { for (int i = 0; i < 4; i++) b[o + i] = (v >> (8 * i)) & 0xff; }

// ---- PK6 against the desktop app's JavaScript -----------------------------------------------------------------------
static void testPk6Fixtures(const std::string &dir) {
  SECTION("pk6: decrypt/parse = desktop app's gen67.mjs (fixtures)");
  std::ifstream f(dir + "/pk6.json");
  std::stringstream ss;
  ss << f.rdbuf();
  Json j;
  CHECK(Json::parse(ss.str(), j) && j.isArray() && j.size() == 5);
  for (size_t i = 0; i < j.size(); i++) {
    const Json &c = j.at(i);
    std::string hex = c["hex"].asString();
    std::vector<uint8_t> raw(hex.size() / 2);
    for (size_t k = 0; k < raw.size(); k++) raw[k] = (uint8_t)std::stoi(hex.substr(k * 2, 2), nullptr, 16);
    Mon m;
    CHECK(raw.size() == 260 && parsePK6(raw.data(), c["gen"].asInt(), m));
    const Json &e = c["expect"];
    CHECK(m.key == e["key"].asString());
    CHECK(m.species == e["species"].asInt());
    CHECK(m.level == e["level"].asInt());
    CHECK(m.hp == e["hp"].asInt() && m.maxHp == e["maxHp"].asInt());
    CHECK(m.status == e["status"].asInt());
    CHECK(m.metLocation == e["metLocation"].asInt());
    CHECK(m.nickname == e["nickname"].asString());
    CHECK(std::string(1, m.gender) == e["gender"].asString());
    CHECK(m.shiny == e["shiny"].asBool());
    CHECK(m.isEgg == e["isEgg"].asBool());
    // and back: re-encrypting the decrypted bytes gives the input again
    uint8_t plain[PK_SIZE], again[PK_SIZE];
    CHECK(decryptPK6(raw.data(), PK_SIZE, plain));
    encryptPK6(plain, PK_SIZE, again);
    CHECK(!std::memcmp(again, raw.data(), PK_SIZE));
  }
}

// a party slot in RAM layout
static void writeMon(uint8_t *region, int slot, int species, int level, int hp, int maxHp, uint32_t pid, int met) {
  uint8_t p[PK_SIZE] = {0}, e[PK_SIZE];
  put32(p, 0, 0x1000 + pid * 7);
  put16(p, 0x08, species);
  put16(p, 0x0C, 111);
  put16(p, 0x0E, 222);
  put32(p, 0x18, pid);
  put16(p, 0xDA, met);
  p[0xDD] = level;
  p[0xEC] = level;
  put16(p, 0xF0, hp);
  put16(p, 0xF2, maxHp);
  for (int o = 0xF4; o < 0xFE; o += 2) put16(p, o, 40);
  encryptPK6(p, PK_SIZE, e);
  std::memcpy(region + slot * SLOT_STRIDE, e, BOX_SIZE);
  std::memcpy(region + slot * SLOT_STRIDE + STATS_OFFSET, e + BOX_SIZE, STATS_SIZE);
}

struct FakeMem {
  std::vector<uint8_t> party = std::vector<uint8_t>(PARTY_BYTES, 0);
  uint8_t badges = 0;
  std::vector<uint8_t> wild = std::vector<uint8_t>(580 * 12, 0), trainer = std::vector<uint8_t>(580 * 12, 0);
  uint16_t zone = 0;
};
static FakeMem g_mem;
static bool fakeRead(uint32_t addr, void *out, uint32_t len, void *) {
  const GameInfo *g = gameByKey("xy");
  auto copy = [&](const uint8_t *src, uint32_t base, uint32_t size) {
    if (addr < base || addr + len > base + size) return false;
    std::memcpy(out, src + (addr - base), len);
    return true;
  };
  if (copy(g_mem.party.data(), g->partyAddrs[1].party, PARTY_BYTES)) return true;
  if (copy(&g_mem.badges, g->partyAddrs[1].badges, 1)) return true;
  if (copy(g_mem.wild.data(), g->battle.wild, (uint32_t)g_mem.wild.size())) return true;
  if (copy(g_mem.trainer.data(), g->battle.trainer, (uint32_t)g_mem.trainer.size())) return true;
  if (copy(reinterpret_cast<const uint8_t *>(&g_mem.zone), g->zoneAddr, 2)) return true;
  return false;  // unmapped (the 1.5 address)
}
static void battler(std::vector<uint8_t> &base, int block, int species, int level, int hp, int maxHp, int slot) {
  uint8_t *o = base.data() + block * 580;
  put16(o, 4, species);
  put16(o, 6, maxHp);
  put16(o, 8, hp);
  o[0x10] = level;
  o[0x11] = slot;
}

static void testPartyReader() {
  SECTION("pk6: party reader (address fallback, shrink rule, Bad Egg)");
  const GameInfo &g = *gameByKey("xy");
  g_mem = FakeMem();
  writeMon(g_mem.party.data(), 0, 656, 5, 20, 20, 1, 10);
  writeMon(g_mem.party.data(), 1, 661, 3, 12, 12, 2, 12);
  g_mem.badges = 0x07;
  PartyReaderState st;
  PartyRead r = readParty(g, fakeRead, nullptr, st);
  CHECK(r.ok && r.party.size() == 2 && r.badges == 3);
  CHECK(st.addrIndex == 1 && r.version && std::string(r.version) == "1.0");
  // a slot disappears: only accepted on the second read
  std::memset(g_mem.party.data() + SLOT_STRIDE, 0, SLOT_STRIDE);
  r = readParty(g, fakeRead, nullptr, st);
  CHECK(!r.ok);
  r = readParty(g, fakeRead, nullptr, st);
  CHECK(r.ok && r.party.size() == 1);
  // slot 0 damaged → no party
  g_mem.party[3 * 16] ^= 0xff;
  g_mem.party[20] ^= 0x55;
  r = readParty(g, fakeRead, nullptr, st);
  CHECK(!r.ok);
}

static void testBattle() {
  SECTION("pk6: battle reader (Gen 6 wild / trainer, Gen 7 flags)");
  const GameInfo &g = *gameByKey("xy");
  g_mem = FakeMem();
  writeMon(g_mem.party.data(), 0, 656, 5, 20, 20, 1, 10);
  PartyReaderState st;
  PartyRead pr = readParty(g, fakeRead, nullptr, st);
  BattleRead b = readBattle(g, fakeRead, nullptr, &pr.party);
  CHECK(!b.inBattle);
  battler(g_mem.wild, 0, 656, 5, 17, 20, 0);
  battler(g_mem.wild, 1, 263, 3, 11, 15, 12);
  b = readBattle(g, fakeRead, nullptr, &pr.party);
  CHECK(b.inBattle && !b.trainer && b.enemySpecies == 263 && b.enemyLevel == 3 && b.activeIndex == 0 && b.activeHp == 17);
  std::fill(g_mem.wild.begin(), g_mem.wild.end(), 0);
  battler(g_mem.trainer, 0, 656, 5, 0, 20, 0);
  battler(g_mem.trainer, 1, 664, 6, 18, 18, 12);
  b = readBattle(g, fakeRead, nullptr, &pr.party);
  CHECK(b.inBattle && b.trainer && b.activeHp == 0);
}

// ---- RomFS / GARC / LZ11 ---------------------------------------------------------------------------------------------
struct Img {
  std::vector<uint8_t> b;
  void at(size_t o, const void *d, size_t n) {
    if (b.size() < o + n) b.resize(o + n);
    std::memcpy(b.data() + o, d, n);
  }
};
static bool imgRead(uint64_t off, void *out, uint32_t len, void *ctx) {
  Img *im = static_cast<Img *>(ctx);
  if (off + len > im->b.size()) return false;
  std::memcpy(out, im->b.data() + off, len);
  return true;
}
static std::vector<uint8_t> u16name(const std::string &s) {
  std::vector<uint8_t> v;
  for (char c : s) { v.push_back((uint8_t)c); v.push_back(0); }
  while (v.size() % 4) v.push_back(0);
  return v;
}
// a RomFS level 3 with the directory chain a/0/1 and one file "2" → returns its bytes
static std::vector<uint8_t> level3(const std::string &dirs3, const std::vector<uint8_t> &file) {
  // directories: root, a, 0, 1 (each the only child of the one before); one file in the last
  std::vector<uint8_t> dm, fm;
  const char *names[] = {"", "a", "0", "1"};
  std::vector<uint32_t> offs;
  uint32_t o = 0;
  for (int i = 0; i < 4; i++) {
    offs.push_back(o);
    o += 0x18 + (uint32_t)u16name(names[i]).size();
  }
  for (int i = 0; i < 4; i++) {
    uint8_t e[0x18] = {0};
    put32(e, 0x00, i ? offs[i - 1] : 0);
    put32(e, 0x04, 0xffffffff);
    put32(e, 0x08, i < 3 ? offs[i + 1] : 0xffffffff);
    put32(e, 0x0C, i == 3 ? 0 : 0xffffffff);
    put32(e, 0x10, 0xffffffff);
    std::vector<uint8_t> n = u16name(names[i]);
    put32(e, 0x14, (uint32_t)std::strlen(names[i]) * 2);
    dm.insert(dm.end(), e, e + 0x18);
    dm.insert(dm.end(), n.begin(), n.end());
  }
  uint8_t fe[0x20] = {0};
  put32(fe, 0x00, offs[3]);
  put32(fe, 0x04, 0xffffffff);
  put32(fe, 0x08, 0);
  put32(fe, 0x10, (uint32_t)file.size());
  put32(fe, 0x18, 0xffffffff);
  put32(fe, 0x1C, 2);
  fm.insert(fm.end(), fe, fe + 0x20);
  std::vector<uint8_t> fn = u16name(dirs3);
  fm.insert(fm.end(), fn.begin(), fn.end());
  std::vector<uint8_t> out(0x28, 0);
  put32(out.data(), 0, 0x28);
  put32(out.data(), 0x0C, 0x28);
  put32(out.data(), 0x10, (uint32_t)dm.size());
  put32(out.data(), 0x1C, 0x28 + (uint32_t)dm.size());
  put32(out.data(), 0x20, (uint32_t)fm.size());
  uint32_t data = 0x28 + (uint32_t)dm.size() + (uint32_t)fm.size();
  data = (data + 15) & ~15u;
  put32(out.data(), 0x24, data);
  out.insert(out.end(), dm.begin(), dm.end());
  out.insert(out.end(), fm.begin(), fm.end());
  out.resize(data, 0);
  out.insert(out.end(), file.begin(), file.end());
  return out;
}
static std::vector<uint8_t> garc(const std::vector<std::vector<uint8_t>> &files) {
  std::vector<uint8_t> fato, fatb, data;
  uint32_t n = (uint32_t)files.size();
  fato.resize(0x0C + 4 * n);
  std::memcpy(fato.data(), "OTAF", 4);
  put32(fato.data(), 4, (uint32_t)fato.size());
  put16(fato.data(), 8, n);
  put16(fato.data(), 10, 0xffff);
  fatb.resize(0x0C + 16 * n);
  std::memcpy(fatb.data(), "BTAF", 4);
  put32(fatb.data(), 4, (uint32_t)fatb.size());
  put32(fatb.data(), 8, n);
  for (uint32_t i = 0; i < n; i++) {
    put32(fato.data(), 0x0C + 4 * i, i * 16);
    put32(fatb.data(), 0x0C + 16 * i, 1);
    put32(fatb.data(), 0x10 + 16 * i, (uint32_t)data.size());
    put32(fatb.data(), 0x14 + 16 * i, (uint32_t)(data.size() + files[i].size()));
    put32(fatb.data(), 0x18 + 16 * i, (uint32_t)files[i].size());
    data.insert(data.end(), files[i].begin(), files[i].end());
    while (data.size() % 4) data.push_back(0);
  }
  std::vector<uint8_t> h(0x1C, 0);
  std::memcpy(h.data(), "CRAG", 4);
  put32(h.data(), 4, 0x1C);
  put32(h.data(), 0x10, (uint32_t)(0x1C + fato.size() + fatb.size() + 0x0C));
  std::vector<uint8_t> fimb(0x0C, 0);
  std::memcpy(fimb.data(), "BMIF", 4);
  put32(fimb.data(), 4, 0x0C);
  put32(fimb.data(), 8, (uint32_t)data.size());
  h.insert(h.end(), fato.begin(), fato.end());
  h.insert(h.end(), fatb.begin(), fatb.end());
  h.insert(h.end(), fimb.begin(), fimb.end());
  h.insert(h.end(), data.begin(), data.end());
  return h;
}

static void testZones(const std::string &dumpPath) {
  SECTION("ctrzones: NCCH → IVFC → RomFS → GARC → zone table (X/Y layout)");
  const uint16_t places[] = {6, 10, 12, 14, 16, 300};
  std::vector<uint8_t> zt;
  for (uint16_t p : places) {
    uint8_t z[56] = {0};
    z[0x1C] = p & 0xff;
    z[0x1D] = (p >> 8) & 1;
    zt.insert(zt.end(), z, z + 56);
  }
  std::vector<uint8_t> enc(48, 0);  // an "encounter file" before it
  std::vector<uint8_t> g = garc({enc, zt});
  std::vector<uint8_t> l3 = level3("2", g);
  // IVFC: master hash 0x20, level 3 block 0x1000 → level 3 at 0x1000
  std::vector<uint8_t> ivfc(0x1000, 0);
  std::memcpy(ivfc.data(), "IVFC", 4);
  put32(ivfc.data(), 0x08, 0x20);
  put32(ivfc.data(), 0x4C, 12);
  ivfc.insert(ivfc.end(), l3.begin(), l3.end());
  Img nc;
  std::vector<uint8_t> h(0x200, 0);
  std::memcpy(h.data() + 0x100, "NCCH", 4);
  h[0x18F] = 0x04;  // NoCrypto
  put32(h.data(), 0x1B0, 2);  // RomFS at unit 2 (0x400)
  put32(h.data(), 0x1B4, (uint32_t)((ivfc.size() + 0x1ff) / 0x200));
  nc.at(0, h.data(), h.size());
  nc.at(0x400, ivfc.data(), ivfc.size());
  const GameInfo &xy = *gameByKey("xy");
  std::vector<uint16_t> got = zonePlaces(xy, imgRead, &nc);
  CHECK(got.size() == 6);
  for (size_t i = 0; i < got.size() && i < 6; i++) CHECK(got[i] == places[i]);
  // the same RomFS as FS's RomFS archive hands it out (IVFC at 0) and as a bare level 3
  Img iv;
  iv.at(0, ivfc.data(), ivfc.size());
  CHECK(zonePlaces(xy, imgRead, &iv, ImageKind::Auto).size() == 6);
  Img bare;
  bare.at(0, l3.data(), l3.size());
  CHECK(zonePlaces(xy, imgRead, &bare, ImageKind::Auto).size() == 6);
  // ORAS keeps it second to last: with the table last it still finds it (the other place as fallback)
  CHECK(zonePlaces(*gameByKey("oras"), imgRead, &nc).size() == 0);  // ORAS wants a/0/1/3 – not in this image
  // an encrypted NCCH is refused
  nc.b[0x18F] = 0;
  CHECK(zonePlaces(xy, imgRead, &nc).empty());
  nc.b[0x18F] = 0x04;
  if (!dumpPath.empty()) {
    std::ofstream o(dumpPath, std::ios::binary);
    o.write(reinterpret_cast<const char *>(nc.b.data()), (std::streamsize)nc.b.size());
  }

  SECTION("ctrzones: LZ11");
  // "ABCABCABCABC" as LZ11: literals A B C, then a reference (len 9, disp 3)
  std::vector<uint8_t> lz = {0x11, 12, 0, 0, 0x10, 'A', 'B', 'C', 0x80, 0x02};
  // flags 0x10: bits 7..5 literal (A B C), bit 4 reference; reference b0 = (len-1)<<4 | (disp-1)>>8 with len 9 → 0x8?
  std::vector<uint8_t> out = lz11(lz);
  CHECK(out.size() == 12 && std::string(out.begin(), out.end()) == "ABCABCABCABC");
  std::vector<uint8_t> bad = {0x11, 12, 0, 0, 0x10, 'A', 'B', 'C', 0x80, 0x09};  // reference before the start
  CHECK(lz11(bad).empty());
  std::vector<uint8_t> plain = {1, 2, 3};
  CHECK(lz11(plain) == plain);
}

// ---- JSON + run file ---------------------------------------------------------------------------------------------
static void testJsonRun() {
  SECTION("json + run file: round trip, desktop-style rules, unicode");
  Json j;
  CHECK(Json::parse("{\"a\":[1,2.5,true,null,\"\\u00e4\\ud83d\\ude00\"],\"b\":{}}", j));
  CHECK(j["a"].size() == 5 && j["a"].at(1).asNumber() == 2.5 && j["a"].at(4).asString() == "\xc3\xa4\xf0\x9f\x98\x80");
  CHECK(!Json::parse("{\"a\":}", j));
  CHECK(!Json::parse("[1,2", j));
  RunStore st;
  Rules r;
  r.applyPreset("hardcore");
  CHECK(r.presetName() == "hardcore");
  Run &run = st.create("Kims \"Nuzlocke\" \xc3\xa4", "oras", r, 1000);
  run.encounters.push_back({"Route 101", 6, 263, 2, "caught", "k1", false, 5});
  RunMon m;
  m.key = "k1";
  m.species = 263;
  m.dead = true;
  m.cause = "Wurmple Lv 3";
  run.mons.push_back(m);
  std::string text = st.toText();
  RunStore back;
  CHECK(back.parseText(text));
  CHECK(back.runs.size() == 1 && back.runs[0].name == "Kims \"Nuzlocke\" \xc3\xa4");
  CHECK(back.runs[0].rules.levelCaps == "strict" && back.runs[0].rules.presetName() == "hardcore");
  CHECK(back.runs[0].mons[0].dead && back.runs[0].mons[0].cause == "Wurmple Lv 3");
  CHECK(back.active == back.runs[0].id);
  // the desktop app writes levelCaps as a boolean
  RunStore d;
  CHECK(d.parseText("{\"runs\":[{\"id\":\"x\",\"game\":\"xy\",\"rules\":{\"levelCaps\":true}}]}"));
  CHECK(d.runs[0].rules.levelCaps == "warn");
  CHECK(!d.parseText("not json"));
}

// ---- the rules engine: the mock game's scenario ------------------------------------------------------------------
static int count(const std::vector<Event> &ev, EventKind k) {
  int n = 0;
  for (auto &e : ev) n += e.kind == k;
  return n;
}

static void testEngine() {
  SECTION("nuzlocke engine: starter, new area, first encounter, catch, dupe, cap, death, badge, whiteout");
  g_lang = Lang::De;
  RunStore st;
  Rules rules;
  rules.applyPreset("classic");
  Run &run = st.create("Test", "xy", rules, 0);
  Engine eng(run, *gameByKey("xy"));
  auto mon = [](const char *key, int sp, int lv, int hp, int maxHp, int met) {
    Mon m;
    m.key = key;
    m.species = sp;
    m.level = lv;
    m.hp = hp;
    m.maxHp = maxHp;
    m.metLocation = met;
    return m;
  };
  LiveFrame f;
  f.partyOk = true;
  f.badges = 0;
  f.party = {mon("p1", 656, 5, 20, 20, 10)};
  f.place = 10;  // Aquacorde Town
  long long t = 100;
  std::vector<Event> ev = eng.update(f, t);
  CHECK(run.mons.size() == 1 && run.encounters.size() == 1 && run.encounters[0].starter);
  CHECK(count(ev, EventKind::NewArea) == 1);
  f.place = 12;  // Route 2
  ev = eng.update(f, t += 5);
  CHECK(count(ev, EventKind::NewArea) == 1 && ev[0].title == "Route 2");
  // the same area again: quiet
  ev = eng.update(f, t += 1);
  CHECK(ev.empty());
  // a wild Fletchling: the area's encounter
  f.battle.inBattle = true;
  f.battle.enemySpecies = 661;
  f.battle.enemyLevel = 3;
  f.battle.enemyHp = f.battle.enemyMaxHp = 12;
  f.battle.activeIndex = 0;
  f.battle.activeHp = 20;
  f.battle.activeMaxHp = 20;
  ev = eng.update(f, t += 5);
  CHECK(count(ev, EventKind::FirstEncounter) == 1);
  CHECK(run.encounterFor("Route 2") && run.encounterFor("Route 2")->status == "open");
  f.battle = BattleRead();
  eng.update(f, t += 3);
  f.party.push_back(mon("p2", 661, 3, 12, 12, 12));
  ev = eng.update(f, t += 1);
  CHECK(count(ev, EventKind::Caught) == 1 && run.encounterFor("Route 2")->status == "caught");
  // another area, a Fletchling again: a dupe (doesn't use up the area)
  f.place = 14;
  eng.update(f, t += 5);
  f.battle.inBattle = true;
  f.battle.enemySpecies = 662;  // Fletchinder: same evolution line
  f.battle.enemyLevel = 4;
  f.battle.enemyMaxHp = f.battle.enemyHp = 13;
  ev = eng.update(f, t += 5);
  CHECK(count(ev, EventKind::DupeEncounter) == 1 && !run.encounterFor("Santalune Forest"));
  f.battle = BattleRead();
  eng.update(f, t += 3);
  // over the cap (12 before the first badge)
  f.party[0].level = 13;
  ev = eng.update(f, t += 1);
  CHECK(count(ev, EventKind::OverCap) == 1);
  ev = eng.update(f, t += 1);
  CHECK(count(ev, EventKind::OverCap) == 0);  // once per level
  // trainer battle: warned again, then Fletchling faints in it
  f.battle.inBattle = true;
  f.battle.trainer = true;
  f.battle.enemySpecies = 664;
  f.battle.enemyLevel = 6;
  f.battle.activeIndex = 1;
  f.battle.activeHp = 12;
  f.battle.activeMaxHp = 12;
  ev = eng.update(f, t += 5);
  CHECK(count(ev, EventKind::OverCap) == 1);
  f.battle.activeHp = 0;
  ev = eng.update(f, t += 2);
  CHECK(count(ev, EventKind::Death) == 1 && run.mon("p2")->dead);
  CHECK(run.mon("p2")->cause.find("Purmel") != std::string::npos);  // Scatterbug in German
  CHECK(run.mon("p2")->killer == 664 && run.mon("p2")->killerLevel == 6 && run.mon("p2")->killerTrainer);
  CHECK(run.stats.capBreaks == 1);  // the trainer battle started over the cap
  // the party data catches up: no second death
  f.party[1].hp = 0;
  ev = eng.update(f, t += 1);
  CHECK(count(ev, EventKind::Death) == 0);
  // battle over, a badge
  f.battle = BattleRead();
  f.badges = 1;
  ev = eng.update(f, t += 5);
  CHECK(count(ev, EventKind::Badge) == 1 && run.badges == 1 && run.levelCap() == 25);
  CHECK(run.stats.badgeAt.size() == 1 && run.stats.badgeAt[0] == t);
  CHECK(run.caught() == 2 && run.dead() == 1 && run.alive() == 1);
  // whiteout: the run is lost (blackout rule "party")
  f.party[0].hp = 0;
  ev = eng.update(f, t += 5);
  CHECK(count(ev, EventKind::Whiteout) == 1 && run.status == "lost" && run.stats.endedAt == t);
  // a missed encounter: battle in a new area, nothing joins, the player walks on
  RunStore st2;
  Run &r2 = st2.create("Miss", "xy", rules, 0);
  Engine e2(r2, *gameByKey("xy"));
  LiveFrame g;
  g.partyOk = true;
  g.party = {mon("q1", 650, 5, 20, 20, 10)};
  g.place = 12;
  e2.update(g, 1);
  g.battle.inBattle = true;
  g.battle.enemySpecies = 16;
  g.battle.enemyLevel = 2;
  g.battle.enemyMaxHp = g.battle.enemyHp = 10;
  e2.update(g, 5);
  g.battle = BattleRead();
  e2.update(g, 9);
  g.place = 16;
  ev = e2.update(g, 15);
  CHECK(count(ev, EventKind::Missed) == 1 && r2.encounterFor("Route 2")->status == "missed");
  CHECK(count(ev, EventKind::NewArea) == 1);
  // gift rule: a Pokémon without a battle uses up its met location
  g.party.push_back(mon("q2", 1, 10, 30, 30, 16));
  ev = e2.update(g, 20);
  CHECK(count(ev, EventKind::Gift) == 1 && r2.encounterFor("Route 3") && r2.encounterFor("Route 3")->status == "gift");
}

static void testEngineMoments() {
  SECTION("engine moments: evolution, exactly 1 HP, a monotype team, underdog badge");
  RunStore st;
  Rules rules;
  rules.applyPreset("classic");
  Run &run = st.create("M", "xy", rules, 0);
  Engine eng(run, *gameByKey("xy"));
  auto mon = [](const char *key, int sp, int lv, int hp) {
    Mon m;
    m.key = key;
    m.species = sp;
    m.level = lv;
    m.hp = hp;
    m.maxHp = 40;
    m.metLocation = 10;
    return m;
  };
  LiveFrame f;
  f.partyOk = true;
  f.badges = 0;
  f.place = 10;
  f.party = {mon("w1", 7, 10, 30)};  // Squirtle
  eng.update(f, 1);
  CHECK(run.mons[0].firstSpecies == 7);
  f.party[0].species = 8;  // Wartortle
  std::vector<Event> ev = eng.update(f, 2);
  CHECK(count(ev, EventKind::Evolved) == 1 && run.stats.evolutions == 1 && run.mons[0].species == 8 && run.mons[0].firstSpecies == 7);
  f.party[0].hp = 1;
  eng.update(f, 3);
  eng.update(f, 4);  // still 1 HP: counted once
  CHECK(run.stats.oneHp == 1);
  // six Water-types
  const int water[6] = {8, 54, 60, 72, 90, 116};
  f.party.clear();
  for (int i = 0; i < 6; i++) f.party.push_back(mon(fmt("w%d", i + 1).c_str(), water[i], 10, 30));
  f.party[0].species = 8;
  eng.update(f, 5);
  CHECK(run.stats.monotypeAt == 5);
  // a badge with everyone below the cap (12): underdog
  f.badges = 1;
  eng.update(f, 6);
  CHECK(run.stats.underdog == 1 && run.stats.badgeAt.size() == 1);
}

static void testTrophies() {
  SECTION("achievements: unlocking from the runs, kept in the profile, medals, round trip");
  RunStore st;
  CHECK(evaluateTrophies(st).size() == (size_t)trophyCount() && trophyCount() > 50);
  for (int i = 0; i < trophyCount(); i++) CHECK(trophyById(trophyAt(i).id) == &trophyAt(i));
  CHECK(unlockTrophies(st, 1).empty());  // nothing yet
  Rules r;
  r.applyPreset("hardcore");
  Run &run = st.create("T", "xy", r, 1000);
  auto names = [](const std::vector<const TrophyDef *> &v) {
    std::string s;
    for (auto *d : v) s += std::string(d->id) + ",";
    return s;
  };
  std::string got = names(unlockTrophies(st, 2000));
  CHECK(got == "firstRun,");
  CHECK(unlockTrophies(st, 2001).empty());  // once
  // a catch, a badge with no death since the start, a Pokémon killed by a wild Magikarp
  run.encounters.push_back({"Route 2", 12, 129, 3, "caught", "a", false, 1100});
  RunMon a;
  a.key = "a";
  a.species = a.firstSpecies = 129;
  a.level = 10;
  run.mons.push_back(a);
  run.stats.badgeAt = {1500};
  run.badges = 1;
  got = names(unlockTrophies(st, 3000));
  CHECK(got.find("firstCatch,") != std::string::npos && got.find("firstBadge,") != std::string::npos &&
        got.find("cleanBadge,") != std::string::npos);
  run.mons[0].species = 130;  // evolved into Gyarados …
  run.mons[0].dead = true;    // … and lost to a Magikarp
  run.mons[0].killer = 129;
  run.mons[0].deathTs = 1600;
  got = names(unlockTrophies(st, 4000));
  CHECK(got.find("gyarados,") != std::string::npos && got.find("magikarp,") != std::string::npos);
  // the run is won with the hardcore rules … but not deathless
  run.end("won", 5000);
  got = names(unlockTrophies(st, 5000));
  CHECK(got.find("firstWin,") != std::string::npos && got.find("hardcoreWin,") != std::string::npos &&
        got.find("lowDeathWin,") != std::string::npos && got.find("deathlessWin") == std::string::npos);
  // progress of a counted one
  std::vector<TrophyState> states = evaluateTrophies(st);
  for (auto &s : states)
    if (std::string(s.def->id) == "catches50") CHECK(s.n == 1 && !s.ok);
  // the profile keeps them when the run goes; the owned species too
  st.remove(run.id);
  states = evaluateTrophies(st);
  int ok = 0;
  for (auto &s : states) ok += s.ok;
  CHECK(ok == (int)st.profile.trophies.size() && ok >= 8);
  CHECK(st.profile.has("magikarp") && st.profile.when("firstRun") == 2000);
  CHECK(std::binary_search(st.profile.dex.begin(), st.profile.dex.end(), 130));
  std::vector<MedalState> medals = medalStates(states);
  CHECK(medals.size() == 5 && medals[4].kind == 4 && medals[4].value == ok);
  CHECK(medals[0].tier >= 0);  // journey: bronze at least
  RunStore back;
  CHECK(back.parseText(st.toText()));
  CHECK(back.profile.trophies.size() == st.profile.trophies.size() && back.profile.dex == st.profile.dex &&
        back.profile.runsCreated == 1 && back.settings.onboarded == st.settings.onboarded);
  // run stats + the killer survive the run file
  RunStore s2;
  Run &r2 = s2.create("S", "sm", r, 0);
  r2.stats.badgeAt = {10, 20};
  r2.stats.oneHp = 2;
  r2.stats.monotypeAt = 30;
  RunMon k;
  k.key = "k";
  k.species = 25;
  k.firstSpecies = 172;
  k.dead = true;
  k.killer = 19;
  k.killerLevel = 4;
  r2.mons.push_back(k);
  RunStore b2;
  CHECK(b2.parseText(s2.toText()));
  const Run &br = b2.runs[0];
  CHECK(br.stats.badgeAt.size() == 2 && br.stats.oneHp == 2 && br.stats.monotypeAt == 30);
  CHECK(br.mons[0].firstSpecies == 172 && br.mons[0].killer == 19 && br.mons[0].killerLevel == 4 && !br.mons[0].killerTrainer);
  // an old file (no profile, no "onboarded"): runs mean the introduction was seen
  RunStore old;
  CHECK(old.parseText("{\"runs\":[{\"id\":\"x\",\"game\":\"xy\"}]}") && old.settings.onboarded && old.profile.runsCreated == 1);
  RunStore none;
  CHECK(none.parseText("{\"runs\":[]}") && !none.settings.onboarded);
}

static void testScreens() {
  SECTION("launcher screens: menu focus + actions, introduction language + pages, settings, achievements filter");
  g_lang = Lang::De;
  RunStore st;
  std::vector<InstalledGame> inst;
  Canvas top(400, 240), bottom(320, 240);
  MainMenu menu;
  menu.draw(top, bottom, st, inst, "");
  Input in;
  in.pressed = K_A;
  CHECK(menu.handle(in, st) == MenuAction::NewRun);  // no run yet: "continue" creates one
  in.pressed = K_DOWN;
  menu.handle(in, st);
  in.pressed = K_RIGHT;
  menu.handle(in, st);
  in.pressed = K_DOWN;
  menu.handle(in, st);
  CHECK(menu.focus == 4);
  in.pressed = K_A;
  CHECK(menu.handle(in, st) == MenuAction::Settings);
  // a tap on the achievements tile (left column, second row)
  menu.draw(top, bottom, st, inst, "");
  in = Input();
  in.touchDown = true;
  in.tx = 60;
  in.ty = 140;
  menu.handle(in, st);
  in = Input();
  in.touchUp = true;
  in.tx = 60;
  in.ty = 140;
  CHECK(menu.handle(in, st) == MenuAction::Trophies);
  // the introduction: English, through the pages, done
  Onboarding ob;
  ob.start(false);
  ob.draw(top, bottom, st);
  in = Input();
  in.pressed = K_RIGHT;
  CHECK(ob.handle(in, st) == IntroAction::Language && st.settings.lang == "en" && g_lang == Lang::En);
  in.pressed = K_A;
  for (int i = 0; i < Onboarding::kPages - 1; i++) CHECK(ob.handle(in, st) == IntroAction::Changed);
  CHECK(ob.page == Onboarding::kPages - 1 && !st.settings.onboarded);
  CHECK(ob.handle(in, st) == IntroAction::NewRun && st.settings.onboarded);
  // settings: language back to German, overlays off
  SettingsScreen se;
  se.draw(top, bottom, st);
  in.pressed = K_LEFT;
  CHECK(se.handle(in, st) == SettingsAction::Language && st.settings.lang == "de" && g_lang == Lang::De);
  in.pressed = K_DOWN;
  se.handle(in, st);
  in.pressed = K_A;
  se.handle(in, st);
  CHECK(!st.settings.overlays);
  // achievements: L / R switch the category
  TrophyScreen ts;
  ts.draw(top, bottom, st);
  in.pressed = K_R;
  CHECK(ts.handle(in, st) == TrophyAction::Changed && ts.filter == 1);
  in.pressed = K_B;
  CHECK(ts.handle(in, st) == TrophyAction::Back);
  // a screen change blends the two pictures
  Canvas a(8, 2), b(8, 2), o(8, 2);
  a.clear(Color(0, 0, 0));
  b.clear(Color(200, 100, 50));
  transition(o, a, b, 1.f, 0);
  CHECK(o.pixels()[0] == 0xffc86432u);
  transition(o, a, b, 0.f, 0);
  CHECK(o.pixels()[0] == 0xff000000u);
}

// ---- the blitter ------------------------------------------------------------------------------------------------
static void testBlit() {
  SECTION("fbblit: every GSP format, column-major layout, alpha blending");
  for (int f = 0; f < 5; f++) {
    PixFmt fmt = (PixFmt)f;
    std::vector<uint8_t> mem((size_t)400 * 240 * bytesPerPixel(fmt), 0);
    FbTarget t = ctruTarget(mem.data(), 400, fmt);
    Canvas c(400, 240);
    c.clear(rgb(0x204080));
    c.fill(10, 20, 5, 5, rgb(0xff0000));
    blit(c, {0, 0, 400, 240}, t, 0, 0, false);
    Color a = readPixel(t, 12, 22), b = readPixel(t, 100, 100);
    int tol = f == 4 ? 17 : 8;
    CHECK(std::abs(a.r - 255) <= tol && a.g <= tol && a.b <= tol);
    CHECK(std::abs(b.r - 0x20) <= tol && std::abs(b.g - 0x40) <= tol && std::abs(b.b - 0x80) <= tol);
    // y = 0 is the end of the first column (the 3DS screen is stored rotated)
    CHECK(mem.size() >= 240u * bytesPerPixel(fmt));
    // blending a half-transparent white over it
    Canvas ov(400, 240);
    ov.fill(100, 100, 1, 1, Color(255, 255, 255, 128));
    blit(ov, {100, 100, 1, 1}, t, 100, 100, true);
    Color m = readPixel(t, 100, 100);
    CHECK(std::abs(m.r - (0x20 + 255) / 2) <= tol + 2 && std::abs(m.b - (0x80 + 255) / 2) <= tol + 2);
  }
  // the rotated layout itself
  std::vector<uint8_t> mem(400 * 240 * 3, 0);
  FbTarget t = ctruTarget(mem.data(), 400, PixFmt::BGR8);
  Canvas c(400, 240);
  c.fill(0, 0, 1, 1, rgb(0x010203));
  blit(c, {0, 0, 1, 1}, t, 0, 0, false);
  CHECK(mem[239 * 3] == 3 && mem[239 * 3 + 1] == 2 && mem[239 * 3 + 2] == 1);
}

// ---- UI smoke: every screen draws, touch targets work ------------------------------------------------------------
static void testUi() {
  SECTION("ui: screens draw; a tap on a tab switches it; the corner tab hit area");
  RunStore st;
  Rules r;
  r.applyPreset("classic");
  st.create("UI", "usum", r, 0);
  Run *run = st.activeRun();
  ViewCtx ctx;
  ctx.run = run;
  ctx.game = gameByKey("usum");
  RunView v;
  Canvas top(400, 240), bottom(320, 240);
  v.drawTop(top, ctx);
  v.drawBottom(bottom, ctx);
  Input in;
  in.touchDown = true;
  in.tx = 200;
  in.ty = 10;  // the "Friedhof" tab
  v.handle(in, ctx, 0);
  in = Input();
  in.touchUp = true;
  in.tx = 200;
  in.ty = 10;
  CHECK(v.handle(in, ctx, 0) == ViewAction::Changed && v.tab == 2);
  // Gen 7: the trials stepper changes the cap
  v.tab = 3;
  v.drawBottom(bottom, ctx);
  int capBefore = run->levelCap();
  in = Input();
  in.touchDown = true;
  in.tx = 320 - 52 + 10;
  in.ty = 30 + 4 + 16 + 10;
  v.handle(in, ctx, 0);
  in.touchDown = false;
  in.touchUp = true;
  v.handle(in, ctx, 0);
  CHECK(run->milestone == 1 && run->levelCap() > capBefore);
  CHECK(OverlayLayer::tabRect().contains(310, 230));
  // overlay layer: a toast fades in and out, nothing drawn when idle
  OverlayLayer ol;
  Canvas ov(400, 240);
  CHECK(ol.drawTop(ov, 0).w == 0);
  ol.push({EventKind::Info, Tone::Info, "a", "b"}, 1.0);
  CHECK(ol.drawTop(ov, 0.5).w == 0 && ol.drawTop(ov, 1.5).w > 0 && ol.drawTop(ov, 9.0).w == 0);
}

int main(int argc, char **argv) {
  std::string fixtures = argc > 1 ? argv[1] : "../test/fixtures";
  std::string dump = argc > 2 ? argv[2] : "";
  testPk6Fixtures(fixtures);
  testPartyReader();
  testBattle();
  testZones(dump);
  testJsonRun();
  testEngine();
  testEngineMoments();
  testTrophies();
  testScreens();
  testBlit();
  testUi();
  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
