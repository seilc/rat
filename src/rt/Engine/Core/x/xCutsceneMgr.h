#ifndef XCUTSCENEMGR_H
#define XCUTSCENEMGR_H

#include "xBase.h"
#include "xCutscene.h"
#include "xCutsceneMgrAsset.h"

struct xCutsceneMgr : xBase
{
    xCutsceneMgrAsset* tasset;
    xCutscene* csn;
    U32 stop;
    F32 oldfov;
};

#endif
