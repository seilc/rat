#include "xSurface.h"

#include "xMemMgr.h"
#include "xDebug.h"

static xSurface* surfs;
static U16 nsurfs;

void xSurfaceInit(U16 num_surfs)
{
    nsurfs = num_surfs;

    if (nsurfs) {
        surfs = (xSurface*)xMEMALLOC(nsurfs * sizeof(xSurface), 0, eMemMgrTag_EnvAbstract, 0, 28);
        xASSERT(29, surfs);

        for (U16 i = 0; i < nsurfs; i++) {
            surfs[i].idx = i;
        }
    } else {
        surfs = NULL;
    }
}

#ifndef NON_MATCHING
void xSurfaceInit(U16 n, const xSurface* s)
{
    U32 dflt = 0;
    xASSERT(0, dflt);
    surfs[n] = *s;
}
#endif

void xSurfaceExit()
{
}

void xSurfaceSave(xSurface* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT(75, ent);
    xASSERT(76, s);
}

void xSurfaceLoad(xSurface* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT(94, ent);
    xASSERT(95, s);
}

void xSurfaceReset(xSurface*)
{
}

U16 xSurfaceGetNumSurfaces()
{
    return nsurfs;
}

xSurface* xSurfaceGetByIdx(U16 n)
{
    if (!surfs) {
        return NULL;
    }

    xASSERT(115, n < nsurfs);

    return &surfs[n];
}
