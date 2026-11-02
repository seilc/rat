#ifndef I2DQUADBUFFER_H
#define I2DQUADBUFFER_H

#include "types.h"
#include "xColor.h"
#include "xDebug.h"
#include "iCamera.h"

#include <rwcore.h>

struct i2dRect
{
    union
    {
        struct
        {
            F32 tlx;
            F32 tly;
            F32 brx;
            F32 bry;
        };
        struct
        {
            F32 tlu;
            F32 tlv;
            F32 bru;
            F32 brv;
        };
    };

    i2dRect() {}
};

class i2dQuadBuffer
{
public:
    i2dQuadBuffer(RwIm2DVertex* verts, U16 vertexCount)
    {
        vertsUsed = 0;
        vertsMax = vertexCount;
        vertArray = verts;
    }

    bool Set(i2dRect* location, i2dRect* uv_coords, xColor color)
    {
        xASSERT_L(70, vertsUsed < vertsMax);
        xASSERT_L(71, location);
        xASSERT_L(72, uv_coords);

        F32 r1_8 = giCameraOONearClip;

        RwIm2DVertexSetScreenX(&vertArray[vertsUsed], location->tlx);
        RwIm2DVertexSetScreenY(&vertArray[vertsUsed], location->tly);
        RwIm2DVertexSetU(&vertArray[vertsUsed], uv_coords->tlu, r1_8);
        RwIm2DVertexSetV(&vertArray[vertsUsed], uv_coords->tlv, r1_8);

        RwIm2DVertexSetScreenX(&vertArray[vertsUsed + 1], location->tlx);
        RwIm2DVertexSetScreenY(&vertArray[vertsUsed + 1], location->bry);
        RwIm2DVertexSetU(&vertArray[vertsUsed + 1], uv_coords->tlu, r1_8);
        RwIm2DVertexSetV(&vertArray[vertsUsed + 1], uv_coords->brv, r1_8);

        RwIm2DVertexSetScreenX(&vertArray[vertsUsed + 2], location->brx);
        RwIm2DVertexSetScreenY(&vertArray[vertsUsed + 2], location->bry);
        RwIm2DVertexSetU(&vertArray[vertsUsed + 2], uv_coords->bru, r1_8);
        RwIm2DVertexSetV(&vertArray[vertsUsed + 2], uv_coords->brv, r1_8);

        RwIm2DVertexSetScreenX(&vertArray[vertsUsed + 3], location->brx);
        RwIm2DVertexSetScreenY(&vertArray[vertsUsed + 3], location->tly);
        RwIm2DVertexSetU(&vertArray[vertsUsed + 3], uv_coords->bru, r1_8);
        RwIm2DVertexSetV(&vertArray[vertsUsed + 3], uv_coords->tlv, r1_8);

        RwIm2DVertexSetIntRGBA(&vertArray[vertsUsed], color.r, color.g, color.b, color.a);
        RwIm2DVertexSetIntRGBA(&vertArray[vertsUsed + 1], vertArray[vertsUsed].r, vertArray[vertsUsed].g, vertArray[vertsUsed].b, vertArray[vertsUsed].a);
        RwIm2DVertexSetIntRGBA(&vertArray[vertsUsed + 2], vertArray[vertsUsed].r, vertArray[vertsUsed].g, vertArray[vertsUsed].b, vertArray[vertsUsed].a);
        RwIm2DVertexSetIntRGBA(&vertArray[vertsUsed + 3], vertArray[vertsUsed].r, vertArray[vertsUsed].g, vertArray[vertsUsed].b, vertArray[vertsUsed].a);

        vertsUsed += 4;
        if (vertsUsed == vertsMax) {
            Flush();
        }
        return true;
    }

    bool Set(const i2dRect*, const i2dRect*, xColor, F32);
    bool Flush();

private:
    U16 vertsUsed;
    U16 vertsMax;
    RwIm2DVertex* vertArray;
};

#endif
