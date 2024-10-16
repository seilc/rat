#ifndef XNAVIGATIONMESHASSET_H
#define XNAVIGATIONMESHASSET_H

#include "xBaseAsset.h"
#include "xMath3.h"
#include "zMeshCircle.h"

class xNavigationMeshAssetSubMesh
{
public:
    struct Exit
    {
        S32 exitTriangleIndex;
        S32 destTriangleIndex;
        S32 neighborMeshIndex;
    };

    struct Triangle
    {
        U8 a;
        U8 b;
        U8 c;
        U8 flags;
    };

    U8* portalLookup;
    U8* portal;
    F32* edgeShift;
    Exit* exits;
    S32 numExits;
    xVec3* vertices;
    S32 numVertices;
    Triangle* triangles;
    S32 numTriangles;
    U32* objects;
    S32 numObjects;
    U8* levelTwoRouteExits;
    U32 runtimeFlags;

    S32 getTriangle(const xVec3& point, S32 current_triangle) const;
    bool inTriangle(S32 triangle_index, const xVec3& point) const;
    S32 lookup_next(S32 source, S32 destination) const;

    F32 getHeight(S32 triangle, F32 x, F32 z) const;
};

class xNavigationMeshAsset : public xBaseAsset
{
public:
    S32 numSubMeshes;
    xNavigationMeshAssetSubMesh* subMeshes;
    zMeshCircle* circleList;

    void Init();
    void RegisterAllEntities();
    void RegisterEntity(U32 id);
    zMeshCircle* GetNearbyCircles(const xVec3& position, F32 radius, F32 speed, xEnt* skipEntity, xEnt* ourEntity, U32 flags);
    zMeshCircle* FindMeshCircleForEnt(const xEnt* ent);
    void AddCircle(zMeshCircle* circle);
    bool RemoveCircle(zMeshCircle* circle);

    static void SceneInit();
    static void SceneExit();
    static zMeshCircle* GetFreeCircle();
    static void ReleaseCircle(zMeshCircle* circle);

#ifndef NONMATCHING
    U32 getId(const xNavigationMeshAssetSubMesh*, U32, U32) const;
#endif
};

#endif
