#include "xNavigationMeshAsset.h"

#include "xEnt.h"
#include "zScene.h"
#include "xGlobals.h"
#include "zNPCCommon.h"

#include "decomp.h"

static const U32 MAX_CIRCLES = 50;

#if !MASTER
DECOMP_FORCEFLOAT(1.0f)
DECOMP_FORCEACTIVE(&zMeshCircle::SetPosition)
DECOMP_FORCEACTIVE(xMath2NearestPointOnLine)
#endif

namespace {
    zMeshCircle* sFreeCirclePool = NULL;
}

void xNavigationMeshAsset::Init()
{
    circleList = NULL;
}

void xNavigationMeshAsset::RegisterAllEntities()
{
    for (S32 m = 0; m < numSubMeshes; m++) {
        for (S32 i = 0; i < subMeshes[m].numObjects; i++) {
            RegisterEntity(subMeshes[m].objects[i]);
        }
    }
}

void xNavigationMeshAsset::RegisterEntity(U32 id)
{
    for (zMeshCircle* circle = circleList; circle != NULL; circle = circle->masterListNext) {
        if (circle->ownerEntity != NULL && circle->ownerEntity->id == id) {
            return;
        }
    }

    xEnt* entity = (xEnt*)zSceneFindObject(id);
    xFAIL_RETURN_IF_L(76, entity == NULL);
    xASSERT_L(77, entity->baseFlags & k_XBASE_IS_ENTITY);

    if (entity->frame == 0) {
        xASSERT_FMT_L(82, entity->frame != 0,
                     "Entity %s added to navigation mesh but it doesn't have a frame.  Maybe a bad Maya reference.",
                     xSceneID2Name(xglobals->sceneCur, entity->id));
        return;
    }

    xCylinder cylinder;
    xBoundGetCylinder(cylinder, entity->bound);

    zMeshCircle* newCircle = new (xMEMALLOC_L(sizeof(zMeshCircle), 0, eMemMgrTag_NavigationMesh, 0, 89)) zMeshCircle();

    newCircle->ownerEntity = entity;
    newCircle->mesh = this;
    newCircle->SetPosition(entity->frame->mat.pos);
    newCircle->radius = cylinder.r;

    newCircle->masterListNext = circleList;
    circleList = newCircle;
}

zMeshCircle* xNavigationMeshAsset::GetNearbyCircles(const xVec3& position, F32 radius, F32 speed, xEnt* skipEntity, xEnt* ourEntity, U32 flags)
{
    zMeshCircle* nearbyCircles = NULL;
    F32 distanceTolerance = radius + speed + 1.0f;
    distanceTolerance *= distanceTolerance;

    for (zMeshCircle* circle = circleList; circle != NULL; circle = circle->masterListNext) {
        if (skipEntity == circle->ownerEntity) {
            continue;
        }
        if (ourEntity != NULL && ourEntity->baseType == eBaseTypeNPC) {
            zNPC::zNPCCommon* npc = (zNPC::zNPCCommon*)ourEntity;
            if (!circle->IsActive(npc->GetNPCType())) {
                continue;
            }
        }
        if (ourEntity != NULL && ourEntity->baseType == eBaseTypePlayer) {
            if (!circle->IsActive(10)) {
                continue;
            }
        }
        if ((flags & 0x1) && circle->ownerEntity != NULL && (circle->ownerEntity->miscflags & 0x40)) {
            continue;
        }
        if ((flags & 0x2) && circle->ownerEntity->baseType == eBaseTypePlayer) {
            continue;
        }
        if ((flags & 0x8) && circle->ownerEntity->baseType == eBaseTypeNPC) {
            continue;
        }
        if ((flags & 0x4) && ourEntity->baseType == eBaseTypeNPC) {
            zNPC::zNPCCommon* npc = (zNPC::zNPCCommon*)ourEntity;
            if (npc->perception.GetPriorityTarget() == circle->ownerEntity) {
                continue;
            }
        }
        if (circle->ownerEntity != NULL && !xEntIsEnabled(circle->ownerEntity)) {
            continue;
        }

        if (position.Distance2(circle->GetCenter()) < distanceTolerance) {
            circle->nextNearbyCircle = nearbyCircles;
            nearbyCircles = circle;
        }
    }
    
    return nearbyCircles;
}

zMeshCircle* xNavigationMeshAsset::FindMeshCircleForEnt(const xEnt* ent)
{
    for (zMeshCircle* circle = circleList; circle != NULL; circle = circle->masterListNext) {
        if (circle->ownerEntity == ent) {
            return circle;
        }
    }
    return NULL;
}

void xNavigationMeshAsset::AddCircle(zMeshCircle* circle)
{
    xFAIL_RETURN_IF_L(200, circle == NULL);

    if (circle->mesh != this) {
        circle->masterListNext = circleList;
        circleList = circle;
        circle->mesh = this;
    }
}

