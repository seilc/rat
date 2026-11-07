#ifndef XANIM_H
#define XANIM_H

#include "xMath3.h"
#include "xSndMgr.h"
#include "xMemMgr.h"

struct xModelInstance;

struct xAnimPhysicsData;
struct xAnimFile;
struct xAnimMultiFileBase;
struct xAnimMultiFileEntry;
struct xAnimMultiFile;
struct xAnimEffect;
struct xAnimActiveEffect;
struct xAnimState;
struct xAnimTransition;
struct xAnimTransitionList;
struct xAnimTable;
struct xAnimSingle;
struct xAnimPlay;

struct xAnimPhysicsData
{
    xVec3* tranTable;
    F32* yawTable;
    S32 tranCount;
};

struct xAnimFile
{
    xAnimFile* Next;
    const char* Name;
    U32 ID;
    U32 FileFlags;
    F32 Duration;
    F32 TimeOffset;
    U16 BoneCount;
    U8 NumAnims[2];
    void* RawData;
    xAnimPhysicsData* PhysicsData;
};

struct xAnimMultiFileBase
{
    U32 Count;
};

struct xAnimMultiFileEntry
{
    U32 ID;
    xAnimFile* File;
};

struct xAnimMultiFile : xAnimMultiFileBase
{
    xAnimMultiFileEntry Files[1];
};

typedef U32(*xAnimEffectCallback)(U32, xAnimActiveEffect*, xAnimSingle*, void*);

struct xAnimEffect
{
    xAnimEffect* Next;
    U16 Flags;
    U16 Probability;
    F32 StartTime;
    F32 EndTime;
    xAnimEffectCallback Callback;
};

struct xAnimActiveEffect
{
    xAnimEffect* Effect;
    union
    {
        U32 Handle;
        xSndHandle SndHandle;
    };
};

typedef void(*xAnimBeforeEnterCallback)(xAnimPlay*, xAnimState*, void*);
typedef void(*xAnimStateCallback)(xAnimState*, xAnimSingle*, void*);
typedef void(*xAnimBeforeAnimMatricesCallback)(xAnimPlay*, xQuat*, xVec3*, S32);

struct xAnimState
{
    xAnimState* Next;
    const char* Name;
    U32 ID;
    U32 Flags;
    U32 UserFlags;
    F32 Speed;
    xAnimFile* Data;
    xAnimEffect* Effects;
    xAnimTransitionList* Default;
    xAnimTransitionList* List;
    F32* BoneBlend;
    F32* TimeSnap;
    F32 FadeRecip;
    U16* FadeOffset;
    void* CallbackData;
    xAnimMultiFile* MultiFile;
    xAnimBeforeEnterCallback BeforeEnter;
    xAnimStateCallback StateCallback;
    xAnimBeforeAnimMatricesCallback BeforeAnimMatrices;
};

typedef U32(*xAnimTransitionCallback)(xAnimTransition*, xAnimSingle*, void*);

struct xAnimTransition
{
    xAnimTransition* Next;
    xAnimState* Dest;
    xAnimTransitionCallback Conditional;
    xAnimTransitionCallback Callback;
    U32 Flags;
    U32 UserFlags;
    F32 SrcTime;
    F32 DestTime;
    U16 Priority;
    U16 QueuePriority;
    F32 BlendRecip;
    U16* BlendOffset;
};

struct xAnimTransitionList
{
    xAnimTransitionList* Next;
    xAnimTransition* T;
};

struct xAnimTable
{
    const char* Name;
    xAnimTransition* TransitionList;
    xAnimState* StateList;
    U32 AnimIndex;
    U32 MorphIndex;
    U32 UserFlags;
};

struct xAnimSingle
{
    U32 SingleFlags;
    xAnimState* State;
    F32 Time;
    F32 CurrentSpeed;
    F32 BilinearLerp[2];
    xAnimEffect* Effect;
    U32 ActiveCount;
    F32 LastTime;
    xAnimActiveEffect* ActiveList;
    xAnimPlay* Play;
    xAnimTransition* Sync;
    xAnimTransition* Tran;
    xAnimSingle* Blend;
    F32 BlendFactor;
    xVec3 PhysDisp;
    F32 YawDisp;
    U32 pad[1];
};

struct xAnimPlay
{
    xAnimPlay* Next;
    U16 NumSingle;
    U16 BoneCount;
    xAnimSingle* Single;
    void* Object;
    xAnimTable* Table;
    xMemPool* Pool;
    xModelInstance* ModelInst;
    xAnimBeforeAnimMatricesCallback BeforeAnimMatrices;
};

void xAnimInit();
xAnimTable* xAnimTableNew(const char* name, U32 userFlags);
xAnimState* xAnimTableNewState(xAnimTable* table, const char* name, U32 flags, U32 userFlags, F32 speed, F32* boneBlend, F32* timeSnap, F32 fadeRecip, U16* fadeOffset, void* callbackData, xAnimBeforeEnterCallback beforeEnter, xAnimStateCallback stateCallback, xAnimBeforeAnimMatricesCallback beforeAnimMatrices);
void xAnimPlayChooseTransition(xAnimPlay* play);
void xAnimPlayUpdate(xAnimPlay* play, F32 timeDelta);
void xAnimPlayEval(xAnimPlay* play);
void xAnimPoolInit(xMemPool* pool, U32 count, U32 singles, U32 blendFlags, U32 effectMax);
void xAnimPoolFree(xAnimPlay* play);

inline F32 xAnimFileRawTime(xAnimFile* afile, F32 time)
{
    if ((afile->FileFlags & 0x1000) || ((afile->FileFlags & 0x2000) && time > 0.5f * afile->Duration))
        return afile->TimeOffset + afile->Duration - time;
    return afile->TimeOffset + time;
}

#endif
