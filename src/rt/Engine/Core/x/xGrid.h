#ifndef XGRID_H
#define XGRID_H

#include "xGridBound.h"
#include "xMath3.h"

struct xEnt;
struct xQCData;

struct xGrid
{
    U8 ingrid_id;
    U8 pad[3];
    U16 nx;
    U16 nz;
    F32 minx;
    F32 minz;
    F32 maxx;
    F32 maxz;
    F32 csizex;
    F32 csizez;
    F32 inv_csizex;
    F32 inv_csizez;
    F32 maxr;
    xGridBound** cells;
    xGridBound* other;
    S32 iter_active;
};

typedef S32(*xGridCallback)(xEnt* ent, void* cbdata);

void xGridCheckPosition(xGrid* grid, xVec3* pos, xQCData* qcd, xGridCallback hitCB, void* cbdata);

#endif
