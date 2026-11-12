// Client for Luma3DS' plugin loader service ("plg:ldr", Rosalina) – just what the launcher needs: is it on, switch it
// on, and "load this plugin for the next start of title X". IPC layout as in Luma3DS (sysmodules/rosalina, plgldr).
#pragma once
#include <3ds.h>

typedef struct {
  bool noFlash;        // no screen flash when the plugin loads
  u32 lowTitleId;      // the title the parameters are for (low 32 bits of its title ID)
  char path[256];      // SD path of the .3gx, starting with '/'
  u32 config[32];      // handed to the plugin
} PluginLoadParameters;

Result plgLdrInit(void);
void plgLdrExit(void);
Result PLGLDR__IsPluginLoaderEnabled(bool *isEnabled);
Result PLGLDR__SetPluginLoaderState(bool enabled);
Result PLGLDR__SetPluginLoadParameters(PluginLoadParameters *parameters);
