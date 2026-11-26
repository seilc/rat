#include "xCommon.h"
#include "zPlayerConstrainer.h"

#include "xCam.h"
#include "xCollideSweptSphere.h"
#include "xDraw.h"
#include "xGlobals.h"
#include "zAutoTweak.h"
#include "zGlobals.h"
#include "zPlayer.h"

xTWEAKABLE F32 zPlayerConstrainer::sDefaultNearWall = 5.0f;
xTWEAKABLE F32 zPlayerConstrainer::sDefaultFarWall = 20.0f;
F32 zPlayerConstrainer::sNearWall = sDefaultNearWall;
F32 zPlayerConstrainer::sFarWall = sDefaultFarWall;
bool zPlayerConstrainer::sEnabled = true;
bool zPlayerConstrainer::sDebugRender;
bool zPlayerConstrainer::sPlayerDisable;
bool zPlayerConstrainer::sCameraDisable;
U32 zPlayerConstrainer::sDrawnFrame;

xTWEAKABLE F32 cPlayerRadius = 0.65f;
xTWEAKABLE F32 cCamMotionMarginAngle = (PI / 180.0f) * 4.0f;
xTWEAKABLE F32 cPlayerMotionMarginAngle = (PI / 180.0f) * 6.0f;

zPlayerConstrainer::zPlayerConstrainer()
{
    sEnabled = true;
    sDebugRender = false;
    sDrawnFrame = -1;
}

void zPlayerConstrainer::SetConstraints(F32 nearWall, F32 farWall)
{
    sNearWall = nearWall;
    sFarWall = farWall;
}

void zPlayerConstrainer::GetDefaults(F32& nearWall, F32& farWall)
{
    nearWall = sDefaultNearWall;
    farWall = sDefaultFarWall;
}

static inline F32 DotXZ(const xVec3& a, const xVec3& b) { return a.x * b.x + a.z * b.z; }

static inline void ReflectVector(xVec3& o, const xVec3& v, const xVec3& unitAxis)
{
    xVec3SMul(o, unitAxis, 2.0f * xVec3Dot(&unitAxis, &v));
    xVec3SubFrom(o, v);
}

static inline void FrustrumSideNormals(xVec3& nLeft, xVec3& nRight, const xMat4x3& viewMat, F32 fov)
{
    xVec3RotateOnAxis(nLeft, viewMat.right, viewMat.up, 0.5f * fov);
    ReflectVector(nRight, nLeft, viewMat.right);
    xVec3Inv(nLeft);
}

static void DrawPlane(const xVec3& normal, const xMat4x3& viewMat, F32 offset, F32 width, F32 height)
{
    xVec3 forward;
    xVec3Cross(forward, viewMat.up, normal);

    xVec3 c1, c2;
    c2.AddScale(viewMat.pos, normal, offset);
    c1.AddCombine(c2, viewMat.up, 0.5f * height, forward, width);
    c2.AddScale(viewMat.up, 0.5f * -height);
    xDrawGrid(c1, c2, viewMat.up, 0.5f);
}

static void DrawFrustrumPlanes(xVec3& nLeft, xVec3& nRight, const xMat4x3& viewMat, F32 offset)
{
    DrawPlane(nLeft, viewMat, offset, 20.0f, 5.0f);
    DrawPlane(nRight, viewMat, offset, -20.0f, 5.0f);
}

struct PlayerPlaneInfo
{
    xVec3 nPlane;
    F32 newDistToPlane;
};