bool xNavigationMeshAsset::RemoveCircle(zMeshCircle* circle)
{
    xFAIL_RETURN_VALUE_IF_L(219, circle == NULL, false);
    xFAIL_RETURN_VALUE_IF_L(220, circle->mesh != this, false);

    zMeshCircle* currentCircle = circleList;
    zMeshCircle* prevCircle = NULL;
    while (currentCircle != NULL) {
        if (currentCircle == circle) {
            if (prevCircle != NULL) {
                prevCircle->masterListNext = circle->masterListNext;
            } else {
                circleList = circle->masterListNext;
            }
            circle->masterListNext = NULL;
            circle->mesh = NULL;
            return true;
        }
        prevCircle = currentCircle;
        currentCircle = currentCircle->masterListNext;
    }

    return false;
}

void xNavigationMeshAsset::SceneInit()
{
    xASSERT_L(262, sFreeCirclePool == NULL);

    sFreeCirclePool = new (xMEMALLOC_L(sizeof(zMeshCircle) * MAX_CIRCLES, 0, eMemMgrTag_NavigationMesh, 0, 263)) zMeshCircle();

    for (U32 i = 0; i < MAX_CIRCLES-1; i++) {
        sFreeCirclePool[i].masterListNext = &sFreeCirclePool[i+1];
    }
}

void xNavigationMeshAsset::SceneExit()
{
    sFreeCirclePool = NULL;
}

zMeshCircle* xNavigationMeshAsset::GetFreeCircle()
{
    xASSERT_FMT_L(279, sFreeCirclePool, "Running out of free mesh circles.  Increase pool size?");

    if (sFreeCirclePool) {
        zMeshCircle* circle = sFreeCirclePool;

        sFreeCirclePool = sFreeCirclePool->masterListNext;

        circle->masterListNext = NULL;
        circle->radius = 0.0f;

        return circle;
    }

    return NULL;
}

void xNavigationMeshAsset::ReleaseCircle(zMeshCircle* circle)
{
    circle->masterListNext = sFreeCirclePool;
    sFreeCirclePool = circle;
}

DECOMP_FORCEACTIVE(
    "((U32)0xFFFFFFFF) / (MAX_VERTICES_PER_SUB_MESH * MAX_VERTICES_PER_SUB_MESH ) > subMeshId",
    "We have hit the U32 limit and wrapped around the ID. This is bad because triangles will share IDs.",
    "id <= ( id + sub_mesh_shifted )"
)

S32 xNavigationMeshAssetSubMesh::getTriangle(const xVec3& point, S32 current_triangle) const
{
    xASSERT_L(460, (current_triangle == NO_TRIANGLE) || ((current_triangle < numTriangles) && (current_triangle >= 0)));

    if (current_triangle != NO_TRIANGLE) {
        if (inTriangle(current_triangle, point)) {
            F32 triangleHeight = getHeight(current_triangle, point.x, point.z);
            if (point.y + 10.0f > triangleHeight && point.y - 10.0f < triangleHeight) {
                return current_triangle;
            }
        }
        for (S32 i = 0; i < 3; i++) {
            S32 triangle = portal[current_triangle*3+i];
            if (triangle != NO_TRIANGLE) {
                if (inTriangle(portal[current_triangle*3+i], point)) {
                    F32 triangleHeight = getHeight(triangle, point.x, point.z);
                    if (point.y + 3.0f > triangleHeight && point.y - 3.0f < triangleHeight) {
                        return triangle;
                    }
                }
            }
        }
    }
    
    xASSERT_FMT_L(510, numTriangles >= 0 && numTriangles < MAX_TRIANGLES_PER_SUB_MESH,
                 "Corrupted navigation mesh.  Invalid triangle count!");
    
    for (S32 i = 0; i < numTriangles; i++) {
        if (inTriangle(i, point)) {
            F32 triangleHeight = getHeight(i, point.x, point.z);
            if (point.y + 3.0f > triangleHeight && point.y - 3.0f < triangleHeight) {
                return i;
            }
        }
    }
    
    return NO_TRIANGLE;
}

bool xNavigationMeshAssetSubMesh::inTriangle(S32 triangle_index, const xVec3& point) const
{
    xVec3* v[3];
    v[0] = &vertices[triangles[triangle_index].a];
    v[1] = &vertices[triangles[triangle_index].b];
    v[2] = &vertices[triangles[triangle_index].c];

    if (((point.x - v[0]->x) * (v[1]->z - v[0]->z) + (point.z - v[0]->z) * (v[0]->x - v[1]->x)) < -0.1f) {
        return false;
    }

    if (((point.x - v[1]->x) * (v[2]->z - v[1]->z) + (point.z - v[1]->z) * (v[1]->x - v[2]->x)) < -0.1f) {
        return false;
    }

    if (((point.x - v[2]->x) * (v[0]->z - v[2]->z) + (point.z - v[2]->z) * (v[2]->x - v[0]->x)) < -0.1f) {
        return false;
    }

    return true;
}

