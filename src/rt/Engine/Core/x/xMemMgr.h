#ifndef XMEMMGR_H
#define XMEMMGR_H

#include "iMemMgr.h"
#include "xDebug.h"

#include <stddef.h>
#include <new>

struct xMemPool;
typedef void(*xMemPoolInitCallback)(xMemPool*, void*);

struct xMemPool
{
    void* FreeList;
    U16 NextOffset;
    U16 Flags;
    void* UsedList;
    xMemPoolInitCallback InitCB;
    void* Buffer;
    U16 Size;
    U16 NumRealloc;
    U32 Total;
#if DEBUG
    U16 Allocated;
    U16 Pad0;
#endif
};

enum eMemMgrTag
{
    eMemMgrTag_2D,
    eMemMgrTag_Anim,
    eMemMgrTag_Audio,
    eMemMgrTag_Bomb,
    eMemMgrTag_Camera,
    eMemMgrTag_Cinematic,
    eMemMgrTag_Components,
    eMemMgrTag_Destructible,
    eMemMgrTag_Debug,
    eMemMgrTag_Decal,
    eMemMgrTag_Duplicator,
    eMemMgrTag_Event,
    eMemMgrTag_FMV,
    eMemMgrTag_FXActionLine,
    eMemMgrTag_FXRibbon,
    eMemMgrTag_Enemy,
    eMemMgrTag_EnergyStream,
    eMemMgrTag_EntityMisc,
    eMemMgrTag_EnvSpatial,
    eMemMgrTag_EnvAbstract,
    eMemMgrTag_Explosion,
    eMemMgrTag_Flame,
    eMemMgrTag_FlameThrower,
    eMemMgrTag_ForceFieldEffect,
    eMemMgrTag_FrozenObject,
    eMemMgrTag_Grass,
    eMemMgrTag_HierarchyBound,
    eMemMgrTag_HUD,
    eMemMgrTag_IncrediSlam,
    eMemMgrTag_Im3DHelper,
    eMemMgrTag_LaserBeam,
    eMemMgrTag_LaserBolt,
    eMemMgrTag_Light,
    eMemMgrTag_Lightning,
    eMemMgrTag_Lightweight,
    eMemMgrTag_Message,
    eMemMgrTag_Model,
    eMemMgrTag_MotionBlur,
    eMemMgrTag_Movable,
    eMemMgrTag_MovePoint,
    eMemMgrTag_NavigationMesh,
    eMemMgrTag_NPCFX,
    eMemMgrTag_NPCWalls,
    eMemMgrTag_NPCGate,
    eMemMgrTag_OneLiner,
    eMemMgrTag_OrdArrayMgr,
    eMemMgrTag_Packer,
    eMemMgrTag_PackerAssetPool,
    eMemMgrTag_PackerDefault,
    eMemMgrTag_PackerTOC,
    eMemMgrTag_PackerUnknown,
    eMemMgrTag_Particle,
    eMemMgrTag_Physics,
    eMemMgrTag_Player,
    eMemMgrTag_Projectile,
    eMemMgrTag_RenderBuffer,
    eMemMgrTag_Rubble,
    eMemMgrTag_Scene,
    eMemMgrTag_ScreenFX,
    eMemMgrTag_Serializer,
    eMemMgrTag_Shadow,
    eMemMgrTag_Shower,
    eMemMgrTag_SmashRing,
    eMemMgrTag_SoundRAMAsset,
    eMemMgrTag_Splash,
    eMemMgrTag_Spline,
    eMemMgrTag_SpotLight,
    eMemMgrTag_Subtitle,
    eMemMgrTag_Texture,
    eMemMgrTag_TextureStream,
    eMemMgrTag_Throwable,
    eMemMgrTag_ThrowableSystem,
    eMemMgrTag_Trigger,
    eMemMgrTag_UberLaser,
    eMemMgrTag_UI,
    eMemMgrTag_UpdateCullMgr,
    eMemMgrTag_VideoRAMAsset,
    eMemMgrTag_ReactiveAnim,
    eMemMgrTag_RenderWare,
    eMemMgrTag_Shrapnel,
    eMemMgrTag_Water,
    eMemMgrTag_WaterHose,
    eMemMgrTag_AnimCustom,
    eMemMgrTag_SwitchLever,
    eMemMgrTag_PlayerFX,
    eMemMgrTag_RenderWareMisc,
    eMemMgrTag_DecalRWMalloc,
    eMemMgrTag_Heists,
    eMemMgrTag_MiniGame,
    eMemMgrTag_CookingMinigame,
    eMemMgrTag_NumTags
};

