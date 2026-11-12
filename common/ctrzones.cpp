#include "ctrzones.h"

#include <cstring>
#include <string>

namespace sd {

static inline uint16_t u16(const uint8_t *b, size_t o) { return b[o] | (b[o + 1] << 8); }
static inline uint32_t u32(const uint8_t *b, size_t o) {
  return (uint32_t)b[o] | ((uint32_t)b[o + 1] << 8) | ((uint32_t)b[o + 2] << 16) | ((uint32_t)b[o + 3] << 24);
}
static inline uint64_t u64(const uint8_t *b, size_t o) { return u32(b, o) | ((uint64_t)u32(b, o + 4) << 32); }
static inline bool tag(const uint8_t *b, size_t o, const char *t) { return !std::memcmp(b + o, t, 4); }

struct Level3 {
  uint64_t base;
  uint32_t dirOff, dirLen, fileOff, fileLen, data;
};

// the level-3 header (the files) of an IVFC image starting at `off`
static bool ivfcLevel3(ReadAt rd, void *ctx, uint64_t off, uint64_t &l3) {
  uint8_t iv[0x60];
  if (!rd(off, iv, sizeof iv, ctx) || !tag(iv, 0, "IVFC")) return false;
  uint32_t masterHash = u32(iv, 0x08);
  uint32_t blockLog = u32(iv, 0x4C);
  if (blockLog > 20) return false;
  uint64_t block = 1ull << blockLog;
  l3 = off + ((0x60 + masterHash + block - 1) / block) * block;
  return true;
}

static bool readLevel3(ReadAt rd, void *ctx, uint64_t l3, Level3 &out) {
  uint8_t h[0x28];
  if (!rd(l3, h, sizeof h, ctx) || u32(h, 0) != 0x28) return false;
  out = {l3, u32(h, 0x0C), u32(h, 0x10), u32(h, 0x1C), u32(h, 0x20), u32(h, 0x24)};
  return true;
}

static bool findLevel3(ReadAt rd, void *ctx, ImageKind kind, Level3 &out) {
  uint8_t h[0x200];
  if (kind == ImageKind::Auto || kind == ImageKind::Ncch) {
    if (rd(0, h, sizeof h, ctx)) {
      uint64_t n = (uint64_t)-1;
      if (tag(h, 0x100, "NCSD")) n = (uint64_t)u32(h, 0x120) * (0x200ull << h[0x188 + 6]);
      else if (tag(h, 0x100, "NCCH")) n = 0;
      if (n != (uint64_t)-1) {
        uint8_t c[0x200];
        if (!rd(n, c, sizeof c, ctx) || !tag(c, 0x100, "NCCH")) return false;
        if (!(c[0x18F] & 0x04)) return false;  // encrypted contents
        uint64_t unit = 0x200ull << c[0x188 + 6];
        if (!u32(c, 0x1B4)) return false;
        uint64_t l3;
        if (!ivfcLevel3(rd, ctx, n + u32(c, 0x1B0) * unit, l3)) return false;
        return readLevel3(rd, ctx, l3, out);
      }
      if (kind == ImageKind::Ncch) return false;
    }
  }
  if (kind == ImageKind::Auto || kind == ImageKind::Ivfc) {
    uint64_t l3;
    if (ivfcLevel3(rd, ctx, 0, l3)) return readLevel3(rd, ctx, l3, out);
    if (kind == ImageKind::Ivfc) return false;
  }
  // FS's RomFS archive of the running title may also start right at level 3
  return readLevel3(rd, ctx, 0, out);
}

static bool nameIs(const std::vector<uint8_t> &b, size_t o, uint32_t len, const std::string &want) {
  if (len != want.size() * 2 || o + len > b.size()) return false;
  for (size_t i = 0; i < want.size(); i++)
    if (u16(b.data(), o + i * 2) != (unsigned char)want[i]) return false;
  return true;
}

RomfsFile romfsFile(ReadAt rd, void *ctx, const char *path, ImageKind kind) {
  RomfsFile res;
  Level3 fs;
  if (!findLevel3(rd, ctx, kind, fs)) return res;
  if (fs.dirLen > (4u << 20) || fs.fileLen > (8u << 20)) return res;
  std::vector<uint8_t> dirs(fs.dirLen), files(fs.fileLen);
  if (!rd(fs.base + fs.dirOff, dirs.data(), fs.dirLen, ctx) || !rd(fs.base + fs.fileOff, files.data(), fs.fileLen, ctx)) return res;
  std::vector<std::string> parts;
  std::string p = path, cur;
  for (char c : p) {
    if (c == '/') {
      parts.push_back(cur);
      cur.clear();
    } else cur += c;
  }
  parts.push_back(cur);
  uint32_t dir = 0;
  for (size_t k = 0; k < parts.size(); k++) {
    bool last = k + 1 == parts.size();
    if (!last) {
      if (dir + 0x18 > dirs.size()) return res;
      uint32_t child = u32(dirs.data(), dir + 0x08);
      int64_t found = -1;
      for (int guard = 0; child != 0xffffffffu && guard < 100000; guard++) {
        if (child + 0x18 > dirs.size()) return res;
        if (nameIs(dirs, child + 0x18, u32(dirs.data(), child + 0x14), parts[k])) {
          found = child;
          break;
        }
        child = u32(dirs.data(), child + 0x04);
      }
      if (found < 0) return res;
      dir = (uint32_t)found;
    } else {
      if (dir + 0x18 > dirs.size()) return res;
      uint32_t f = u32(dirs.data(), dir + 0x0C);
      for (int guard = 0; f != 0xffffffffu && guard < 100000; guard++) {
        if (f + 0x20 > files.size()) return res;
        if (nameIs(files, f + 0x20, u32(files.data(), f + 0x1C), parts[k])) {
          res.offset = fs.base + fs.data + u64(files.data(), f + 0x08);
          res.size = u64(files.data(), f + 0x10);
          res.ok = true;
          return res;
        }
        f = u32(files.data(), f + 0x04);
      }
    }
  }
  return res;
}

bool garcFile(ReadAt rd, void *ctx, uint64_t at, int index, std::vector<uint8_t> &out, uint32_t maxSize) {
  uint8_t h[0x1C];
  if (!rd(at, h, sizeof h, ctx) || !tag(h, 0, "CRAG")) return false;
  uint32_t headerSize = u32(h, 4), dataOffset = u32(h, 0x10);
  uint8_t fato[0x0C];
  if (!rd(at + headerSize, fato, sizeof fato, ctx) || !tag(fato, 0, "OTAF")) return false;
  uint64_t fatbAt = at + headerSize + u32(fato, 4);
  uint8_t fh[0x0C];
  if (!rd(fatbAt, fh, sizeof fh, ctx) || !tag(fh, 0, "BTAF")) return false;
  uint32_t size = u32(fh, 4), count = u32(fh, 8);
  if (size > (4u << 20) || size < 0x0C || !count || count > (size - 0x0C) / 4) return false;
  std::vector<uint8_t> fatb(size);
  if (!rd(fatbAt, fatb.data(), size, ctx)) return false;
  int64_t want = index < 0 ? (int64_t)count + index : index;
  if (want < 0 || want >= count) return false;
  size_t p = 0x0C;
  for (uint32_t i = 0; i < count; i++) {
    if (p + 4 > fatb.size()) return false;
    uint32_t bits = u32(fatb.data(), p);
    p += 4;
    bool have = false;
    uint32_t start = 0, length = 0;
    for (int b = 0; b < 32; b++) {
      if (!(bits & (1u << b))) continue;
      if (p + 12 > fatb.size()) return false;
      if (!have) {
        start = u32(fatb.data(), p);
        length = u32(fatb.data(), p + 8);
        have = true;
      }
      p += 12;
    }
    if (i == want) {
      if (!have || !length || length > maxSize) return false;
      out.resize(length);
      return rd(at + dataOffset + start, out.data(), length, ctx);
    }
  }
  return false;
}

std::vector<uint8_t> lz11(const std::vector<uint8_t> &src, uint32_t maxSize) {
  if (src.size() < 4 || src[0] != 0x11) return src;
  uint32_t size = src[1] | (src[2] << 8) | (src[3] << 16);
  size_t p = 4;
  if (!size) {
    if (src.size() < 8) return {};
    size = u32(src.data(), 4);
    p = 8;
  }
  if (size > maxSize) return {};
  std::vector<uint8_t> out(size);
  size_t o = 0;
  while (o < size && p < src.size()) {
    uint8_t flags = src[p++];
    for (int bit = 7; bit >= 0 && o < size; bit--) {
      if (!(flags & (1 << bit))) {
        if (p >= src.size()) return {};
        out[o++] = src[p++];
        continue;
      }
      if (p >= src.size()) return {};
      int hi = src[p] >> 4;
      size_t need = hi == 0 ? 3 : hi == 1 ? 4 : 2;
      if (p + need > src.size()) return {};
      uint32_t len, disp;
      uint8_t b0 = src[p];
      if (hi == 0) {
        len = (((b0 & 0xf) << 4) | (src[p + 1] >> 4)) + 0x11;
        disp = (((src[p + 1] & 0xf) << 8) | src[p + 2]) + 1;
      } else if (hi == 1) {
        len = (((b0 & 0xf) << 12) | (src[p + 1] << 4) | (src[p + 2] >> 4)) + 0x111;
        disp = (((src[p + 2] & 0xf) << 8) | src[p + 3]) + 1;
      } else {
        len = hi + 1;
        disp = (((b0 & 0xf) << 8) | src[p + 1]) + 1;
      }
      p += need;
      if (disp > o) return {};
      for (uint32_t i = 0; i < len && o < size; i++, o++) out[o] = out[o - disp];
    }
  }
  if (o != size) return {};
  return out;
}

std::vector<uint16_t> zonePlaces(const GameInfo &g, ReadAt rd, void *ctx, ImageKind kind) {
  const char *path;
  int files[2];
  int nfiles;
  size_t entry;
  switch (g.id) {
    case GameId::XY: path = "a/0/1/2"; files[0] = -1; files[1] = -2; nfiles = 2; entry = 56; break;
    case GameId::ORAS: path = "a/0/1/3"; files[0] = -2; files[1] = -1; nfiles = 2; entry = 56; break;
    case GameId::SM:
    case GameId::USUM: path = "a/0/7/7"; files[0] = 0; nfiles = 1; entry = 0x54; break;
    default: return {};
  }
  RomfsFile f = romfsFile(rd, ctx, path, kind);
  if (!f.ok) return {};
  std::vector<uint8_t> table;
  for (int k = 0; k < nfiles; k++) {
    std::vector<uint8_t> raw;
    if (!garcFile(rd, ctx, f.offset, files[k], raw)) continue;
    std::vector<uint8_t> b = lz11(raw);
    if (b.size() >= entry && b.size() % entry == 0) {
      table = std::move(b);
      break;
    }
  }
  std::vector<uint16_t> places;
  for (size_t o = 0; o + entry <= table.size(); o += entry)
    places.push_back(g.gen == 6 ? (uint16_t)(table[o + 0x1C] | ((table[o + 0x1D] & 1) << 8)) : (uint16_t)u32(table.data(), o + 0x1C));
  return places;
}

}  // namespace sd
