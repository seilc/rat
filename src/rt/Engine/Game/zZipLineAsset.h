#ifndef ZZIPLINEASSET_H
#define ZZIPLINEASSET_H

#include "xBase.h"
#include "xVec3.h"

class xNurbs;
struct xEnt;

struct zZipLineAsset : xBaseAsset
{
    U8 dismount_type;
    xVec3 position;
    F32 hang_length;
    union
    {
        U32 startSplineID;
        xNurbs* spline;
    };
    union
    {
        U32 boundEntityID;
        xEnt* boundEntity;
    };
    F32 speed;
    U32 flags;
};

#endif
