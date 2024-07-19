#ifndef XHIERARCHYBOUND_H
#define XHIERARCHYBOUND_H

#include "xMath3.h"
#include "xCollideSweptSphere.h"

struct xHierarchTweakInfo
{
    const char** boneNames;
    U8 lastCount;
    S8 hilight;
    S8 boneCount;
    bool rootSelected;
};

struct xHierarchyNode
{
    xSphere sphere;
    U16 userData;
    S8 bone;
};

struct xHierarchyBound
{
    xSphere master;
    xHierarchyNode* nodes;
    U8 count;
    U8 maxCount;
#ifdef DEBUGRELEASE
    xHierarchTweakInfo tweakInfo;
#endif
};

struct xHierarchyBoundInitData
{
    S8 bone;
    F32 radius;
    U16 userData;
};

S32 xHierarchyBoundSetup(xHierarchyBound* bound, const xHierarchyBoundInitData* init, U32 extraBounds);
S32 xHierarchyBoundSetup(xHierarchyBound* bound, U32 nodesNumber);
void xHierarchyBoundInit(xHierarchyBound* bound, const xHierarchyBoundInitData* init, F32 masterRadius, F32 scale);
void xHierarchyBoundDraw(const xHierarchyBound* bound, U8 mR, U8 mG, U8 mB, U8 mA, U8 sR, U8 sG, U8 sB, U8 sA, S8 hilight, U8 hR, U8 hG, U8 hB, U8 hA);
bool xHierarchyBoundTest(const xHierarchyBound* bound, xSweptSphere* sphere, U16* boundUserData, S8* subsphere, bool skipMasterCheck);

#ifdef DEBUGRELEASE
void xHierarchyBoundAddDebugTweaks(xHierarchyBound* bound, const char* base, const char** boneNames, S32 boneCount);
#else
#define xHierarchyBoundAddDebugTweaks(bound, base, boneNames, boneCount)
#endif

#endif
