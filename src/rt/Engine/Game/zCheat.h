#ifndef ZCHEAT_H
#define ZCHEAT_H

#include "types.h"

struct zCheats
{
    bool halfDamage : 1;
    bool incredipower : 1;
    bool chronos : 1;
    bool noFriction : 1;
    bool scaleHead : 1;
    bool firePlants : 1;
    bool discoMode : 1;
    bool flamingFeet : 1;
    bool blurry : 1;
    bool ethereal : 1;
    bool glowingSwimmer : 1;
    bool bigMouth : 1;
    bool bigCritters : 1;
    bool makeHarder : 1;
    bool makeEasier : 1;
};

enum zCheatStatus
{
    zCheatStatus_Bad,
    zCheatStatus_Ok,
    zCheatStatus_CreditCode,
    zCheatStatus_HILogo
};

extern zCheats gCheats;

void zCheatInit();
void zCheatRemoveAll();
void zCheatReset();
void zCheatUpdate(F32 dt);
F32 zCheatGetHeadScale();
zCheatStatus zCheatProcess(const char* name);

#endif
