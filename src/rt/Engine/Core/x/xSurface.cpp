#include "xSurface.h"

#include "xMemMgr.h"
#include "xDebug.h"

#include "decomp.h"

static xSurface* surfs;
static U16 nsurfs;

void xSurfaceInit(U16 num_surfs)
{
    nsurfs = num_surfs;

    if (nsurfs) {
        surfs = (xSurface*)xMEMALLOC_L(nsurfs * sizeof(xSurface), 0, eMemMgrTag_EnvAbstract, 0, 28);
        xASSERT_L(29, surfs);

        for (U16 i = 0; i < nsurfs; i++) {
            surfs[i].idx = i;
        }
    } else {
        surfs = NULL;
    }
}

DECOMP_FORCEACTIVE("dflt", &xBase::operator=)

void xSurfaceExit()
{
}

void xSurfaceSave(xSurface* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(75, ent);
    xASSERT_L(76, s);
}

void xSurfaceLoad(xSurface* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(94, ent);
    xASSERT_L(95, s);
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

    xASSERT_L(115, n < nsurfs);

    return &surfs[n];
}
