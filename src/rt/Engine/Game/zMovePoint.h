#ifndef ZMOVEPOINT_H
#define ZMOVEPOINT_H

#include "xMovePoint.h"

struct zScene;

struct zMovePoint : xMovePoint
{
#if DEBUG || RELEASE
    S32 dbdraw_inprog;
#endif
};

typedef S32 (*zMovePointLOSCallback)(void*, xVec3*, const zMovePoint*);

zMovePoint* zMovePoint_GetMemPool(S32 cnt, U32 memtag);
void zMovePointInit(zMovePoint* m, xMovePointAsset* asset);
S32 zMovePoint_GetCount(); // UNUSED
zMovePoint* zMovePoint_GetInst(S32 n);
void zMovePointSetup(zMovePoint* mvpt, zScene* scn);
zMovePoint* zMovePoint_From_xAssetID(U32 aid); // UNUSED
void zMovePointSave(zMovePoint* ent, xSerial* s);
void zMovePointLoad(zMovePoint* ent, xSerial* s);
void zMovePointReset(zMovePoint* ent);
void zMovePointEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                       U32 toParamWidgetID);
F32 zMovePointGetNext(const zMovePoint* current, const zMovePoint* prev, zMovePoint** next, xVec3* hdng); // UNUSED
xVec3* zMovePointGetPos(const zMovePoint* m);
U16 zMovePointGetNumPoints(const zMovePoint* m); // UNUSED
zMovePoint* zMovePointGetPoint(const zMovePoint* m, U16 n); // UNUSED
F32 zMovePointZoneRadius(const zMovePoint* m); // UNUSED
F32 zMovePointGetDelay(const zMovePoint* m); // UNUSED
S32 zMovePointIsOn(const zMovePoint* m); // UNUSED
S32 zMovePointIsJumper(const zMovePoint* a, const zMovePoint* b); // UNUSED
zMovePoint* zMovePointFindNearest(S32 flg_opts, xVec3* yourpos); // UNUSED
zMovePoint* zMovePointFindNearest(S32 flg_opts, xVec3* yourpos, void* yourdata, zMovePointLOSCallback losfunc); // UNUSED
zMovePoint* zMovePointFindNearest(S32 flg_opts, xVec3* yourpos, F32 okdist, F32 nohope, void* yourdata,
                                  zMovePointLOSCallback losfunc); // UNUSED
zMovePoint* zMovePointFindNearest(S32 flg_opts, xVec3* yourpos, F32 okdist, F32 nohope, void* yourdata,
                                  zMovePointLOSCallback losfunc, S32* idx_resume, zMovePoint** prev_best); // UNUSED
#if DEBUG || RELEASE
void zMovePoint_db_draw(const zMovePoint* from, const zMovePoint* to, S32); // UNUSED
void zMovePoint_db_drawNetwork(const zMovePoint* m); // UNUSED
void zMovePoint_db_drawNetwork(const zMovePoint* m, S32 recurs); // UNUSED
void zMovePoint_db_drawNetwork(const zMovePoint* m, S32 recurs, S32 level); // UNUSED
#endif

#endif
