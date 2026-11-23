#include "xCommon.h"
#include "zMovePoint.h"

#include "containers.h"
#include "xFont.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

#include "decomp.h"

static zMovePoint* g_mvpt_list;
static S32 g_mvpt_cnt;

zMovePoint* zMovePoint_GetMemPool(S32 cnt, U32 memtag)
{
    if (!cnt) g_mvpt_list = NULL;
    else
        g_mvpt_list = (zMovePoint*)xMEMALLOC_L(cnt * sizeof(zMovePoint), 0, memtag, 0, 63);

    g_mvpt_cnt = cnt;
    return g_mvpt_list;
}

void zMovePointInit(zMovePoint* m, xMovePointAsset* asset)
{
    xASSERT_L(72, m);

    xMovePointInit(m, asset);

    m->eventFunc = zMovePointEventCB;

    if (m->linkCount) {
        m->link = (xLinkAsset*)((U8*)asset + sizeof(xMovePointAsset) + asset->numPoints * sizeof(U32));
    } else {
        m->link = NULL;
    }

#if DEBUG
    m->dbdraw_inprog = 0;
#endif
}

zMovePoint* zMovePoint_GetInst(S32 n)
{
    xVALIDATE_L(101, n<g_mvpt_cnt);
    xASSERT_L(102, g_mvpt_list);
    return &g_mvpt_list[n];
}

void zMovePointSetup(zMovePoint* mvpt, zScene* scn)
{
    xMovePointSetup(mvpt, scn);
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("MOVP", "zMovePoint Find Failed for Asset ID %d\n");
#endif

void zMovePointSave(zMovePoint* ent, xSerial* s)
{
    xMovePointSave(ent, s);

    xASSERT_L(150, ent);
    xASSERT_L(151, s);
}

void zMovePointLoad(zMovePoint* ent, xSerial* s)
{
    xMovePointLoad(ent, s);

    xASSERT_L(169, ent);
    xASSERT_L(170, s);
}

void zMovePointReset(zMovePoint* ent)
{
    xMovePointReset(ent);
}

void zMovePointEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    zMovePoint* m = (zMovePoint*)to;

    xASSERT_L(197, m);
    switch (toEvent) {
    case eEventArrive:
        break;
    case eEventOn:
        m->on = 1;
        break;
    case eEventOff:
        m->on = 0;
        break;
    case eEventDebugReset:
    case eEventReset:
        zMovePointReset(m);
        break;
    }
}

xVec3* zMovePointGetPos(const zMovePoint* m)
{
    return xMovePointGetPos(m);
}

// UNUSED: needed to match (speculative code)
zMovePoint* zMovePointGetPoint(const zMovePoint* m, U16 n)
{
    return (zMovePoint*)xMovePointGetPoint(m, n);
}

DECOMP_FORCEACTIVE(
    DEBUG ? "%s(%d) : (m) in '%s'\n" : 0,
    DEBUG ? "%s(%d) : (m->asset) in '%s'\n" : 0,
    DEBUG ? "%s(%d) : (((flg_opts & MVPT_SCAN_RESUME) && idx_resume && prev_best) || !(flg_opts & MVPT_SCAN_RESUME)) in '%s'\n" : 0,
    DEBUG ? "%s(%d) : (tmp_mvpt) in '%s'\n" : 0,
    DEBUG ? "%s(%d) : (idx_resume && prev_best) in '%s'\n" : 0,
    (DEBUG || RELEASE) ? "MVPT: Scan ... unhandled case %d\n" : 0
)

static S32 MVPT_dflt_chklos(void*, xVec3* mypos, const zMovePoint* mvpt); // UNUSED

#if DEBUG
DECOMP_FORCEACTIVE(
    "%s(%d) : (globals.sceneCur) in '%s'\n",
    "%s(%d) : (globals.sceneCur->env) in '%s'\n",
    "%s(%d) : (m->dbdraw_inprog >= 0) in '%s'\n"
)
#endif
