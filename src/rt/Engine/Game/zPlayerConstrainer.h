#ifndef ZPLAYERCONSTRAINER_H
#define ZPLAYERCONSTRAINER_H

#include "xMath.h"
#include "xVec3.h"

struct xCam;

class zPlayerConstrainer
{
    static F32 sDefaultNearWall;
    static F32 sDefaultFarWall;
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
        flag_OOBCameraFlags = 0x7
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
    };

    static void SetConstraints(F32 nearWall, F32 farWall);
    static void GetDefaults(F32& nearWall, F32& farWall);
    static U32 PlayersOOB(OOBExtent& extent, const xCam& cam, const xVec3& depenAxis, U32 calcFlags);
};

#endif
