#include "pk6.h"

#include <cstdio>
#include <cstring>

namespace sd {

static inline uint16_t u16(const uint8_t *b, int o) { return b[o] | (b[o + 1] << 8); }
static inline uint32_t u32(const uint8_t *b, int o) {
  return (uint32_t)b[o] | ((uint32_t)b[o + 1] << 8) | ((uint32_t)b[o + 2] << 16) | ((uint32_t)b[o + 3] << 24);
}

static const uint8_t BLOCK_POSITION[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 3, 1, 2}, {0, 2, 3, 1}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {2, 0, 1, 3}, {3, 0, 1, 2}, {2, 0, 3, 1}, {3, 0, 2, 1},
    {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 1, 0, 3}, {3, 1, 0, 2}, {2, 3, 0, 1}, {3, 2, 0, 1},
    {1, 2, 3, 0}, {1, 3, 2, 0}, {2, 1, 3, 0}, {3, 1, 2, 0}, {2, 3, 1, 0}, {3, 2, 1, 0},
};
static const uint8_t BLOCK_POSITION_INVERT[24] = {0, 1, 2, 4, 3, 5, 6, 7, 12, 18, 13, 19, 8, 10, 14, 20, 16, 22, 9, 11, 15, 21, 17, 23};
constexpr int BLOCK = 56;

static void cryptArray(uint8_t *d, int start, int end, uint32_t seed) {
  for (int i = start; i + 1 < end; i += 2) {
    seed = 0x41C64E6Du * seed + 0x6073u;
    uint16_t v = (uint16_t)((d[i] | (d[i + 1] << 8)) ^ (seed >> 16));
    d[i] = v & 0xff;
    d[i + 1] = v >> 8;
  }
}

bool decryptPK6(const uint8_t *src, int len, uint8_t *out) {
  uint8_t d[PK_SIZE];
  std::memcpy(d, src, len);
  uint32_t ec = u32(d, 0);
  cryptArray(d, 8, BOX_SIZE, ec);
  if (len > BOX_SIZE) cryptArray(d, BOX_SIZE, len, ec);
  const uint8_t *row = BLOCK_POSITION[((ec >> 13) & 31) % 24];
  std::memcpy(out, d, len);
  for (int i = 0; i < 4; i++) std::memcpy(out + 8 + i * BLOCK, d + 8 + row[i] * BLOCK, BLOCK);
  uint16_t sum = 0;
  for (int i = 8; i < BOX_SIZE; i += 2) sum = (uint16_t)(sum + u16(out, i));
  return sum == u16(out, 6);
}

void encryptPK6(const uint8_t *plain, int len, uint8_t *out) {
  uint8_t d[PK_SIZE];
  std::memcpy(d, plain, len);
  uint32_t ec = u32(d, 0);
  uint16_t sum = 0;
  for (int i = 8; i < BOX_SIZE; i += 2) sum = (uint16_t)(sum + u16(d, i));
  d[6] = sum & 0xff;
  d[7] = sum >> 8;
  const uint8_t *row = BLOCK_POSITION[BLOCK_POSITION_INVERT[((ec >> 13) & 31) % 24]];
  std::memcpy(out, d, len);
  for (int i = 0; i < 4; i++) std::memcpy(out + 8 + i * BLOCK, d + 8 + row[i] * BLOCK, BLOCK);
  cryptArray(out, 8, BOX_SIZE, ec);
  if (len > BOX_SIZE) cryptArray(out, BOX_SIZE, len, ec);
}

// UTF-16 → UTF-8; the gender signs sit in the private use area
static std::string text16(const uint8_t *d, int off, int maxChars) {
  std::string s;
  for (int i = 0; i < maxChars; i++) {
    uint32_t c = u16(d, off + i * 2);
    if (!c || c == 0xffff) break;
    if (c == 0xE08E) c = 0x2642;
    else if (c == 0xE08F) c = 0x2640;
    else if (c == 0xE08D) c = 0x2026;
    if (c < 0x80) s += (char)c;
    else if (c < 0x800) {
      s += (char)(0xC0 | (c >> 6));
      s += (char)(0x80 | (c & 0x3f));
    } else {
      s += (char)(0xE0 | (c >> 12));
      s += (char)(0x80 | ((c >> 6) & 0x3f));
      s += (char)(0x80 | (c & 0x3f));
    }
  }
  return s;
}

