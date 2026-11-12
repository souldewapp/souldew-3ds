// German (default, like the desktop app's main audience) and English UI texts: tr("Deutsch", "English").
#pragma once
#include <string>
#include "data.h"

namespace sd {

extern Lang g_lang;
inline const char *tr(const char *de, const char *en) { return g_lang == Lang::En ? en : de; }
std::string fmt(const char *f, ...);

}  // namespace sd
