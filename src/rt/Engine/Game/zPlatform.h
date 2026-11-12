#ifndef ZPLATFORM_H
#define ZPLATFORM_H

#include "xEntDrive.h"
#include "xEntMotion.h"
#include "xPlatformAsset.h"
#include "zDestructible.h"
#include "zEnt.h"
#include "zEntSimpleObj.h"
#include "iSnd.h"

struct xMovePoint;
struct zPlatFMRunTime;

struct zPlatform : zEnt, zEntAnimData
{
    xPlatformAsset* plat_asset;
    xEntMotion motion;
    U16 state;
    U16 plat_flags;
    F32 tmr;
    S32 ctr;
    xMovePoint* src;
    xModelInstance* am;
    xModelInstance* bm;
    S32 moving;
    xEntDrive drv;
    zPlatFMRunTime* fmrt;
    zDestructible* destructible;
    F32 elapsedTime;
    xEntOpacity opacity;
    iSndGroupHandle soundGroupTeeterCreak;
    iSndGroupHandle soundGroupTeeterEnd;
    xSndHandle soundTeeterCreak;
    xSndHandle soundTeeterEnd;
    xSndHandle sndID;
    U8 hasStoppedRotating;
    U8 hasReachedEnd;
};

#endif
