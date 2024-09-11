#include "xParGroup.h"

#include "xDebug.h"
#include "xCommon.h"

#define REG_TABLE_MAX 255

static S32 sParGroupRegTableInit = FALSE;
static S32 sParGroupRegTableCount = 0;
xParGroup* sParGroupRegTable[REG_TABLE_MAX];

static F32 sSortDistance = 0.0f;
static U32 sSortID = 0;

#define CLAMP(x,a,b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

void xParGroupInit(xParGroup* ps)
{
    xASSERT(41, ps);

    ps->m_root = NULL;
    ps->m_dead = NULL;
    ps->m_alive = FALSE;
    ps->m_num_of_particles = 0;
    ps->m_killWhenDead = FALSE;
    ps->m_flags = 0;
    ps->m_active = TRUE;
    ps->m_culled = FALSE;
    ps->m_visible = TRUE;
    ps->m_cmdTex = NULL;
    ps->m_priority = 0;
    ps->m_next = NULL;
    ps->m_prev = NULL;
    ps->m_flags |= 0x2;
    ps->m_flags |= 0x1;
}

static void xParGroupRegisterInit()
{
    for (S32 i = 0; i < REG_TABLE_MAX; i++) {
        sParGroupRegTable[i] = NULL;
    }

    sParGroupRegTableInit = TRUE;
    sParGroupRegTableCount = 0;
}

void xParGroupSetAging(xParGroup* ps, S32 age)
{
    xASSERT(76, ps);

    if (age) {
        FLAG_REMOVE(ps->m_flags, 0x4);
    } else {
        ps->m_flags |= 0x4;
    }
}

void xParGroupSetBack2Life(xParGroup* ps, S32 b2l)
{
    xASSERT(90, ps);

    if (b2l) {
        FLAG_REMOVE(ps->m_flags, 0x8);
    } else {
        ps->m_flags |= 0x8;
    }
}

void xParGroupSetVisibility(xParGroup* ps, S32 vis)
{
    xASSERT(118, ps);

    ps->m_visible = vis;
}

void xParGroupSetPriority(xParGroup* ps, U8 val)
{
    xASSERT(126, ps);

    ps->m_priority = val;
}

void xParGroupRegister(xParGroup* ps)
{
    xASSERT(133, ps);

    if (!sParGroupRegTableInit) {
        xParGroupRegisterInit();
    }

    for (U8 i = 0; i < REG_TABLE_MAX; i++) {
        if (sParGroupRegTable[i] == NULL) {
            sParGroupRegTable[i] = ps;
            ps->m_regidx = i;
            break;
        }
    }
}

void xParGroupUnregister(xParGroup* ps)
{
    xASSERT(159, ps);
    xASSERT(160, sParGroupRegTableInit);

    for (U8 i = 0; i < REG_TABLE_MAX; i++) {
        if (sParGroupRegTable[i] == ps) {
            sParGroupRegTable[i] = NULL;
            ps->m_regidx = 0;
            break;
        }
    }
}

void xParGroupSetActive(xParGroup* ps, U32 isActive)
{
    xASSERT(226, ps);

    ps->m_active = isActive;
}

void xParGroupKillAllParticles(xParGroup* ps)
{
    xPar* i = NULL, *tmp = NULL;

    xASSERT(274, ps);

    i = ps->m_root;
    while (i) {
        tmp = i;
        i = i->m_next;
        xParGroupKillPar(ps, tmp);
    }
}

void xParGroupAnimate(xParGroup* ps, F32 dt)
{
    xPar* i, *tmp;
    F32 age = dt;

    xASSERT(303, ps);

    i = ps->m_root;

    if (ps->m_flags & 0x4) {
        age = 0.0f;
    }
    if (ps->m_root == NULL) {
        ps->m_alive = FALSE;
    }

    while (i) {
        F32 col;

        i->m_lifetime -= age;
        if (i->m_lifetime < 0.0f) {
            tmp = i;
            i = i->m_next;
            xParGroupKillPar(ps, tmp);
            continue;
        }
        
        col = CLAMP(i->m_cfl[0] + i->m_cvel[0] * dt, 0.0f, 255.0f);
        i->m_cfl[0] = col;
        i->m_c[0] = col;

        col = CLAMP(i->m_cfl[1] + i->m_cvel[1] * dt, 0.0f, 255.0f);
        i->m_cfl[1] = col;
        i->m_c[1] = col;

        col = CLAMP(i->m_cfl[2] + i->m_cvel[2] * dt, 0.0f, 255.0f);
        i->m_cfl[2] = col;
        i->m_c[2] = col;

        col = CLAMP(i->m_cfl[3] + i->m_cvel[3] * dt, 0.0f, 255.0f);
        i->m_cfl[3] = col;
        i->m_c[3] = col;

        i->m_size += i->m_sizeVel * dt;

        i = i->m_next;
    }    
}

