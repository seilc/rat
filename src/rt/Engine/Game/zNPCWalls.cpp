#include "xCommon.h"
#include "zNPCWalls.h"

#include "xGroup.h"
#include "xMemory.h"
#include "xMovePoint.h"
#include "zBase.h"
#include "zScene.h"

#include "decomp.h"

#include <float.h>

void zNPCWalls::InitDynamic(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidateMin_L(DEBUG ? 34 : 36, asset, asset_size, zNPCWallsAsset, "Enemy:NPC Walls");
    new (&data) zNPCWalls((zNPCWallsAsset*)&asset, asset_size);
}

zNPCWalls::zNPCWalls(zNPCWallsAsset* asset, U32)
{
    xBaseInit(this, asset);

    eventFunc = NULL;
    npcWallsAsset = asset;
}

void zNPCWalls::Setup()
{
    if (npcWallsAsset->npcWallsGroup == 0) {
        xFAIL_MSG_L(DEBUG ? 54 : 56, "There are no walls group attached to NPC Walls widget");
        return;
    }

    bool areWallsInverted = npcWallsAsset->flags & 0x1;
    xBase* b = zSceneFindObject(npcWallsAsset->npcWallsGroup);
    if (!b || b->baseType != eBaseTypeGroup) {
        xFAIL_MSG_L(DEBUG ? 63 : 65, "The widget attached to the NPC walls is not a group!");
        return;
    }

    xGroup* wallsGroup = (xGroup*)b;
    Initialize(wallsGroup, areWallsInverted);
}

void zNPCWalls::Initialize(xGroup* group, bool invertWallNormals)
{
    numberOfWalls = xGroupGetCount(group);
    walls = new (eMemStaticType_0, eMemMgrTag_NPCWalls) zWall[numberOfWalls];

    for (S32 i = 0; i < numberOfWalls; i++) {
        xBase* b = xGroupGetItemPtr(group, i);
        if (!b) {
            continue;
        }

        if (b->baseType != eBaseTypeMovePoint) {
            xFAIL_MSG_L(DEBUG ? 91 : 93, "This NPC Walls group contains a non move point.");
            continue;
        }

        xMovePoint* mp1 = (xMovePoint*)b;
        xMovePoint* mp2 = xMovePointGetPoint(mp1, 0);
        if (!mp2) {
            xFAIL_MSG_L(DEBUG ? 100 : 102, "This NPC Wall point is not connected.");
            continue;
        }

#if RELEASE
        if (invertWallNormals)
            walls[i].InitializeXZ(*mp1->pos, *mp2->pos);
        else
            walls[i].InitializeXZ(*mp2->pos, *mp1->pos);
#else
        if (invertWallNormals)
            walls[i].Initialize(mp1->pos, mp2->pos);
        else
            walls[i].Initialize(mp2->pos, mp1->pos);
#endif
    }
}

#if RELEASE
bool zNPCWalls::IntersectsRayXZ(const xVec3& iRayStart, const xVec3& iRayDelta, F32& oDistRatio, xVec3& oIPoint,
                                xVec3& oNormal)
{
    S32 closestWall = -1;
    xVec2 curPoint, closestPoint;
    F32 ratioToThisWall = 0.0f;
    F32 ratioToClosestWall = HUGE;

    xVec2 iRayStart2D, iRayDelta2D;
    iRayStart2D.assign(iRayStart.x, iRayStart.z); iRayDelta2D.assign(iRayDelta.x, iRayDelta.z);

    for (S32 i = 0; i < numberOfWalls; i++) {
        if (walls[i].IntersectsRay(iRayStart2D, iRayDelta2D, ratioToThisWall, curPoint)) {
            if (ratioToThisWall < ratioToClosestWall) {
                ratioToClosestWall = ratioToThisWall;
                closestWall = i;
                closestPoint = curPoint;
            }
        }
    }

    if (closestWall >= 0) {
        oIPoint.assign(closestPoint.x, 0.0f, closestPoint.y);
        oDistRatio = ratioToClosestWall;
        oNormal.assign(walls[closestWall].normal.x, 0.0f, walls[closestWall].normal.y);
        return true;
    }

    return false;
}

DECOMP_FORCEACTIVE("wallID<numberOfWalls && wallID>=0");

bool zNPCWalls::IntersectsSweptCircle(zSweptCircle& swCircle, S32& wallID)
{
    wallID = -1;

    for (S32 i = 0; i < numberOfWalls; i++) {
        if (walls[i].IntersectsSweptCircle(swCircle)) {
            wallID = i;
        }
    }

    return swCircle.hitSomething;
}
#else
bool zNPCWalls::Intersects(const xVec3& iPointA, const xVec3& iDirection, F32& oDistRatio, xVec3& oIPoint,
                           xVec3& oNormal)
{
    S32 closestWall = -1;
    xVec3 curPoint, closestPoint;
    F32 ratioToThisWall = 0.0f;
    F32 ratioToClosestWall = HUGE;

    for (S32 i = 0; i < numberOfWalls; i++) {
        if (walls[i].Intersects(iPointA, iDirection, ratioToThisWall, curPoint)) {
            if (ratioToThisWall < ratioToClosestWall) {
                ratioToClosestWall = ratioToThisWall;
                closestWall = i;
                closestPoint = curPoint;
            }
        }
    }

    if (closestWall >= 0) {
        oIPoint = closestPoint;
        oDistRatio = ratioToClosestWall;
        oNormal.x = walls[closestWall].nx;
        oNormal.y = 0.0f;
        oNormal.z = walls[closestWall].nz;
        return true;
    }

    return false;
}
#endif

