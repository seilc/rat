#ifndef ZCOMPLOGICSTIRRINGPLAYER_H
#define ZCOMPLOGICSTIRRINGPLAYER_H

#include "zICompLogic.h"

class zCompLogicStirringPlayer : public zICompLogic
{
    U32 whichPot;
    U32 targetPot;
    F32 stirringAccum;
    F32 potTemp[4];
    F32 potPos[4];
    F32 potRate[4];
    bool youLose;
    F32 maxMoveSpeed;
};

#endif
