#ifndef XMOVEPOINT_H
#define XMOVEPOINT_H

#include "xBase.h"
#include "xDebug.h"
#include "xMovePointAsset.h"

struct xSpline3;

struct xMovePoint : xBase
{
    xMovePointAsset* asset;
    xVec3* pos;
    xMovePoint** nodes;
    xMovePoint* prev;
    U32 node_wt_sum;
    U8 on;
    U8 pad[3];
#if DEBUG || RELEASE
    U32 start_point_of_id;
#endif
    F32 delay;
    xSpline3* spl;
};

struct xScene;

void xMovePointInit(xMovePoint* m, xMovePointAsset* asset);
void xMovePointSave(xMovePoint* ent, xSerial* s);
void xMovePointLoad(xMovePoint* ent, xSerial* s);
void xMovePointReset(xMovePoint* m);
void xMovePointSetup(xMovePoint* m, xScene* sc);
void xMovePointSplineSetup(xMovePoint* m);
void xMovePointSplineDestroy(xMovePoint* m);
xSpline3* xMovePointSplineFind(xMovePoint* m, bool search_back);
F32 xMovePointGetNext(const xMovePoint* m, const xMovePoint* prev, xMovePoint** next, xVec3* hdng);
xVec3* xMovePointGetPos(const xMovePoint* m);
S32 xMovePointGetAllLinked(xMovePoint** mps, S32 mps_max, xMovePoint* mp, S32 mps_used);

inline xMovePoint* xMovePointGetPoint(const xMovePoint* m, U16 n)
{
    xASSERT_L(114, n < m->asset->numPoints);
    return m->nodes[n];
}

#endif
