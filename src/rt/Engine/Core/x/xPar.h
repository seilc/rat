#ifndef XPAR_H
#define XPAR_H

#include "xVec3.h"

#define XPAR_POOL_MAX 500

struct xParEmitterAsset;

struct xPar
{
    xPar* m_next;
    xPar* m_prev;
    F32 m_lifetime;
    U8 m_c[4];
    xVec3 m_pos;
    F32 m_size;
    xVec3 m_vel;
    F32 m_sizeVel;
    U8 m_flag;
    U8 m_mode;
    U8 m_texIdx[2];
    U8 m_rotdeg[3];
    U8 pad8;
    F32 totalLifespan;
    xParEmitterAsset* m_asset;
    F32 m_cvel[4];
    F32 m_cfl[4];
    U32 m_sortID;
    F32 m_dist2Camera;
    U32 pad[2];
};

extern xPar gParPool[XPAR_POOL_MAX];
extern xPar* gParDead;

void xParMemInit();
xPar* xParAlloc();
void xParFree(xPar* par);
void xParInit(xPar* p);

#endif
