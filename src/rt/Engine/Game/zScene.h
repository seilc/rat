#ifndef ZSCENE_H
#define ZSCENE_H

#include "xScene.h"
#include "zBase.h"
#include "zPortal.h"
#include "zEnv.h"
#include "xDynAsset.h"

struct zSceneParameters : xDynAsset
{
    U32 idle03ExtraCount;
    xAnimFile** idle03Extras;
    U32 idle04ExtraCount;
    xAnimFile** idle04Extras;
    U8 bombCount;
    U8 extraIdleDelay;
    U8 hdrGlow;
    U8 hdrDarken;
    U32 uDefaultMusicHash;
    U32 flags;
    F32 waterTileWidth;
    F32 lodFadeDistance;
    F32 waterTileOffsetX;
    F32 waterTileOffsetZ;
    U8 numCheckpoints;
    U8 pad1[3];
    F32 grassDistFade;
    F32 grassDistCull;
    U32 piggybank;
    U32 maxAnimationMem;
    U32 maxArtMem;
    U32 maxDesignMem;
    U32 maxProgrammingMem;
    U32 pad[3];
};

struct zScene : xScene
{
    zPortal* pendingPortal;
    S32 num_base;
    xBase** base;
    U32 num_update_base;
    xBase** update_base;
    S32 baseCount[eBaseTypeCount];
    xBase* baseList[eBaseTypeCount];
    zEnv* zen;
    zSceneParameters* parameters;
    bool enableDrawing;
};

xBase* zSceneFindObject(U32 gameID);

#endif
