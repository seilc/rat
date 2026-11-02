#ifndef XLIGHTASSET_H
#define XLIGHTASSET_H

#include "xDynAsset.h"
#include "xColor.h"
#include "xMath3.h"

enum eLightType
{
    eLightType_Ambient,
    eLightType_Spotlight,
    eLightType_Directional,
    eLightType_Point
};

struct xLightAsset : xDynAsset
{
    static const char* type_name() { return "Effect:light"; }

    enum { VERSION = 2 };

    eLightType lightType;
    U32 lightEffectID;
    F32 lightEffectSpeed;
    U32 lightFlags;
    xFColor lightColor;
    xSphere lightSphere;
    U32 attachID;
    U32 lightCardID;
    F32 lightCardScale;
};

#endif