// UNUSED: needed to match (code from RotU GC)
bool zPlayerConstrainer::SweptSphereToCameraSideWalls(xSweptSphere& sws)
{
    if (!sEnabled || sPlayerDisable)
        return false;

    F32 fov = globals.cam->fov - cPlayerMotionMarginAngle;
    const xMat4x3& camMat = globals.cam->coll_mat;

    xVec3 newPos;
    newPos.AddScale(sws.start, sws.dir, sws.curdist);

    xVec3 toNew;
    toNew.Sub(newPos, camMat.pos);

    xVec3 toStart;
    toStart.Sub(sws.start, camMat.pos);

    PlayerPlaneInfo left, right;
    FrustrumSideNormals(left.nPlane, right.nPlane, camMat, fov);
    left.newDistToPlane = toNew.dot(left.nPlane);
    right.newDistToPlane = toNew.dot(right.nPlane);
    xASSERT(left.newDistToPlane > 0.f || right.newDistToPlane > 0.f);

#if DEBUG || RELEASE
    if (sDebugRender) {
        xDrawSetColor(0, 0, 255, 255);
        DrawFrustrumPlanes(left.nPlane, right.nPlane, camMat, 0.0f);
    }
#endif

    const PlayerPlaneInfo& planeInfo = (left.newDistToPlane < right.newDistToPlane) ? left : right;
    F32 dirDotPlane = sws.dir.dot(planeInfo.nPlane);
    if (dirDotPlane < -0.0001f && planeInfo.newDistToPlane < sws.radius) {
        F32 startDistToPlane = toStart.dot(planeInfo.nPlane);
        if (startDistToPlane <= sws.radius) {
            sws.curdist = 0.0f;
            sws.worldPos.AddScale(sws.start, planeInfo.nPlane, -sws.radius);
        } else {
            F32 t = -(startDistToPlane - sws.radius) / dirDotPlane;
            sws.curdist = t;

            xVec3 intersection;
            intersection.AddScale(sws.start, sws.dir, t);
            sws.worldPos.AddScale(intersection, planeInfo.nPlane, -sws.radius);
        }
        return true;
    }
    return false;
}

// UNUSED: needed to match (code from RotU GC)
bool zPlayerConstrainer::SweptSphereToCameraNearWall(xSweptSphere& sws)
{
    const xMat4x3& camMat = globals.cam->coll_mat;

    xVec3 newPos;
    newPos.AddScale(sws.start, sws.dir, sws.curdist);

    xVec3 toNew;
    toNew.Sub(newPos, camMat.pos);
    F32 distToNew2 = DotXZ(toNew, toNew);

    F32 atDotDir = DotXZ(globals.cam->coll_atXZ, sws.dir);
    if (atDotDir < -0.0001f) {
        F32 planarDistanceToNew = DotXZ(globals.cam->coll_atXZ, toNew) - sws.radius;
        if (planarDistanceToNew < sNearWall) {
            xVec3 toStart;
            toStart.Sub(sws.start, camMat.pos);
            F32 planarDistToStart = DotXZ(toStart, globals.cam->coll_atXZ) - sws.radius;
            if (planarDistToStart <= sNearWall) {
                sws.curdist = 0.0f;
                sws.worldPos.AddScale(sws.start, globals.cam->coll_atXZ, -sws.radius);
            } else {
                F32 t = -(planarDistToStart - sNearWall) / atDotDir;
                xASSERT(t < sws.curdist);
                sws.curdist = t;

                xVec3 intersection;
                intersection.AddScale(sws.start, sws.dir, t);
                sws.worldPos.AddScale(intersection, globals.cam->coll_atXZ, -sws.radius);
            }
            return true;
        }
    }
    return false;
}

