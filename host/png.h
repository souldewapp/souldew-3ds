// Minimal PNG writer (RGB, zlib "stored" blocks) – no dependencies, for the host renderer.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace png {

inline uint32_t crc(const uint8_t *d, size_t n, uint32_t c = 0xffffffffu) {
  static uint32_t t[256];
  static bool init = false;
  if (!init) {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t v = i;
      for (int k = 0; k < 8; k++) v = v & 1 ? 0xedb88320u ^ (v >> 1) : v >> 1;
      t[i] = v;
    }
    init = true;
  }
  for (size_t i = 0; i < n; i++) c = t[(c ^ d[i]) & 0xff] ^ (c >> 8);
  return c;
}

inline void be32(std::vector<uint8_t> &o, uint32_t v) {
  o.push_back(v >> 24); o.push_back(v >> 16); o.push_back(v >> 8); o.push_back(v);
}

inline void chunk(std::vector<uint8_t> &o, const char *type, const std::vector<uint8_t> &data) {
  be32(o, (uint32_t)data.size());
  std::vector<uint8_t> td(type, type + 4);
  td.insert(td.end(), data.begin(), data.end());
  o.insert(o.end(), td.begin(), td.end());
  be32(o, crc(td.data(), td.size()) ^ 0xffffffffu);
}

// rgb: w*h*3 bytes
inline bool write(const std::string &path, int w, int h, const std::vector<uint8_t> &rgb) {
  std::vector<uint8_t> raw;
  for (int y = 0; y < h; y++) {
    raw.push_back(0);
    raw.insert(raw.end(), rgb.begin() + (size_t)y * w * 3, rgb.begin() + (size_t)(y + 1) * w * 3);
  }
  std::vector<uint8_t> z = {0x78, 0x01};
  uint32_t a = 1, b = 0;
  for (uint8_t v : raw) { a = (a + v) % 65521; b = (b + a) % 65521; }
  for (size_t p = 0; p < raw.size();) {
    size_t n = std::min<size_t>(65535, raw.size() - p);
    bool last = p + n == raw.size();
    z.push_back(last ? 1 : 0);
    z.push_back(n & 0xff); z.push_back(n >> 8);
    z.push_back(~n & 0xff); z.push_back((~n >> 8) & 0xff);
    z.insert(z.end(), raw.begin() + p, raw.begin() + p + n);
    p += n;
  }
  be32(z, (b << 16) | a);
  std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  std::vector<uint8_t> ihdr;
  be32(ihdr, w); be32(ihdr, h);
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
  chunk(out, "IHDR", ihdr);
  chunk(out, "IDAT", z);
  chunk(out, "IEND", {});
  FILE *f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  std::fwrite(out.data(), 1, out.size(), f);
  std::fclose(f);
  return true;
}

}  // namespace png
