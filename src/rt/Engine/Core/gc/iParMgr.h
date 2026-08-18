#ifndef IPARMGR_H
#define IPARMGR_H

#include "types.h"

struct xParGroup;

void iParMgrInit();
void iParMgrUpdate(F32);
void iParMgrRenderParSys_Sprite(void* data, xParGroup* ps);
void iParMgrRenderParSys_Streak(void* data, xParGroup* ps);
void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps);
void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps);
void iParMgrRenderParSys_Static(void* data, xParGroup*);
void iParMgrRenderParSys_Ground(void* data, xParGroup* ps);
void iParMgrRenderParSys_Flat(void* data, xParGroup* ps);

#endif
