// Which place (met location) each zone of a 3DS Pokémon game belongs to, from the game's own RomFS – a port of the
// souldew desktop app's native/shared/live/ctrzones.mjs (pk3DS layouts: X/Y a/0/1/2 and ORAS a/0/1/3 encounter
// GARCs, 56 bytes per zone, place at +0x1C (9 bits); SM/USUM a/0/7/7 file 0, LZ11, 0x54 bytes, u32 at +0x1C).
//
// On the console the plugin reads the RomFS of the game it runs in (FS ARCHIVE_ROMFS: the IVFC image), on a computer
// the tests feed a synthetic NCCH / IVFC image. Both go through `readAt(offset, length)`.
#pragma once
#include <cstdint>
#include <vector>
#include "games.h"

namespace sd {

using ReadAt = bool (*)(uint64_t off, void *out, uint32_t len, void *ctx);

enum class ImageKind { Auto, Ncch, Ivfc, Level3 };

struct RomfsFile {
  uint64_t offset = 0, size = 0;
  bool ok = false;
};

// a file by path ("a/0/1/2") in a RomFS image
RomfsFile romfsFile(ReadAt readAt, void *ctx, const char *path, ImageKind kind = ImageKind::Auto);

// file `index` (negative: from the end) of a GARC at `at`
bool garcFile(ReadAt readAt, void *ctx, uint64_t at, int index, std::vector<uint8_t> &out, uint32_t maxSize = 1u << 20);

// LZ11 (0x11 header) → unpacked; returns the input unchanged when it isn't LZ11, empty on damage
std::vector<uint8_t> lz11(const std::vector<uint8_t> &src, uint32_t maxSize = 4u << 20);

// places[zone] = met location id; empty on failure
std::vector<uint16_t> zonePlaces(const GameInfo &g, ReadAt readAt, void *ctx, ImageKind kind = ImageKind::Auto);

}  // namespace sd
