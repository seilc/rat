#ifndef XPARGROUP_H
#define XPARGROUP_H

#include "xPar.h"

struct xParCmdTex;

struct xParGroup
{
    xPar* m_root;
    xPar* m_dead;
    S32 m_num_of_particles;
    U8 m_alive;
    U8 m_killWhenDead;
    U8 m_active;
    U8 m_visible;
    U8 m_culled;
    U8 m_priority;
    U8 m_flags;
    U8 m_regidx;
    xParGroup* m_next;
    xParGroup* m_prev;
    void(*draw)(void*, xParGroup*);
    xParCmdTex* m_cmdTex;
};

void xParGroupInit(xParGroup* ps);
void xParGroupSetAging(xParGroup* ps, S32 age);
void xParGroupSetBack2Life(xParGroup* ps, S32 b2l);
void xParGroupSetVisibility(xParGroup* ps, S32 vis);
void xParGroupSetPriority(xParGroup* ps, U8 val);
void xParGroupRegister(xParGroup* ps);
void xParGroupUnregister(xParGroup* ps);
void xParGroupSetActive(xParGroup* ps, U32 isActive);
void xParGroupKillAllParticles(xParGroup* ps);
void xParGroupAnimate(xParGroup* ps, F32 dt);
void xParGroupSetSortDistance(F32 sortDistance, U32 sortID);
void xParGroupAddParP(xParGroup* ps, xPar* p);
xPar* xParGroupAddPar(xParGroup* ps);
void xParGroupKillPar(xParGroup* ps, xPar* p);
void xParGroupAddParToDeadList(xParGroup* ps, xPar* p);

#endif
