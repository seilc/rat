#ifndef XNAVIGATIONMESHASSET_H
#define XNAVIGATIONMESHASSET_H

#include "xBaseAsset.h"
#include "xMath3.h"
#include "zMeshCircle.h"

static const U8 NO_TRIANGLE = 0xFF;
static const U32 MAX_TRIANGLES_PER_SUB_MESH = (NO_TRIANGLE-1);

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

        U32 vertex(S32 i)
        {
            xFAIL_AND_RETURN_VALUE_IF(219, i < 0 || i > 2, 0);
            return ((U8*)&a)[i];
        }
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
    S32 GetNeighbor(S32 triangle, S32 neighborNumber) const;
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

    bool GetTriangle(const xVec3& position, S32 subMeshGuess, S32 triGuess, S32& subMeshIndex, S32& triIndex);
    void CheckSubMeshExits(S32& subMeshIndex, S32& triangleIndex);

    F32 getHeight(S32 subMeshIndex, S32 triangle, const xVec3& A) const
    {
        xASSERT(236, subMeshIndex >= 0 && subMeshIndex < numSubMeshes);
        return subMeshes[subMeshIndex].getHeight(triangle, A.x, A.z);
    }

#ifndef NON_MATCHING
    U32 getId(const xNavigationMeshAssetSubMesh*, U32, U32) const;
#endif
};

inline F32 xNavigationMeshAssetSubMesh::getHeight(S32 triangle, F32 x, F32 z) const
{
    xFAIL_AND_RETURN_VALUE_IF(250, triangle < 0 || triangle >= numTriangles, 99999.0f);

    xVec3* a = &vertices[triangles[triangle].a];
    xVec3* b = &vertices[triangles[triangle].b];
    xVec3* c = &vertices[triangles[triangle].c];
    F32 Ax = a->x;
    F32 Az = a->z;
    F32 ABx = b->x - Ax;
    F32 ABz = b->z - Az;
    F32 ACx = c->x - Ax;
    F32 ACz = c->z - Az;
    F32 Ny = ABz * ACx - ACz * ABx;

    if (!Ny) {
        return 0.0f;
    }

    F32 iNy = 1.0f / Ny;
    F32 Ay = a->y;
    F32 ABy = b->y - Ay;
    F32 ACy = c->y - Ay;
    F32 Nx = ABy * ACz - ACy * ABz;
    F32 Nz = ABx * ACy - ACx * ABy;

    return (Nx * (Ax - x) + Ay * Ny + Nz * (Az - z)) * iNy;
}

inline S32 xNavigationMeshAssetSubMesh::GetNeighbor(S32 triangle, S32 neighborNumber) const
{
    xFAIL_AND_RETURN_VALUE_IF(314, triangle < 0 || triangle >= numTriangles, NO_TRIANGLE);
    xFAIL_AND_RETURN_VALUE_IF(315, neighborNumber < 0 || neighborNumber > 2, NO_TRIANGLE);
    return portal[triangle * 3 + neighborNumber];
}

#endif
