#ifndef XPARTICLEBATCH_H
#define XPARTICLEBATCH_H

#include "xPtankPool.h"

struct tweak_info;

typedef S32(*xParticleBatchUpdateCallback)(U8*, S32, ptank_pool&, F32, void*);

struct xParticleBatchSystem
{
    ptank_group_type pool_type;
    ptank_pool::render_state rs;
    U32 order_group;
    S32 order_index;
    S32 stride;
    xParticleBatchUpdateCallback update;
    void* context;
};

class xParticleBatchGroup
{
    U8* elements;
    S32 elements_size;

public:
    void create();
#if DEBUG || RELEASE
    S32 create_system(const xParticleBatchSystem& system, const char* dbgFile, S32 dbgLine);
    void add_tweaks(S32* system_id, const char* prefix);
    static void on_reload(const tweak_info& info);
#else
    S32 create_system(const xParticleBatchSystem& system);
#endif
};

void xParticleBatchSceneEnter();
void xParticleBatchSceneExit();
void xParticleBatchReset();
void xParticleBatchPreRender();

#if DEBUG || RELEASE
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system, const char* dbgFile, S32 dbgLine);
#define xPARTICLEBATCHADDSYSTEM_L(system, line) xParticleBatchAddSystem((system), __FILE__, (line))
#else
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system);
#define xPARTICLEBATCHADDSYSTEM_L(system, line) xParticleBatchAddSystem((system))
#endif

#define xPARTICLEBATCHADDSYSTEM(system) xPARTICLEBATCHADDSYSTEM_L(system, __LINE__)
void xParticleBatchReplaceSystem(S32 system_id, const xParticleBatchSystem& system);
F32 xParticleBatchRequest(S32 system_id, F32 amount, S32 max_blocks);
S32 xParticleBatchEmit(S32 system_id, S32 amount, U8** mem);
U8* xParticleBatchEmit(S32 system_id);

#if DEBUG || RELEASE
void xParticleBatchDebugCB();
#endif

#endif