#if RELEASE
void zNPCWalls::zWall::Initialize(xVec2& point1, xVec2& point2)
{
    start = point1;
    end = point2;

    dir.Sub(point2, point1);
    length = dir.SafeNormalize();
    xASSERT_L(206, this->length>0.0f);

    normal.x = dir.y;
    normal.y = -dir.x;
}
#else
void zNPCWalls::zWall::Initialize(xVec3* point1, xVec3* point2)
{
    x = point1->x;
    z = point1->z;
    dx = point2->x - point1->x;
    dz = point2->z - point1->z;

    length = xsqrt(xsqr(dx) + xsqr(dz));

    nx = dz / length;
    nz = -dx / length;
}
#endif

#if RELEASE
bool zNPCWalls::zWall::IntersectsRay(const xVec2& iRayStart, const xVec2& iRayDelta, F32& oDistRatio, xVec2& oIPoint)
{
    F32 dx = end.x - start.x, dy = end.y - start.y, dx2 = iRayDelta.x, dy2 = iRayDelta.y;
    F32 dxStart = iRayStart.x - start.x, dyStart = iRayStart.y - start.y;

    F32 Bot = dx2 * dy - dy2 * dx;

    if (Bot == 0.0f) {
        oDistRatio = 0.0f;
        return false;
    }

    F32 s = (dyStart * dx2 - dxStart * dy2) / Bot;

    if (s >= 0.0f && s < 1.0f) {
        F32 r = (dyStart * dx - dxStart * dy) / Bot;

        if (r >= 0.0f && r < 1.0f) {
            oDistRatio = r;
            oIPoint.x = iRayStart.x + dx2 * r;
            oIPoint.y = iRayStart.y + dy2 * r;
            return true;
        }
    }

    oDistRatio = 0.0f;
    return false;
}

F32 zNPCWalls::zWall::IntersectsLine(const xVec2& iLinePoint, const xVec2& iLineDelta)
{
    F32 dx2 = iLineDelta.x, dy2 = iLineDelta.y, dxStart = iLinePoint.x - start.x, dyStart = iLinePoint.y - start.y;

    F32 Bot = dx2 * (end.y - start.y) - dy2 * (end.x - start.x);

    if (Bot < EPSILON && Bot > -EPSILON) {
        return FLT_MAX;
    }

    return (dyStart * dx2 - dxStart * dy2) / Bot;
}

bool zNPCWalls::zWall::IntersectsSweptCircle(zSweptCircle& swCircle)
{
    F32 R = swCircle.radius;

    xVec2 circleStartToWallStart, circleEndToWallStart;
    circleStartToWallStart.Sub(swCircle.start, start);
    circleEndToWallStart.Sub(swCircle.curEnd, start);

    F32 circleStartDistance = circleStartToWallStart.dot(normal);
    F32 circleEndDistance = circleEndToWallStart.dot(normal);

    if (circleStartDistance <= circleEndDistance) {
        return false;
    }

    if (circleStartDistance < 0.0f || circleEndDistance >= R) {
        return false;
    }

    if (circleStartDistance <= R) {
        F32 circleStartAlong = circleStartToWallStart.dot(dir);

        if (circleStartAlong < 0.0f) {
            return swCircle.SweptCircleToPoint(start) != 0;
        }

        if (circleStartAlong > length) {
            return swCircle.SweptCircleToPoint(end) != 0;
        }

        swCircle.hitSomething = true;
        swCircle.curDist = 0.0f;
        swCircle.curEnd = swCircle.start;
        swCircle.contactPoint.AddScale(start, dir, circleStartAlong);
        return true;
    }

    xVec2 LineSweepStart;
    LineSweepStart.AddScale(swCircle.start, normal, -R);

    F32 s = IntersectsLine(LineSweepStart, swCircle.dir);

    if (s > 1.0f) {
        return swCircle.SweptCircleToPoint(end) != 0;
    }

    if (s < 0.0f) {
        return swCircle.SweptCircleToPoint(start) != 0;
    }

    swCircle.hitSomething = true;
    swCircle.contactPoint.AddScale(start, dir, length * s);

    xVec2 toStart;
    toStart.Sub(swCircle.start, swCircle.contactPoint);

    F32 distAway = swCircle.left.dot(toStart);
    F32 startAlong = -swCircle.dir.dot(toStart);

    F32 backDist = xsqrt(xsqr(swCircle.radius) - xsqr(distAway) + EPSILON);

    swCircle.curDist = startAlong - backDist;
    swCircle.curEnd.AddScale(swCircle.start, swCircle.dir, swCircle.curDist);
    return true;
}
#else
bool zNPCWalls::zWall::Intersects(const xVec3& iPointA, const xVec3& iDirection, F32& oDistRatio, xVec3& oIPoint)
{
    F32 dx2 = iDirection.x, dz2 = iDirection.z, dxStart = iPointA.x - x, dzStart = iPointA.z - z;

    F32 Bot = dx2 * dz - dz2 * dx;

    if (Bot == 0.0f) {
        oDistRatio = 0.0f;
        return false;
    }

    F32 sTop = dzStart * dx2 - dxStart * dz2, s = sTop / Bot;

    if (s >= 0.0f && s < 1.0f) {
        F32 rTop = dzStart * dx - dxStart * dz, r = rTop / Bot;

        if (r >= 0.0f && r < 1.0f) {
            oDistRatio = r;
            oIPoint.AddScale(iPointA, iDirection, r);
            return true;
        }
    }

    oDistRatio = 0.0f;
    return false;
}
#endif
