#ifndef ZNPCONELINER_H
#define ZNPCONELINER_H

#include "xSnd.h"

namespace zNPC {
class zNPCCommon;
}

enum ezNPCOneLinerType
{
    ezOLT_Noticing,
    ezOLT_Attacking,
    ezOLT_Retreating,
    ezOLT_TakingDamage,
    ezOLT_PickedUp,
    ezOLT_Frozen,
    ezOLT_Defeated,
    ezOLT_Blocking,
    ezOLT_External,
    ezOLT_Total
};

enum ezNPCOneLinerPriority
{
    ezOLP_Lowest,
    ezOLP_Low,
    ezOLP_Medium,
    ezOLP_High,
    ezOLP_Highest,
    ezOLP_Total
};

void zNPCOneLinerSystemPreUpdate(F32 dt);
void zNPCOneLinerSystemPostUpdate(F32 dt);
void zNPCOneLinerPlayInternal(zNPC::zNPCCommon* inSource, ezNPCOneLinerType inType, ezNPCOneLinerPriority inPriority);
void zNPCOneLinerPlayExternal(zNPC::zNPCCommon* inSource, xSndGroupHandle inSoundGroup, F32 inTime);
void zNPCOneLinerSceneReset();

#endif