static const int STATUS_CODE[] = {0, 0x40, 1, 0x20, 0x10, 0x08};
static bool statusOk(uint32_t s) { return s <= 7 || s == 8 || s == 16 || s == 32 || s == 64 || s == 128; }
static int statusCode(uint32_t s) { return s < 6 ? STATUS_CODE[s] : s <= 7 ? 0 : (int)(s & 0xff); }

bool parsePK6(const uint8_t *raw, int gen, Mon &m) {
  uint8_t d[PK_SIZE];
  if (!decryptPK6(raw, PK_SIZE, d)) return false;
  int species = u16(d, 0x08);
  if (!species || species > (gen >= 7 ? 807 : 721)) return false;
  uint32_t iv32 = u32(d, 0x74);
  bool isEgg = (iv32 & 0x40000000u) != 0;
  uint32_t status = u32(d, 0xE8);
  int level = d[0xEC];
  int hp = u16(d, 0xF0), maxHp = u16(d, 0xF2);
  if (!statusOk(status)) return false;
  if (isEgg ? (level > 100 || hp > maxHp) : (level < 1 || level > 100 || !maxHp || hp > maxHp)) return false;
  for (int o = 0xF2; o < 0xFE; o += 2)
    if (u16(d, o) > 999) return false;
  uint32_t pid = u32(d, 0x18);
  int tid = u16(d, 0x0C), sid = u16(d, 0x0E);
  int formByte = d[0x1D];
  char key[40];
  std::snprintf(key, sizeof key, "%08lx-%d-%d", (unsigned long)pid, tid, sid);
  m.key = key;
  m.pid = pid;
  m.ec = u32(d, 0);
  m.species = species;
  m.form = formByte >> 3;
  m.nickname = text16(d, 0x40, 12);
  m.isEgg = isEgg;
  m.item = u16(d, 0x0A);
  for (int i = 0; i < 4; i++) m.moves[i] = u16(d, 0x5A + i * 2);
  m.metLocation = u16(d, 0xDA);
  m.eggLocation = u16(d, 0xD8);
  m.metLevel = d[0xDD] & 0x7f;
  int g = (formByte >> 1) & 3;
  m.gender = g == 0 ? 'M' : g == 1 ? 'F' : '-';
  m.shiny = ((uint32_t)(tid ^ sid ^ (pid >> 16) ^ (pid & 0xffff))) < 16;
  m.status = statusCode(status);
  m.level = level;
  m.hp = hp;
  m.maxHp = maxHp;
  return true;
}

static void slotBytes(const uint8_t *region, int i, uint8_t *out) {
  const uint8_t *at = region + i * SLOT_STRIDE;
  std::memcpy(out, at, BOX_SIZE);
  std::memcpy(out + BOX_SIZE, at + STATS_OFFSET, STATS_SIZE);
}

constexpr int BAD_EGG_READS = 3;

std::vector<Mon> partyFromRegion(const uint8_t *region, int gen, BadEggState *bad) {
  std::vector<Mon> party;
  for (int i = 0; i < 6; i++) {
    uint8_t raw[PK_SIZE];
    slotBytes(region, i, raw);
    bool zero = true;
    for (int k = 0; k < PK_SIZE; k++)
      if (raw[k]) {
        zero = false;
        break;
      }
    if (zero) break;
    Mon m;
    if (!parsePK6(raw, gen, m)) {
      uint8_t tmp[PK_SIZE];
      if (bad && !decryptPK6(raw, PK_SIZE, tmp)) {
        // the very same bytes failing the checksum several times: a Bad Egg, skipped instead of ending the party
        if (bad->n[i] && !std::memcmp(bad->sig[i], raw, 16)) bad->n[i]++;
        else {
          std::memcpy(bad->sig[i], raw, 16);
          bad->n[i] = 1;
        }
        if (bad->n[i] >= BAD_EGG_READS) continue;
      }
      if (i == 0) return {};
      break;
    }
    if (bad) bad->n[i] = 0;
    for (auto &x : party)
      if (x.pid == m.pid && x.ec == m.ec) return {};  // the party menu swapping two slots
    party.push_back(m);
  }
  return party;
}

