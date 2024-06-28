#include "xFog.h"

#include "iCamera.h"
#include "zBase.h"
#include "zEvent.h"

void xFogClearFog()
{
    iCameraSetFogParams(NULL, 0.0f);
}

void xFogInit(void* b, void* tasset)
{
    xFogInit((xBase*)b, (xFogAsset*)tasset);
}

void xFogInit(xBase* b, xFogAsset* tasset)
{
    xBaseInit(b, tasset);

    xFog* t = (xFog*)b;

    t->eventFunc = xFogEventCB;
    t->tasset = tasset;

    if (t->linkCount) {
        t->link = (xLinkAsset*)((U8*)t->tasset + sizeof(xFogAsset));
    } else {
        t->link = NULL;
    }
}

void xFogReset(xFog* t)
{
    xASSERT(61, t);
    xASSERT(62, t->tasset);

    xBaseReset(t, t->tasset);
}

void xFogSave(xFog* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT(83, ent);
    xASSERT(84, s);
}

void xFogLoad(xFog* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT(102, ent);
    xASSERT(103, s);
}

void xFogEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xFog* t = (xFog*)to;

    xASSERT(123, to);
    xASSERT(124, to->baseType == eBaseTypeFog);

    switch (toEvent) {
    case eEventOn:
    {
        iFogParams fog;
        fog.type = rwFOGTYPELINEAR;
        fog.start = t->tasset->fogStart;
        fog.stop = t->tasset->fogStop;
        fog.density = t->tasset->fogDensity;
        fog.fogcolor.red = t->tasset->fogColor[0];
        fog.fogcolor.green = t->tasset->fogColor[1];
        fog.fogcolor.blue = t->tasset->fogColor[2];
        fog.fogcolor.alpha = t->tasset->fogColor[3];
        fog.bgcolor.red = t->tasset->bkgndColor[0];
        fog.bgcolor.green = t->tasset->bkgndColor[1];
        fog.bgcolor.blue = t->tasset->bkgndColor[2];
        fog.bgcolor.alpha = t->tasset->bkgndColor[3];
        fog.table = NULL;
        iCameraSetFogParams(&fog, t->tasset->transitionTime);
        break;
    }
    case eEventOff:
        iCameraSetFogParams(NULL, 0.0f);
        break;
    case eEventReset:
    case eEventDebugReset:
        xFogReset(t);
        break;
    }
}

void xFogUpdate(xBase* to, xScene* sc, F32)
{
    xASSERT(172, to);
    xASSERT(173, sc);

#ifdef DEBUG
    xFog* t = (xFog*)to;
    xASSERT(178, t->baseType == eBaseTypeFog);
#endif
}
