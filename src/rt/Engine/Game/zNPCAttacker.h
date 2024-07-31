#ifndef ZNPCATTACKER_H
#define ZNPCATTACKER_H

#include "types.h"

namespace zNPC {
class zNPCCommon;
}

struct zNPCAttackerData
{
    zNPC::zNPCCommon* npcAttacker[128];
    S32 npcAttackerTotal;
    S32 npcAttackerTotalLastFrame;
};

#endif
