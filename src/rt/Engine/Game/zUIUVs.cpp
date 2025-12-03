#include "zUIUVs.h"

#include "xDebug.h"
#include "xMemMgr.h"

F32* zUIUVs_Init(RpAtomic* atomic, U32& texCoordsCount)
{
    F32* uvCoords = NULL;
    texCoordsCount = 0;

    RpGeometry* geom = RpAtomicGetGeometry(atomic);
    xASSERT(10, geom);

    if (geom) {
        RwTexCoords* c = RpGeometryGetVertexTexCoords(geom, rwTEXTURECOORDINATEINDEX0);
        xASSERT(14, c);

        if (c) {
            S32 numcoordsets = RpGeometryGetNumTexCoordSets(geom);
            S32 i;
            S32 numVerts = RpGeometryGetNumVertices(geom);

            texCoordsCount = numVerts * 2;
            uvCoords = new (eMemStaticType_0, eMemMgrTag_HUD) F32[texCoordsCount];

            F32* uv = uvCoords;
            RpGeometryLock(geom, rpGEOMETRYLOCKTEXCOORDS1);
            for (i = 0; i < numVerts; i++) {
                uv[0] = c->u;
                uv[1] = c->v;
                uv += 2;
                c++;
            }
            RpGeometryUnlock(geom);
        }
    }

    return uvCoords;
}
