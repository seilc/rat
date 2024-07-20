#ifndef XSCENE_H
#define XSCENE_H

#include "xEnt.h"
#include "xEnv.h"

typedef xBase*(*xSceneResolvIDCallback)(U32);
typedef char*(*xSceneBase2NameCallback)(xBase*);
typedef char*(*xSceneID2NameCallback)(U32);

struct xScene
{
    U32 sceneID;
    U16 flags;
    U16 numTrigs;
    U16 numSpecialTriggers;
    U16 numStats;
    U16 numDyns;
    U16 numNpcs;
    U16 numActEnts;
    F32 gravity;
    F32 drag;
    F32 friction;
    U16 numEntsAllocd;
    U16 numTrigsAllocd;
    U16 numSpecialTriggersAllocd;
    U16 numStatsAllocd;
    U16 numDynsAllocd;
    U16 numNpcsAllocd;
    xEnt** trigs;
    xEnt** specialTriggers;
    xEnt** stats;
    xEnt** dyns;
    xEnt** npcs;
    xEnt** actEnts;
    xEnv* env;
    xMemPool mempool;
    xSceneResolvIDCallback resolvID;
    xSceneBase2NameCallback base2Name;
    xSceneID2NameCallback id2Name;
};

xBase* xSceneResolvID(xScene* sc, U32 id);

#endif
