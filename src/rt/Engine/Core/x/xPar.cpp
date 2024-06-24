#include "xPar.h"

#include "xDebug.h"

xPar gParPool[XPAR_POOL_MAX];
xPar* gParDead;

void xParMemInit()
{
    for (S32 i = 0; i < XPAR_POOL_MAX; i++) {
        xPar* p = &gParPool[i];
        p->m_next = NULL;
        p->m_prev = NULL;

        if (gParDead) {
            gParDead->m_prev = p;
            p->m_next = gParDead;
        }

        gParDead = p;
    }
}

xPar* xParAlloc()
{
    xPar* p = NULL;

    if (!gParDead) {
        return NULL;
    }

    p = gParDead;

    if (p->m_next) {
        p->m_next->m_prev = NULL;
    }

    gParDead = p->m_next;
    p->m_next = NULL;
    p->m_prev = NULL;

    return p;
}

void xParFree(xPar* par)
{
    xASSERT(100, par);

    if (par->m_next) {
        par->m_next->m_prev = par->m_prev;
    }

    if (par->m_prev) {
        par->m_prev->m_next = par->m_next;
    }

    if (gParDead) {
        gParDead->m_prev = par;
    }

    par->m_next = gParDead;
    par->m_prev = NULL;
    gParDead = par;
}

void xParInit(xPar* p)
{
    p->m_pos.x = 0.0f;
    p->m_pos.y = 0.0f;
    p->m_pos.z = 0.0f;
    p->m_vel.x = 0.0f;
    p->m_vel.y = 0.0f;
    p->m_vel.z = 0.0f;
    p->m_size = 0.0f;
    p->m_sizeVel = 0.0f;
    p->m_lifetime = 0.0f;
    p->m_cvel[0] = 0.0f;
    p->m_cvel[1] = 0.0f;
    p->m_cvel[2] = 0.0f;
    p->m_cvel[3] = 0.0f;
    p->m_cfl[0] = 255.0f;
    p->m_cfl[1] = 255.0f;
    p->m_cfl[2] = 255.0f;
    p->m_cfl[3] = 255.0f;
    p->m_c[0] = 255;
    p->m_c[1] = 255;
    p->m_c[2] = 255;
    p->m_c[3] = 255;
    p->m_flag = 0;

    for (S32 i = 0; i < 3; i++) {
        p->m_rotdeg[i] = 0;
    }

    p->m_next = NULL;
    p->m_prev = NULL;
    p->m_texIdx[0] = 0;
    p->m_texIdx[1] = 0;
    p->m_sortID = 0;
    p->m_dist2Camera = 0.0f;
}