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

#if DEBUG || RELEASE
#define xPARTICLEBATCHGROUP_CREATE_SYSTEM_L(group, system, line) (group).create_system((system), __FILE__, (line))
#define xPARTICLEBATCHGROUP_CREATE_SYSTEM_NAMED(group, system, name) (group).create_system((system), (name), 0)
#else
#define xPARTICLEBATCHGROUP_CREATE_SYSTEM_L(group, system, line) (group).create_system((system))
#define xPARTICLEBATCHGROUP_CREATE_SYSTEM_NAMED(group, system, name) (group).create_system((system))
#endif

#define xPARTICLEBATCHGROUP_CREATE_SYSTEM(group, system) xPARTICLEBATCHGROUP_CREATE_SYSTEM_L(group, system, __LINE__)

void xParticleBatchSceneEnter();
void xParticleBatchSceneExit();
void xParticleBatchReset();
void xParticleBatchPreRender();

#if DEBUG || RELEASE
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system, const char* dbgFile, S32 dbgLine);
#define xPARTICLEBATCHADDSYSTEM_L(system, line) xParticleBatchAddSystem((system), __FILE__, (line))
#define xPARTICLEBATCHADDSYSTEM_NAMED(system, name) xParticleBatchAddSystem((system), (name), 0)
#else
S32 xParticleBatchAddSystem(const xParticleBatchSystem& system);
#define xPARTICLEBATCHADDSYSTEM_L(system, line) xParticleBatchAddSystem((system))
#define xPARTICLEBATCHADDSYSTEM_NAMED(system, name) xParticleBatchAddSystem((system))
#endif

#define xPARTICLEBATCHADDSYSTEM(system) xPARTICLEBATCHADDSYSTEM_L(system, __LINE__)
void xParticleBatchReplaceSystem(S32 system_id, const xParticleBatchSystem& system);
F32 xParticleBatchRequest(S32 system_id, F32 amount, S32 max_blocks);
S32 xParticleBatchEmit(S32 system_id, S32 amount, U8** mem);
U8* xParticleBatchEmit(S32 system_id); // UNUSED

#if DEBUG || RELEASE
void xParticleBatchDebugCB();
#endif

#endif
