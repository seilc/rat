#include "xCommon.h"
#include "zPortal.h"

#include "containers.h"
#include "xCam.h"
#include "xFont.h"
#include "xParticleBatch.h"
#include "zBase.h"
#include "zEconomics.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

void zPortalInit(void* portal, void* passet)
{
    zPortalInit((zPortal*)portal, (xPortalAsset*)passet);
}

void zPortalInit(zPortal* portal, xPortalAsset* passet)
{
    xBaseInit(portal, passet);

    xASSERT_L(42, portal->baseType == eBaseTypePortal);

    portal->passet = passet;
    portal->eventFunc = zPortalEventCB;

    if (portal->linkCount)
        portal->link = (xLinkAsset*)(portal->passet + 1);
}

void zPortalReset(zPortal* ent)
{
    xASSERT_L(53, ent);
    xASSERT_L(54, ent->passet);

    xBaseReset(ent, ent->passet);
}

void zPortalSave(zPortal* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(75, ent);
    xASSERT_L(76, s);
}

void zPortalLoad(zPortal* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(93, ent);
    xASSERT_L(94, s);
}

void zPortalEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    xASSERT_L(113, to);

    switch (toEvent) {
    case eEventDebugReset:
    case eEventReset:
        zPortalReset((zPortal*)to);
        break;
    case eEventTeleportPlayer:
        zSceneSwitch((zPortal*)to, toParam[0] != 0.0f, toParam[1] != 0.0f, 0);

        if (toParam[1] == 0.0f) {
            globals.economics->GiveLevelCompletionBonus();
        }
        break;
    }
}