PartyRead readParty(const GameInfo &g, MemRead read, void *ctx, PartyReaderState &st) {
  PartyRead res;
  static uint8_t region[PARTY_BYTES];
  int order[3], n = 0;
  if (st.addrIndex >= 0) order[n++] = st.addrIndex;
  for (int i = 0; i < g.partyAddrCount; i++)
    if (i != st.addrIndex) order[n++] = i;
  for (int k = 0; k < n; k++) {
    int i = order[k];
    const PartyAddr &a = g.partyAddrs[i];
    if (!read(a.party, region, PARTY_BYTES, ctx)) continue;
    std::vector<Mon> party = partyFromRegion(region, g.gen, &st.bad[i]);
    if (party.empty()) continue;
    st.addrIndex = i;
    std::string sig;
    for (auto &m : party) sig += m.key + ",";
    if (st.lastLen && (int)party.size() < st.lastLen && st.shrinkSig != sig) {
      st.shrinkSig = sig;
      return res;  // fewer than last time: only once a second read agrees
    }
    st.shrinkSig.clear();
    st.lastLen = (int)party.size();
    res.ok = true;
    res.party = std::move(party);
    res.version = a.version;
    if (a.badges) {
      uint8_t b;
      if (read(a.badges, &b, 1, ctx)) {
        int c = 0;
        for (int bit = 0; bit < 8; bit++) c += (b >> bit) & 1;
        res.badges = c;
      }
    }
    return res;
  }
  return res;
}

struct Battler {
  int species, maxHp, hp, level, slot;
};

static int battleMons(const uint8_t *bytes, int stride, int gen, int blocks, Battler *out) {
  int n = 0;
  for (int i = 0; i < blocks; i++) {
    const uint8_t *o = bytes + i * stride;
    int species = u16(o, 4), maxHp = u16(o, 6), hp = u16(o, 8), level = o[0x10], slot = o[0x11];
    if (!species || species > (gen >= 7 ? 807 : 721) || !maxHp || maxHp > 999 || hp > maxHp || level < 1 || level > 100) continue;
    out[n++] = {species, maxHp, hp, level, slot};
  }
  return n;
}

BattleRead readBattle(const GameInfo &g, MemRead read, void *ctx, const std::vector<Mon> *party) {
  BattleRead r;
  const BattleAddrs &b = g.battle;
  if (!b.wild && !b.trainer) return r;
  static uint8_t buf[816 * 24];
  int stride = b.stride, blocks = b.blocks;
  if (stride * blocks > (int)sizeof buf) return r;
  Battler mons[24];
  int n = 0;
  bool trainer = false;
  if (b.inBattle) {
    uint8_t flag = 0;
    if (!read(b.inBattle, &flag, 1, ctx) || !flag) return r;
    uint8_t t = 0;
    if (b.isTrainer && read(b.isTrainer, &t, 1, ctx)) trainer = t != 0;
    if (!read(b.wild ? b.wild : b.trainer, buf, stride * blocks, ctx)) return r;
    n = battleMons(buf, stride, g.gen, blocks, mons);
  } else {
    const uint32_t bases[2] = {b.wild, b.trainer};
    for (int k = 0; k < 2 && !n; k++) {
      if (!bases[k] || !read(bases[k], buf, stride * blocks, ctx)) continue;
      Battler m[24];
      int c = battleMons(buf, stride, g.gen, blocks, m);
      bool mine = false, foe = false;
      for (int i = 0; i < c; i++) {
        if (m[i].slot < 6) mine = true;
        if (m[i].slot >= 12) foe = true;
      }
      if (mine && foe) {
        std::memcpy(mons, m, sizeof(Battler) * c);
        n = c;
        trainer = k == 1;
      }
    }
  }
  const Battler *me = nullptr, *foe = nullptr, *firstFoe = nullptr;
  for (int i = 0; i < n; i++) {
    if (mons[i].slot < 6 && !me) me = &mons[i];
    if (mons[i].slot >= 12) {
      if (!firstFoe) firstFoe = &mons[i];
      if (!foe && mons[i].hp > 0) foe = &mons[i];
    }
  }
  if (!foe) foe = firstFoe;
  if (!me || !foe) return r;
  r.inBattle = true;
  r.trainer = trainer;
  r.enemySpecies = foe->species;
  r.enemyLevel = foe->level;
  r.enemyHp = foe->hp;
  r.enemyMaxHp = foe->maxHp;
  r.activeSpecies = me->species;
  r.activeLevel = me->level;
  r.activeHp = me->hp;
  r.activeMaxHp = me->maxHp;
  if (party)
    for (size_t i = 0; i < party->size(); i++) {
      const Mon &p = (*party)[i];
      if (p.species == me->species && p.level == me->level && p.maxHp == me->maxHp) {
        r.activeIndex = (int)i;
        break;
      }
    }
  return r;
}

int readZone(const GameInfo &g, MemRead read, void *ctx) {
  if (!g.zoneAddr) return -1;
  uint8_t b[2];
  if (!read(g.zoneAddr, b, 2, ctx)) return -1;
  return b[0] | (b[1] << 8);
}

}  // namespace sd
