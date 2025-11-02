#include "xNavigationMeshAsset.h"

#include "xEnt.h"
#include "zScene.h"
#include "xGlobals.h"
#include "zNPCCommon.h"

static const U32 MAX_CIRCLES = 50;

static const U8 NO_TRIANGLE = 0xFF;
static const U32 MAX_TRIANGLES_PER_SUB_MESH = (NO_TRIANGLE-1);

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
    xFAIL_AND_RETURN_IF(76, entity == NULL);
    xASSERT(77, entity->baseFlags & k_XBASE_IS_ENTITY);

    if (entity->frame == 0) {
        xASSERTM(82, entity->frame != 0,
                   "Entity %s added to navigation mesh but it doesn't have a frame.  Maybe a bad Maya reference.",
                   xSceneID2Name(xglobals->sceneCur, entity->id));
        return;
    }

    xCylinder cylinder;
    xBoundGetCylinder(cylinder, entity->bound);

    zMeshCircle* newCircle = new (xMEMALLOC(sizeof(zMeshCircle), 0, eMemMgrTag_NavigationMesh, 0, 89)) zMeshCircle();

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
    xFAIL_AND_RETURN_IF(200, circle == NULL);

    if (circle->mesh != this) {
        circle->masterListNext = circleList;
        circleList = circle;
        circle->mesh = this;
    }
}

bool xNavigationMeshAsset::RemoveCircle(zMeshCircle* circle)
{
    xFAIL_AND_RETURN_VALUE_IF(219, circle == NULL, false);
    xFAIL_AND_RETURN_VALUE_IF(220, circle->mesh != this, false);

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
    xASSERT(262, sFreeCirclePool == NULL);

    sFreeCirclePool = new (xMEMALLOC(sizeof(zMeshCircle) * MAX_CIRCLES, 0, eMemMgrTag_NavigationMesh, 0, 263)) zMeshCircle();

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
    xASSERTM(279, sFreeCirclePool, "Running out of free mesh circles.  Increase pool size?");

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

#ifndef NONMATCHING
U32 xNavigationMeshAsset::getId(const xNavigationMeshAssetSubMesh*, U32 subMeshId, U32 sub_mesh_shifted) const
{
    static const U32 MAX_VERTICES_PER_SUB_MESH = 1;
    xASSERTM(0, ((U32)0xFFFFFFFF) / (MAX_VERTICES_PER_SUB_MESH * MAX_VERTICES_PER_SUB_MESH ) > subMeshId,
               "We have hit the U32 limit and wrapped around the ID. This is bad because triangles will share IDs.");
    U32 id = subMeshId;
    xASSERT(0, id <= ( id + sub_mesh_shifted ));
    return 0;
}
#endif

S32 xNavigationMeshAssetSubMesh::getTriangle(const xVec3& point, S32 current_triangle) const
{
    xASSERT(460, (current_triangle == NO_TRIANGLE) || ((current_triangle < numTriangles) && (current_triangle >= 0)));

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
    
    xASSERTM(510, numTriangles >= 0 && numTriangles < MAX_TRIANGLES_PER_SUB_MESH,
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

    return (portalLookup[byteIndex] >> bitShift) & 0x3;
}
