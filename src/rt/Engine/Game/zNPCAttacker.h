#ifndef ZNPCATTACKER_H
#define ZNPCATTACKER_H

#include "types.h"

namespace zNPC {
class zNPCCommon;
}

enum ezNPCAttackerType
{
    ezNPCAT_Melee,
    ezNPCAT_Range,
    ezNPCAT_All,
    ezNPCAT_Total
};

struct zNPCAttackerData
{
    zNPC::zNPCCommon* npcAttacker[128];
    S32 npcAttackerTotal;
    S32 npcAttackerTotalLastFrame;
};

#endif
