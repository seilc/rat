#ifndef ZPLAYERCONSTRAINER_H
#define ZPLAYERCONSTRAINER_H

#include "xMath.h"
#include "xVec3.h"

struct SphereCollisionResults;
struct xCam;
struct xMat4x3;
struct xSweptSphere;
struct zPlayer;

class zPlayerConstrainer
{
    static xTWEAKABLE F32 sDefaultNearWall;
    static xTWEAKABLE F32 sDefaultFarWall;
    static F32 sNearWall;
    static F32 sFarWall;
    static bool sEnabled;
    static bool sDebugRender;
    static bool sPlayerDisable;
    static bool sCameraDisable;
    static U32 sDrawnFrame;

public:
    // OOBExtent::flags
    enum
    {
        flag_OOBNearWall = 0x1,
        flag_OOBFarWall = 0x2,
        flag_OOBLeftPlane = 0x4,
        flag_OOBCameraFlags = 0x7,
        flag_OOBRightPlane = 0x8
    };

    struct OOBExtent
    {
        OOBExtent() : flags(0), nearFarDist(0.0f), sideDist(0.0f), magDepen(0.0f), leftXMargin(HUGE), rightXMargin(HUGE) {}

        U32 flags;
        F32 nearFarDist;
        F32 sideDist;
        xVec3 position;
        xVec3 depen;
        F32 magDepen;
        F32 leftXMargin;
        F32 rightXMargin;

        void init()
        {
            flags = 0; nearFarDist = sideDist = magDepen = 0.0f;
            leftXMargin = HUGE;
            rightXMargin = HUGE;
        }

        bool CalcDepen(F32& sideMargin, const xMat4x3& camMat, const xVec3& toTarget, const xVec3& nPlane, U32 calcFlags);
    };

    zPlayerConstrainer();

    static void SetConstraints(F32 nearWall, F32 farWall);
    static void GetDefaults(F32& nearWall, F32& farWall);
    void Enable(bool enable); // UNUSED
    static bool SweptSphereToCameraSideWalls(xSweptSphere& sws); // UNUSED
    static bool SweptSphereToCameraNearWall(xSweptSphere& sws); // UNUSED
    static bool SweptSphereToCameraFarWall(xSweptSphere& sws); // UNUSED
    static void FarWallCollisionResponse(const xVec3&, const SphereCollisionResults& scene_collide, xVec3& response_dpos);
    static U32 PlayersOOB(OOBExtent& extent, const xCam& cam, const xVec3& depenAxis, U32 calcFlags);
    static U32 PlayerOOB(OOBExtent* extent, const zPlayer* player); // UNUSED
    static U32 PositionOOB(OOBExtent& extent, const xMat4x3& camMat, F32 fov, const xVec3&, const xVec3& position, U32 calcFlags);
#if DEBUG || RELEASE
    static void DebugRender(); // UNUSED
    void SetupTweaks(const char* prefix);
#endif
};

#endif
