#ifndef ZCUTSCENEMGR_H
#define ZCUTSCENEMGR_H

#include "xModelPipe.h"
#include "xMath3.h"

#include <rpworld.h>

struct zCutsceneHack
{
    char* cinname;
    char* modelname;
    U32 flags;
    U32 numPipes;
    RpAtomicCallBackRender renderCB;
    xModelPipe pipe[8];
    xVec3 scale;
    void* dataPtr;
    U32 numAtomics;
    void* morph_dataPtr[8];
    void* atomic_dataPtr[8];
    U32 atomic_Idx[8];
    U32 cinnameID;
    U32 modelnameID;
};

void zCutsceneMgr_Started();
S32 zCutsceneMgrGetHackTableSize();
zCutsceneHack& zCutsceneMgrGetHackTableDataByIdx(S32 idx);

#endif
