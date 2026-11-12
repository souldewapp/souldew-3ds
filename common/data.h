// Static game data (generated: common/data_gen.cpp by tools/gen_data.mjs).
#pragma once

namespace sd {

enum class Lang { De, En };

struct LocName {
  int id;
  const char *en, *de;
};

extern const int kMaxSpecies;  // 807 (Gen 7)

const char *speciesName(int id, Lang lang);
int speciesType(int id, int which);  // 0..17 (normal … fairy), -1 = none
int speciesFamily(int id);           // evolution chain id: same family = same number
const char *typeName(int t, Lang lang);
const char *locationName(int gen, int id, Lang lang);  // met location name, nullptr if unknown

}  // namespace sd