// UNUSED: needed to match (code from RotU GC)
bool zPlayerConstrainer::SweptSphereToCameraFarWall(xSweptSphere& sws)
{
    if (!sEnabled || sPlayerDisable)
        return false;

#if DEBUG || RELEASE
    if (sDebugRender)
        DebugRender();
#endif

    const xMat4x3& camMat = globals.cam->coll_mat;

    xVec3 newPos;
    newPos.AddScale(sws.start, sws.dir, sws.curdist);

    xVec3 toNewPos;
    toNewPos.Sub(newPos, camMat.pos);

    F32 distToNewPos2 = DotXZ(toNewPos, toNewPos), radiusAdjustedFarWallDist2 = xsqr(sFarWall - sws.radius);
    if (distToNewPos2 > radiusAdjustedFarWallDist2) {
        xVec3 toStart;
        toStart.Sub(sws.start, camMat.pos);
        F32 distToStart2 = DotXZ(toStart, toStart);
        if (distToNewPos2 > distToStart2) {
            // BUG: always true, distToNewPos2 instead of distToStart2
            if (distToNewPos2 >= radiusAdjustedFarWallDist2) {
                xVec3 inverseSurfNormal;
                inverseSurfNormal = toStart;
                inverseSurfNormal.y = 0.0f;
                inverseSurfNormal.normalize();
                if (inverseSurfNormal.dot(sws.dir) > 0.0115f) {
                    sws.curdist = 0.0f;
                    sws.worldPos.AddScale(sws.start, inverseSurfNormal, sws.radius);
                } else {
                    return false;
                }
            } else {
                xVec3 w;
                w.cross_ypos(sws.dir);
                w.normalize();

                xVec3 BA;
                BA.Sub(camMat.pos, sws.start);
                F32 d = BA.dot(w);

                xVec3 BAdw;
                BAdw.AddScale(BA, w, -d);

                xVec3 BAdw_Cross_v;
                BAdw_Cross_v.cross_ypos(BAdw);

                xVec3 u_Cross_v;
                u_Cross_v.cross_ypos(sws.dir);

                F32 t = BAdw_Cross_v.dot(u_Cross_v) / u_Cross_v.length2(), u_dot_v = sws.dir.dot(xVec3::m_UnitAxisY);
                t += xsqrt(MAX(0.0f, radiusAdjustedFarWallDist2 - xsqr(d)) / (1.0f - xsqr(u_dot_v)));
                if (t < 0.0f)
                    t = 0.0f;
                sws.curdist = t;

                xVec3 center;
                center.AddScale(sws.start, sws.dir, t);

                xVec3 toSphereXZ = { center.x - camMat.pos.x, 0.0f, center.z - camMat.pos.z };
                sws.worldPos.AddScale(sws.start, toSphereXZ, sws.radius / toSphereXZ.length());
            }
            return true;
        }
    }
    return false;
}

void zPlayerConstrainer::FarWallCollisionResponse(const xVec3&, const SphereCollisionResults& scene_collide, xVec3& response_dpos)
{
    const xMat4x3& camMat = globals.cam->coll_mat;

    xVec3 toDesired;
    toDesired.AddSub(scene_collide.contact, response_dpos, camMat.pos);

    xVec3 toContact;
    toContact.Sub(scene_collide.contact, camMat.pos);
    F32 contactDistance = toContact.length();

    response_dpos.AddScale(camMat.pos, toDesired, contactDistance / toDesired.length());

    response_dpos.Sub(scene_collide.contact);
}

static void myDrawArrow(const xVec3* origin, const xVec3* dir, F32 length)
{
    xDrawArrow(origin, dir, false, length);
}

bool zPlayerConstrainer::OOBExtent::CalcDepen(F32& sideMargin, const xMat4x3& camMat, const xVec3& toTarget, const xVec3& nPlane, U32 calcFlags)
{
    F32 perpDistance = toTarget.dot(nPlane) - cPlayerRadius;
    if (perpDistance >= 0.0f) {
        if (calcFlags & 2) {
            F32 margin = perpDistance * camMat.right.dot(nPlane);
            if (margin < 0.0f)
                margin = -margin;
            sideMargin = margin;
        }
        return false;
    }

    sideMargin = 0.0f;
    return true;
}

U32 zPlayerConstrainer::PlayersOOB(OOBExtent& extent, const xCam& cam, const xVec3& depenAxis, U32 calcFlags)
{
    U32 flags = 0;
    extent.init();

    for (S32 i = 0; i < CURPLAYERS.size(); i++)
        if (CURPLAYERS[i]->GetCameraFocus()) {
            OOBExtent localExtent;
            flags |= PositionOOB(localExtent, cam.coll_mat, cam.fov - cCamMotionMarginAngle, depenAxis, CURPLAYERS[i]->frame->mat.pos, calcFlags);
            if (flags) {
                if (!extent.flags || localExtent.magDepen > extent.magDepen) {
                    extent.position = localExtent.position;
                    extent.depen = localExtent.depen;
                    extent.magDepen = localExtent.magDepen;
                }

                if (flags & (flag_OOBNearWall | flag_OOBFarWall))
                    extent.nearFarDist = localExtent.nearFarDist;
                if (flags & (flag_OOBLeftPlane | flag_OOBRightPlane))
                    extent.sideDist = localExtent.sideDist;
                extent.flags |= flags;
            }

            if (localExtent.leftXMargin < extent.leftXMargin)
                extent.leftXMargin = localExtent.leftXMargin;
            if (localExtent.rightXMargin < extent.rightXMargin)
                extent.rightXMargin = localExtent.rightXMargin;
        }

    return flags;
}