// Not in DWARF data
enum xMemStaticType
{
    eMemStaticType_0, // unknown
};

struct xMemArea
{
    U32 addr;
    U32 size;
    U32 flags;
};

struct xMemInfo
{
    xMemArea system;
    xMemArea stack;
    xMemArea DRAM;
};

#define XMEMMAXSTACK 12

enum { XMEM_MAX_HEAPS = 2 };

typedef void(*xMemFreeFunc)(void*);
typedef void(*xMemBaseNotifyFunc)();

struct xMemFreeLink
{
    xMemFreeLink* next;
    xMemFreeFunc func;
    void* param;
};

#if DEBUG || RELEASE
class xMemMgrUsageTracker
{
public:
    void Init();
    void TrackAllocation(U32 size, U32 tag);
    U32 GetMemUsageByTag(U32 tag) const;

private:
    U32 usageByTag[eMemMgrTag_NumTags];
};
#endif

struct xHeapState
{
    U32 curr;
    U16 blk_ct;
    U16 pad;
    U32 used;
    U32 wasted;
    xMemFreeLink* free_funcs;
#if DEBUG || RELEASE
    xMemMgrUsageTracker memoryUsed;
#endif
};

struct xMemBlock
{
    U32 addr;
    U32 size;
    S32 align;
#if DEBUG
    U32 assetID;
    const char* file;
    S32 line;
    const char* func;
#endif
};

struct xMemHeap
{
    U32 flags;
    U32 hard_base;
    U32 size;
    S16 opp_heap[2];
    xHeapState state[XMEMMAXSTACK];
    U16 state_idx;
    U16 max_blks;
    xMemBlock* blk;
    xMemBlock* lastblk;
};

extern xMemInfo gMemInfo;
extern U32 gActiveHeap;
extern xMemHeap gxHeap[XMEM_MAX_HEAPS];
extern U32 scratch_mem_used;

#if DEBUG || RELEASE
#define xMEMGROWALLOC(size, tag) xMemGrowAlloc(gActiveHeap, (size), (tag))
#else
#define xMEMGROWALLOC(size, tag) xMemGrowAlloc(gActiveHeap, (size))
#endif

#if DEBUG
#define xMEMALLOC_L(size, align, tag, assetID, line) xMemAlloc(gActiveHeap, (size), (align), (tag), (assetID), __FILE__, __FUNCTION__, (line))
#elif RELEASE
#define xMEMALLOC_L(size, align, tag, assetID, line) xMemAlloc(gActiveHeap, (size), (align), (tag))
#else
#define xMEMALLOC_L(size, align, tag, assetID, line) xMemAlloc(gActiveHeap, (size), (align))
#endif

#define xMEMALLOC(size, align, tag, assetID) xMEMALLOC_L(size, align, tag, assetID, __LINE__)

#if DEBUG
#define xMEMPUSHTEMP_L(amt, assetID, line) xMemPushTemp((amt), (assetID), __FILE__, __FUNCTION__, (line))
#else
#define xMEMPUSHTEMP_L(amt, assetID, line) xMemPushTemp((amt))
#endif

#define xMEMPUSHTEMP(amt, assetID) xMEMPUSHTEMP_L(amt, assetID, __LINE__)

#define xMEMPOPTEMP(pointer) iMemPopTemp((pointer))

#if DEBUG || RELEASE
void DebugGetHeapUsage(U32* pTotal, U32* pUsed, U32* pOverhead); // UNUSED
#endif
void xMemDebug_SoakLog(U32 SceneId);
U32 xMemFreeMemTotal(); // UNUSED
#if DEBUG
void xMem_DB_DispStatSum();
U32 xMem_DB_CurMemUsed(U32* total, U32* overhead); // UNUSED
#endif
void xMemInit();
void xMemExit();
void xMemInitHeap(xMemHeap* heap, U32 base, U32 size, U32 flags);

