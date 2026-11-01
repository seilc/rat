#ifndef XHIERARCHYBOUND_H
#define XHIERARCHYBOUND_H

#include "xMath3.h"
#include "xCollideSweptSphere.h"

struct xBound;
struct xSphereList;
struct xSphereListInitData;

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
#if DEBUG || RELEASE
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
void xHierarchyBoundUpdate(xHierarchyBound* bound, const RwMatrix* mats);
bool xHierarchyBoundTest(const xHierarchyBound* bound, const xSphere* sphere, U16* boundUserData, xIsect* isect, S8* subsphere);
bool xHierarchyBoundTest(const xHierarchyBound* bound, const xSphereList* spheres, U16* boundUserData, U16* sphereUserData, xIsect* isect, S8* subsphere);
bool xHierarchyBoundTest(const xHierarchyBound* bound, const xRay3* testRay, U16* boundUserData, xIsect* isect, S8* subsphere);
bool xHierarchyBoundTest(const xHierarchyBound* bound, const xBox* box, U16* boundUserData, xIsect* isect, S8* subsphere);
bool xHierarchyBoundTest(const xHierarchyBound* bound, const xBox* box, const xMat4x3* mat, U16* boundUserData, xIsect* isect, S8* subsphere);
bool xHierarchyBoundTest(const xHierarchyBound* bound, const xBound* testBound, U16* boundUserData, xIsect* isect, S8* subsphere);
bool xHierarchyBoundTest(const xHierarchyBound* bound, xSweptSphere* sphere, U16* boundUserData, S8* subsphere, bool skipMasterCheck);

#if DEBUG || RELEASE
void xHierarchyBoundAddDebugTweaks(xHierarchyBound* bound, const char* base, const char** boneNames, S32 boneCount);
#else
#define xHierarchyBoundAddDebugTweaks(bound, base, boneNames, boneCount)
#endif

void xSphereListClear(xSphereList* list);
void xSphereListInit(xSphereList* list, const xSphereListInitData* init, U32 extraSpheres);
void xSphereListUpdate(xSphereList* list, const RwMatrix* mats);
void xSphereListDraw(const xSphereList* list, U8 rR, U8 rG, U8 rB, U8 rA, S8 hilight, U8 hR, U8 hG, U8 hB, U8 hA);

#if DEBUG || RELEASE
void xSphereListAddDebugTweaks(xSphereList* list, const char* base, const char** boneNames, S32 boneCount);
#endif

#endif
