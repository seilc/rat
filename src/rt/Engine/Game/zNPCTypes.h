#ifndef ZNPCTYPES_H
#define ZNPCTYPES_H

#include "types.h"

enum eNPCType
{
    eNPCType_None = -1,
    eNPCType_Unknown = -1,
    eNPCType_First = 0,
    eNPCType_SwarmMember = 0,
    eNPCType_Swarm_Owl,
    eNPCType_Swarm_Bug,
    eNPCType_Thief,
    eNPCType_Simpleton,
    eNPCType_Alarmer,
    eNPCType_Waiter,
    eNPCType_CMG_StirringPlayer,
    eNPCType_CMG_StirringRemy,
    eNPCType_CMG_LeftArm,
    eNPCType_CMG_RightArm,
    eNPCType_CMG_PourNSwirl,
    eNPCType_SpringBoard,
    eNPCType_ContextSensitive,
    eNPCType_AnimViewer,
    eNPCType_InstaKill,
    eNPCTypeCount,
    eNPCTypeForceInt = FORCEENUMSIZEINT
};

#endif
