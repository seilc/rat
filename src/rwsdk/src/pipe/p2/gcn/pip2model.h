#ifndef PIP2MODEL_H
#define PIP2MODEL_H

#include "batypes.h"
#include "bacolor.h"
#include "drvmodel.h"

/* RWPUBLIC */

#define RXHEAPPLATFORMDEFAULTSIZE  (1 << 12) /* 4k */

typedef struct RxObjSpace3DVertex RxObjSpace3DVertex;
struct RxObjSpace3DVertex
{
    RwReal x;
    RwReal y;
    RwReal z;
    RwReal nx;
    RwReal ny;
    RwReal nz;
    RwUInt8 r;
    RwUInt8 g;
    RwUInt8 b;
    RwUInt8 a;
    RwReal u;
    RwReal v;
};

typedef RxObjSpace3DVertex RxObjSpace3DLitVertex;

typedef RxObjSpace3DLitVertex RwIm3DVertex;

typedef RwIm2DVertex RxScrSpace2DVertex;

#define RwIm3DVertexSetPos(_vert, _imx, _imy, _imz)                                               \
MACRO_START                                                                                       \
{                                                                                                 \
    RwV3d tmp;                                                                                    \
    tmp.x = _imx;                                                                                 \
    tmp.y = _imy;                                                                                 \
    tmp.z = _imz;                                                                                 \
    (_vert)->x = tmp.x;                                                                           \
    (_vert)->y = tmp.y;                                                                           \
    (_vert)->z = tmp.z;                                                                           \
}                                                                                                 \
MACRO_STOP

#define RwIm3DVertexSetNormal(_vert, _imx, _imy, _imz)                                            \
MACRO_START                                                                                       \
{                                                                                                 \
    RwV3d packed;                                                                                 \
    packed.x = _imx;                                                                              \
    packed.y = _imy;                                                                              \
    packed.z = _imz;                                                                              \
    (_vert)->nx = packed.x;                                                                       \
    (_vert)->ny = packed.y;                                                                       \
    (_vert)->nz = packed.z;                                                                       \
}                                                                                                 \
MACRO_STOP

#define RwIm3DVertexSetRGBA(_vert, _r, _g, _b, _a)                                                \
MACRO_START                                                                                       \
{                                                                                                 \
    RwRGBA col;                                                                                   \
    col.red = _r;                                                                                 \
    col.green = _g;                                                                               \
    col.blue = _b;                                                                                \
    col.alpha = _a;                                                                               \
    (_vert)->r = col.red;                                                                         \
    (_vert)->g = col.green;                                                                       \
    (_vert)->b = col.blue;                                                                        \
    (_vert)->a = col.alpha;                                                                       \
}                                                                                                 \
MACRO_STOP

#define RwIm3DVertexSetUV(_vert, _u, _v)                                                          \
MACRO_START                                                                                       \
{                                                                                                 \
    (_vert)->u = _u;                                                                              \
    (_vert)->v = _v;                                                                              \
}                                                                                                 \
MACRO_STOP

/* RWPUBLICEND */
#endif /* PIP2MODEL_H */