void xParGroupSetSortDistance(F32 sortDistance, U32 sortID)
{
    sSortDistance = sortDistance;
    sSortID = sortID;
}

void xParGroupAddParP(xParGroup* ps, xPar* p)
{
    xASSERT(500, p);
    xASSERT(501, ps);

    ps->m_alive = TRUE;
    ps->m_num_of_particles++;

    p->m_prev = NULL;
    p->m_next = NULL;

    if (ps->m_root) {
        if (sSortID != 0) {
            xPar* test, *prev; 

            p->m_dist2Camera = sSortDistance;
            p->m_sortID = sSortID;

            test = ps->m_root;
            prev = NULL;

            xASSERT(535, test->m_prev == NULL);
            xASSERT(536, p != test);

            while (test && sSortID != test->m_sortID && sSortDistance < test->m_dist2Camera) {
                prev = test;
                test = test->m_next;
            }

            p->m_prev = prev;
            p->m_next = test;

            if (test) {
                test->m_prev = p;
            } else {
                xASSERT(550, prev->m_next == NULL);
            }

            if (prev) {
                prev->m_next = p;
            } else {
                ps->m_root = p;
            }

            sSortDistance = 0.0f;
            sSortID = 0;
        } else {
            p->m_next = ps->m_root;
            ps->m_root->m_prev = p;
            ps->m_root = p;
        }
    } else {
        ps->m_root = p;
    }
}

xPar* xParGroupAddPar(xParGroup* ps)
{
    xASSERT(587, ps);

    xPar* p;
    if (ps->m_flags & 0x2) {
        p = xParAlloc();
        if (!p) {
            S32 myPriority = ps->m_priority;
            for (U8 i = 0; i < REG_TABLE_MAX; i++) {
                if (sParGroupRegTable[i] &&
                        sParGroupRegTable[i] != ps &&
                        (sParGroupRegTable[i]->m_flags & 0x2) &&
                        !(sParGroupRegTable[i]->m_flags & 0x10) &&
                        sParGroupRegTable[i]->m_priority < myPriority &&
                        sParGroupRegTable[i]->m_root) {
                    xParGroupKillPar(sParGroupRegTable[i], sParGroupRegTable[i]->m_root);

                    p = xParAlloc();
                    xASSERT(622, p);
                    break;
                }
            }
        }
    } else {
        if (ps->m_dead) {
            p = ps->m_dead;
            ps->m_dead = p->m_next;
            if (ps->m_dead) {
                ps->m_dead->m_prev = NULL;
            }
        } else {
            p = NULL;
        }
    }
    
    if (p) {
        xParInit(p);
        xParGroupAddParP(ps, p);
    }

    return p;
}

void xParGroupKillPar(xParGroup* ps, xPar* p)
{
    xASSERT(661, p);

    if (p == ps->m_root) {
        ps->m_root = p->m_next;
        if (ps->m_root) {
            ps->m_root->m_prev = NULL;
        }
        p->m_next = NULL;
        p->m_prev = NULL;
    } else {
        if (p->m_next) {
            p->m_next->m_prev = p->m_prev;
        }
        if (p->m_prev) {
            p->m_prev->m_next = p->m_next;
        }
        p->m_prev = NULL;
        p->m_next = NULL;
    }

    ps->m_num_of_particles--;

    if (ps->m_flags & 0x2) {
        xParFree(p);
    } else {
        xParGroupAddParToDeadList(ps, p);
    }
}

void xParGroupAddParToDeadList(xParGroup* ps, xPar* p)
{
    xASSERT(738, p);

    if (ps->m_dead) {
        ps->m_dead->m_prev = p;
    }

    p->m_next = ps->m_dead;
    p->m_prev = NULL;

    ps->m_dead = p;
}

#ifndef NON_MATCHING
void xParGroupAllocPars(xParGroup* ps, S32 numPars)
{
    xASSERT(0, ps->m_dead == NULL);
    xASSERT(0, ps->m_alive == FALSE);
    xASSERT(0, numPars >= 0);
}
#endif
