#ifndef I2DQUADBUFFER_H
#define I2DQUADBUFFER_H

#include "types.h"

#include <rwcore.h>

struct i2dQuadBuffer
{
    i2dQuadBuffer(RwIm2DVertex* verts, U16 vertexCount)
    {
        vertsUsed = 0;
        vertsMax = vertexCount;
        vertArray = verts;
    }

private:
    U16 vertsUsed;
    U16 vertsMax;
    RwIm2DVertex* vertArray;
};

#endif
