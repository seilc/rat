#ifndef XUPDATECULL_H
#define XUPDATECULL_H

#include "xGroup.h"

typedef U32(*xUpdateCullEntCallback)(void*, void*);

struct xUpdateCullEnt
{
    U16 index;
    S16 groupIndex;
    xUpdateCullEntCallback update_cull_cb;
    void* cbdata;
    xUpdateCullEnt* nextInGroup;
};

struct xUpdateCullGroup
{
    U32 active;
    U16 startIndex;
    U16 endIndex;
    xGroup* groupObject;
};

typedef void(*xUpdateCullMgrActivateCallback)(void*);
typedef void(*xUpdateCullMgrDeactivateCallback)(void*);

struct xUpdateCullMgr
{
    U32 entCount;
    U32 entActive;
    void* ent;
    xUpdateCullEnt** mgr;
    U32 mgrCount;
    U32 mgrCurr;
    xUpdateCullEnt* mgrList;
    U32 grpCount;
    xUpdateCullGroup* grpList;
    xUpdateCullMgrActivateCallback activateCB;
    xUpdateCullMgrDeactivateCallback deactivateCB;
};

#endif