U32 zPlayerConstrainer::PositionOOB(OOBExtent& extent, const xMat4x3& camMat, F32 fov, const xVec3&, const xVec3& position, U32 calcFlags)
{
    if (sCameraDisable)
        return 0;

    xVec3 to = { position.x - camMat.pos.x, 0.0f, position.z - camMat.pos.z };
    U32 flags = 0;
    xVec3 atNormalizedXZ = camMat.at;
    atNormalizedXZ.y = 0.0f;
    atNormalizedXZ.normalize();
    extent.magDepen = 0.0f;

    F32 adjustedFarWall = sFarWall - cPlayerRadius, dist2 = DotXZ(to, to);
    if (dist2 > xsqr(adjustedFarWall)) {
        flags |= flag_OOBFarWall;
        extent.nearFarDist = xsqrt(dist2) - adjustedFarWall;
    }

    F32 perpDistance = DotXZ(atNormalizedXZ, to) - cPlayerRadius;
    if (perpDistance < sNearWall) {
        flags |= flag_OOBNearWall;
        extent.nearFarDist = sNearWall - perpDistance;
    }

    xVec3 nLeftPlane, nRightPlane;
    FrustrumSideNormals(nLeftPlane, nRightPlane, camMat, fov);

    if (extent.CalcDepen(extent.leftXMargin, camMat, to, nLeftPlane, calcFlags))
        flags |= flag_OOBLeftPlane;
    if (extent.CalcDepen(extent.rightXMargin, camMat, to, nRightPlane, calcFlags))
        flags |= flag_OOBRightPlane;

#if DEBUG || RELEASE
    if (sDebugRender) {
        static U32 drawnFrame = -1;
        if (drawnFrame != gFrameCount) {
            xDrawSetColor(0, 0, 255, 255);
            DrawFrustrumPlanes(nLeftPlane, nRightPlane, camMat, 0.0f);
            xDrawSetColor(0, 0, 255, 255);
            myDrawArrow(&camMat.pos, &nLeftPlane, 4.0f);
            xDrawSetColor(255, 0, 0, 255);
            myDrawArrow(&camMat.pos, &nRightPlane, 4.0f);
        }
        drawnFrame = gFrameCount;
    }
#endif

    extent.flags = flags;
    if (flags)
        extent.position = position;

#if DEBUG || RELEASE
    if (sDebugRender) {
        static U32 drawnFrame = -1;
        if (drawnFrame != gFrameCount) {
            xVec3 left, right;
            xVec3RotateOnAxis(left, camMat.at, camMat.up, 0.5f * fov);
            ReflectVector(right, left, camMat.at);

            xDrawSetColor(0, 0, 255, 255);
            myDrawArrow(&camMat.pos, &left, 4.0f);
            xDrawSetColor(255, 0, 0, 255);
            myDrawArrow(&camMat.pos, &right, 4.0f);
            xDrawSetColor(255, 255, 255, 255);
            myDrawArrow(&camMat.pos, &camMat.at, 4.0f);
        }
        drawnFrame = gFrameCount;
    }
#endif

    return flags;
}

#if DEBUG || RELEASE
void zPlayerConstrainer::SetupTweaks(const char* prefix)
{
    zAUTOTWEAK(prefix, "Constraints|Near Wall", &sDefaultNearWall, 0.1f, 20.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Constraints|Far Wall", &sDefaultFarWall, 0.1f, 100.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Constraints|Debug Render", &sDebugRender, 0, 1, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Constraints|Player Disable", &sPlayerDisable, 0, 1, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Constraints|Camera Disable", &sCameraDisable, 0, 1, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Constraints|Player Radius (for rail camera motion)", &cPlayerRadius, 0.0f, 3.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Constraints|Camera Margin Angle", &cCamMotionMarginAngle, 0.0f, 15.0f, &cb_tweak_convert_degrees, NULL, 2);
    zAUTOTWEAK(prefix, "Constraints|Player Margin Angle", &cPlayerMotionMarginAngle, 0.0f, 15.0f, &cb_tweak_convert_degrees, NULL, 2);
}
#endif
