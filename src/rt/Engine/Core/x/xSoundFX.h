#ifndef XSOUNDFX_H
#define XSOUNDFX_H

#include "xBase.h"
#include "xSoundFXAsset.h"
#include "xSndMgr.h"

struct xSoundFX : xBase
{
    xSoundFXAsset* asset;
    xSndHandle sndHandle;
};

void xSoundFXInit(void* t, void* asset);
void xSoundFXInit(xSoundFX* t, xSoundFXAsset* asset);
void xSoundFXSave(xSoundFX* ent, xSerial* s); // UNUSED
void xSoundFXLoad(xSoundFX* ent, xSerial* s); // UNUSED
void xSoundFXReset(xSoundFX* ent);
void xSoundFXUpdate(xSoundFX* pSoundFX); // UNUSED
void xSoundFXEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32);
xSoundFX* xSoundFXGet(U32 soundfxID);

#endif
