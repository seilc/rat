#ifndef ZNPCMGR_H
#define ZNPCMGR_H

#include "xVec3.h"

namespace zNPC {
class zNPCCommon;
}

struct zSurroundedBy
{
    zNPC::zNPCCommon* npc;
    xVec3 moveToPos;
};

void zNPCMgr_switchedNavmeshes();

extern zSurroundedBy gSurroundedBy[32];
extern S32 gSurroundedByTotal;

#endif