#if DEBUG || RELEASE
void xMemMgrSetDeptMaxMem(U32 deptID, U32 deptMaxMem);
void xMemMgrUsageTrackerInit();
void xMemMgrUsageTrackerShutdown();
void xMemMgrUsageTrackerCB();
void xMemMgrUsageTrackerSetDumpFile(char* filename);
void xMemMgrUsageTrackerDumpLog(char* fmt, ...);
void xMemMgrUsageTrackerCloseLog();
void xMemMgrPackerStaticTally(U32, U32);
void xMemMgrPackerStaticRemove(U32, U32);
void xMemMgrPackerMallocTally(U32 size, U32 tag);
void xMemMgrPackerMallocRemove(U32 size, U32 tag);
void xMemMgrPackerApproxRWMallocTally(U32 size, U32 tag);
void xMemMgrPackerApproxRWMallocRemove(U32 size, U32 tag);
void xMemMgrGenericMallocTally(U32 size, U32 tag);
void xMemMgrGenericMallocRemove(U32 size, U32 tag);
void xMemMgrGenericMallocPushTag(U32 tag);
U32 xMemMgrGenericMallocGetTag();
void xMemMgrGenericMallocPopTag();
bool xMemMgrGenericMallocVerifyClearTag();
U32 xMemMgrGetTotalMemUsageByTag(U32 tag);
#endif

void xMemSwitchHeaps(U32 type); // UNUSED
#if DEBUG
void xMemTest(); // UNUSED
U32 xMemCheck();
#endif

S32 xMemPushBase(U32 heapID);
S32 xMemPushBase();
void* xMemAllocFreeable(void* alloced_memory, S32 asked_for_size, xMemFreeFunc func, void* param);
void xMemFreeDefault(void* addr_of_ptr); // UNUSED
S32 xMemPopBase(U32 heapID, S32 depth);
S32 xMemPopBase(S32 depth);
S32 xMemGetBase(U32 heapID);
void xMemRegisterBaseNotifyFunc(xMemBaseNotifyFunc func);
S32 xMemGetBase();
S32 xMemIsBaseValid(S32 level); // UNUSED
void xMemPoolSetup(xMemPool* pool, void* buffer, U32 nextOffset, U32 flags, xMemPoolInitCallback initCB, U32 size, U32 count, U32 numRealloc);
void* xMemPoolAlloc(xMemPool* pool, U32 memtag);
void xMemPoolFree(xMemPool* pool, void* data);
void* xMemScratchPush(U32 size, U32 align);
void xMemScratchPop(void* mem);

#if DEBUG || RELEASE
void* xMemGrowAlloc(U32 heapID, U32 size, U32 tag);
#else
void* xMemGrowAlloc(U32 heapID, U32 size);
#endif

#if DEBUG
void* xMemAlloc(U32 heapID, U32 size, S32 align, U32 tag, U32 assetID, const char* file, const char* func, S32 line);
#elif RELEASE
void* xMemAlloc(U32 heapID, U32 size, S32 align, U32 tag);
#else
void* xMemAlloc(U32 heapID, U32 size, S32 align);
#endif

#if DEBUG
inline void* xMemPushTemp(U32 amt, U32, const char* file, const char* func, S32 line)
{
    void* ptr = iMemPushTemp(amt);
    xASSERT_FMT_L(391, ptr, "for %d bytes from %s, in %s line %d", amt, func, file, line);
    return ptr;
}
#else
inline void* xMemPushTemp(U32 amt)
{
    void* ptr = iMemPushTemp(amt);
    xASSERT_L(425, ptr);
    return ptr;
}
#endif

inline void* operator new(size_t size, xMemStaticType, U32 tag, U32 assetID = 0)
{
    return xMEMALLOC_L(size, 0, tag, assetID, 650);
}

inline void* operator new[](size_t size, xMemStaticType, U32 tag, U32 assetID = 0)
{
    return xMEMALLOC_L(size, 0, tag, assetID, 655);
}

#if DEBUG || RELEASE
// Not in DWARF data
enum xMemTweakType
{
    eMemTweakType_0, // unknown
};

void* xDebugTweak_StaticAlloc(U32 size);

inline void* operator new(size_t size, xMemTweakType, U32)
{
    return xDebugTweak_StaticAlloc(size);
}

inline void* operator new[](size_t size, xMemTweakType, U32)
{
    return xDebugTweak_StaticAlloc(size);
}
#endif

#endif