S32 xNavigationMeshAssetSubMesh::lookup_next(S32 source, S32 destination) const
{
    U32 bitIndex = ((destination * numTriangles + source) << 1);
    U32 byteIndex = (bitIndex >> 3);
    U32 bitShift = (bitIndex & 0x7);

    return (U8)((portalLookup[byteIndex] >> bitShift) & 0x3);
}

bool xNavigationMeshAsset::GetTriangle(const xVec3& position, S32 subMeshGuess, S32 triGuess, S32& subMeshIndex, S32& triIndex)
{
    xFAIL_RETURN_VALUE_IF_L(618, subMeshGuess < 0 || subMeshGuess >= numSubMeshes, false);
    xFAIL_RETURN_VALUE_IF_L(619, triGuess != NO_TRIANGLE && (triGuess < 0 || triGuess >= subMeshes[subMeshGuess].numTriangles), false);

    for (subMeshIndex = 0; subMeshIndex < numSubMeshes; subMeshIndex++) {
        xNavigationMeshAssetSubMesh* sub_mesh = &subMeshes[subMeshIndex];
        S32 useGuessTriangle = (subMeshIndex == subMeshGuess) ? triGuess : NO_TRIANGLE;

        triIndex = sub_mesh->getTriangle(position, useGuessTriangle);
        if (triIndex != NO_TRIANGLE) {
            return true;
        }
    }

    triIndex = NO_TRIANGLE;
    subMeshIndex = 0;
    return false;
}

// RESTORED: from RotU GC
void xNavigationMeshAsset::GetClosestEdgePoint(const xVec3& anyPosition, S32& subMeshIndex, S32& triangleIndex, xVec3& pointOnMesh)
{
    F32 smallest_distance2 = 1e38f;

    for (S32 m = 0; m < numSubMeshes; m++) {
        xNavigationMeshAssetSubMesh* sub_mesh = &subMeshes[m];

        for (S32 i = 0; i < sub_mesh->numTriangles; i++) {
            const U8* portal = &sub_mesh->portal[i * 3];
            const U8* a = &sub_mesh->triangles[i].a;
            const U8* b = a + 1;
            const U8* end = a + 3;

            for (; a != end; b++, a++, portal++) {
                if (b == end) b = &sub_mesh->triangles[i].a;

                S32 triangle_index = *portal;
                if (triangle_index == NO_TRIANGLE) {
                    const xVec3& vertex_a = sub_mesh->vertices[*a];
                    const xVec3& vertex_b = sub_mesh->vertices[*b];
                    F32 portal_point_x, portal_point_y;

                    F32 line_t = xMath2NearestPointOnLine(portal_point_x, portal_point_y, anyPosition.x, anyPosition.z, vertex_a.x, vertex_a.z, vertex_b.x, vertex_b.z);

                    F32 CPx = portal_point_x - anyPosition.x;
                    F32 CPy = portal_point_y - anyPosition.z;
                    F32 line_dist2 = CPx * CPx + CPy * CPy;
                    F32 y_diff = anyPosition.y - sub_mesh->getHeight(i, portal_point_x, portal_point_y);
                    F32 portal_distance2 = y_diff * y_diff + line_dist2;

                    if (smallest_distance2 > portal_distance2) {
                        smallest_distance2 = line_dist2;
                        pointOnMesh.x = portal_point_x;
                        pointOnMesh.z = portal_point_y;
                        triangleIndex = i;
                        subMeshIndex = m;
                        pointOnMesh.y = sub_mesh->getHeight(triangleIndex, pointOnMesh.x, pointOnMesh.z);
                    }
                }
            }
        }
    }
}
// END RESTORED

void xNavigationMeshAsset::CheckSubMeshExits(S32& subMeshIndex, S32& triangleIndex)
{
    xFAIL_RETURN_IF_L(722, subMeshIndex < 0 || subMeshIndex >= numSubMeshes);

    xNavigationMeshAssetSubMesh* subMesh = &subMeshes[subMeshIndex];

    xFAIL_RETURN_IF_L(727, triangleIndex < 0 || triangleIndex >= subMesh->numTriangles);
    
    for (S32 i = 0; i < subMesh->numExits; i++) {
        if (triangleIndex == subMesh->exits[i].exitTriangleIndex) {
            subMeshIndex = subMesh->exits[i].neighborMeshIndex;
            triangleIndex = subMesh->exits[i].destTriangleIndex;
            break;
        }
    }
}
