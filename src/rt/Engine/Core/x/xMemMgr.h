#ifndef XMEMMGR_H
#define XMEMMGR_H

#include "types.h"

#include <stddef.h>

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
#ifdef DEBUG
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

extern U32 gActiveHeap;

#if defined(DEBUG) || defined(RELEASE)
#define xMEMGROWALLOC(size, tag) xMemGrowAlloc(gActiveHeap, (size), (tag))
#else
#define xMEMGROWALLOC(size, tag) xMemGrowAlloc(gActiveHeap, (size))
#endif

#if defined(DEBUG)
#define xMEMALLOC(size, align, tag, assetID, line) xMemAlloc(gActiveHeap, (size), (align), (tag), (assetID), __FILE__, __FUNCTION__, (line))
#elif defined(RELEASE)
#define xMEMALLOC(size, align, tag, assetID, line) xMemAlloc(gActiveHeap, (size), (align), (tag))
#else
#define xMEMALLOC(size, align, tag, assetID, line) xMemAlloc(gActiveHeap, (size), (align))
#endif

#if defined(DEBUG) || defined(RELEASE)
void* xMemGrowAlloc(U32 heapID, U32 size, U32 tag);
#else
void* xMemGrowAlloc(U32 heapID, U32 size);
#endif

#if defined(DEBUG)
void* xMemAlloc(U32 heapID, U32 size, S32 align, U32 tag, U32 assetID, const char* file, const char* func, S32 line);
#elif defined(RELEASE)
void* xMemAlloc(U32 heapID, U32 size, S32 align, U32 tag);
#else
void* xMemAlloc(U32 heapID, U32 size, S32 align);
#endif

void* operator new(size_t size, xMemStaticType, U32 tag, U32 assetID = 0);

#endif
