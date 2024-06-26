#ifndef XGRIDBOUND_H
#define XGRIDBOUND_H

#include "types.h"

struct xGrid;

struct xGridBound
{
    void* data;
    U16 gx;
    U16 gz;
    U8 oversize;
    U8 deleted;
    U8 gpad;
    U8 pad;
    xGrid* grid;
    xGridBound** head;
    xGridBound* next;
};

#endif
