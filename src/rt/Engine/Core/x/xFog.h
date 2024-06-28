#ifndef XFOG_H
#define XFOG_H

#include "xBase.h"
#include "xFogAsset.h"
#include "iFog.h"

struct xScene;

typedef struct _xFog : xBase
{
    xFogAsset* tasset;
} xFog;

void xFogClearFog();
void xFogInit(void* b, void* tasset);
void xFogInit(xBase* b, xFogAsset* tasset);
void xFogReset(xFog* t);
void xFogSave(xFog* ent, xSerial* s);
void xFogLoad(xFog* ent, xSerial* s);
void xFogEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
void xFogUpdate(xBase* to, xScene* sc, F32);

#endif
