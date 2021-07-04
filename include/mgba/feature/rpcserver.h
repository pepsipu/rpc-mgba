#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <mgba/core/core.h>

#if __cplusplus
extern "C" {
#endif
void setCoreRPC(struct mCore* core, struct GBA* gba);
extern bool rpc_servicing;
extern struct GBA* rpc_gba;
#if __cplusplus
};
#endif

#endif