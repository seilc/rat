#ifndef XLIGHTKIT_H
#define XLIGHTKIT_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>

struct xLightKitLight
{
    RwUInt32 type;
    RwRGBAReal color;
    RwReal matrix[16];
    RwReal radius;
    RwReal angle;
    RpLight* platLight;
};

struct xLightKit
{
    RwUInt32 tagID;
    RwUInt32 groupID;
    RwUInt32 lightCount;
    xLightKitLight* lightList;
    RwBool blended;
};

struct xLightKitBlendInfo
{
    xLightKit* lightKit;
    xLightKit* lightKitBlend;
    RwReal blendPercentage;
};

extern xLightKitBlendInfo gLastLightKit;

struct xModelInstance;

xLightKit* xLightKit_Prepare(void* data);
void xLightKit_GetBlendInfo(xLightKitBlendInfo& lkitInfo, xModelInstance* model);
S32 xLightKit_Compare(xLightKitBlendInfo& lkitInfo1, xLightKitBlendInfo& lkitInfo2);
void xLightKit_Enable(const xModelInstance& model, RpWorld* world);
void xLightKit_Enable(xLightKit* lkit, RpWorld* world, xLightKit* lkit_blend, RwReal blendPercentage);
xLightKitBlendInfo* xLightKit_GetCurrent(RpWorld*);
void xLightKit_Destroy(xLightKit* lkit);

#endif
