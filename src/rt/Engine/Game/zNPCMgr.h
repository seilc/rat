#ifndef ZNPCMGR_H
#define ZNPCMGR_H

#include "xVec3.h"

struct st_XORDEREDARRAY;
struct xBase;
struct xDynAsset;
struct xEntAsset;

namespace zNPC {
class zNPCCommon;
}

struct zSurroundedBy
{
    zNPC::zNPCCommon* npc;
    xVec3 moveToPos;
};

void zNPCMgr_switchedNavmeshes();
st_XORDEREDARRAY* zNPCMgr_GetNPCList();
xBase* zNPCMgr_createDynNPCInst(const xEntAsset* assdat, const xDynAsset* dynass, S32 siz_dynass);

extern zSurroundedBy gSurroundedBy[32];
extern S32 gSurroundedByTotal;

#endif
