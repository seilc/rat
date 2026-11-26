#ifndef ZCAMERATWEAK_H
#define ZCAMERATWEAK_H

#include "xBase.h"
#include "zCameraTweakAsset.h"

struct zCameraTweak : xBase
{
    CameraTweak_asset* casset;
};

void zCameraTweakGlobal_Init();
void zCameraTweakGlobal_Add(U32 owner, F32 priority, F32 time, F32 pitch, F32 distMult, F32 pivotHeight, F32 fov);
void zCameraTweakGlobal_Remove(U32 owner);
void zCameraTweakGlobal_Reset();
void zCameraTweakGlobal_Update(F32 dt);
F32 zCameraTweakGlobal_GetDistAdjust(); // UNUSED
F32 zCameraTweakGlobal_GetPitch();
F32 zCameraTweakGlobal_GetPivotHeight();
F32 zCameraTweakGlobal_GetFOV();
void zCameraTweak_Init(xBase& data, xDynAsset& asset, size_t asset_size);
void zCameraTweak_Init(zCameraTweak* tweak, CameraTweak_asset* asset);
void zCameraTweak_Reset(zCameraTweak* tweak); // UNUSED
void zCameraTweak_Save(zCameraTweak* tweak, xSerial* s);
void zCameraTweak_Load(zCameraTweak* tweak, xSerial* s);
void zCameraTweak_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif
