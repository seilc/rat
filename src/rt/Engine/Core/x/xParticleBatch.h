#ifndef XPARTICLEBATCH_H
#define XPARTICLEBATCH_H

#include "xPtankPool.h"

struct xParticleBatchSystem
{
    ptank_group_type pool_type;
    ptank_pool::render_state rs;
    U32 order_group;
    S32 order_index;
    S32 stride;
    S32 (*update)(U8*, S32, ptank_pool&, F32, void*);
    void* context;
};

#if DEBUG || RELEASE
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system, const char* dbgFile, S32 dbgLine);
#define xPARTICLEBATCHADDSYSTEM(system, line) xParticleBatchAddSystem((system), __FILE__, (line))
#else
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system);
#define xPARTICLEBATCHADDSYSTEM(system, line) xParticleBatchAddSystem((system))
#endif
void xParticleBatchReplaceSystem(S32 system_id, const xParticleBatchSystem& system);
S32 xParticleBatchEmit(S32 system_id, S32 amount, U8** mem);

#endif
