#include "plgldr.h"

static Handle s_handle = 0;

Result plgLdrInit(void) {
  if (s_handle) return 0;
  return svcConnectToPort(&s_handle, "plg:ldr");
}

void plgLdrExit(void) {
  if (s_handle) svcCloseHandle(s_handle);
  s_handle = 0;
}

static Result call(u32 *cmd) {
  Result r = svcSendSyncRequest(s_handle);
  return R_FAILED(r) ? r : (Result)cmd[1];
}

Result PLGLDR__IsPluginLoaderEnabled(bool *isEnabled) {
  u32 *cmd = getThreadCommandBuffer();
  cmd[0] = IPC_MakeHeader(2, 0, 0);
  Result r = call(cmd);
  if (R_SUCCEEDED(r)) *isEnabled = cmd[2] != 0;
  return r;
}

Result PLGLDR__SetPluginLoaderState(bool enabled) {
  u32 *cmd = getThreadCommandBuffer();
  cmd[0] = IPC_MakeHeader(3, 1, 0);
  cmd[1] = enabled ? 1 : 0;
  return call(cmd);
}

Result PLGLDR__SetPluginLoadParameters(PluginLoadParameters *p) {
  u32 *cmd = getThreadCommandBuffer();
  cmd[0] = IPC_MakeHeader(4, 2, 4);
  cmd[1] = p->noFlash ? 1 : 0;
  cmd[2] = p->lowTitleId;
  cmd[3] = IPC_Desc_Buffer(256, IPC_BUFFER_R);
  cmd[4] = (u32)p->path;
  cmd[5] = IPC_Desc_Buffer(32 * sizeof(u32), IPC_BUFFER_R);
  cmd[6] = (u32)p->config;
  return call(cmd);
}
