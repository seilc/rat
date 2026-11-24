#ifndef ZPLAYERCONSTRAINER_H
#define ZPLAYERCONSTRAINER_H

#include "types.h"

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
    static void SetConstraints(F32 nearWall, F32 farWall);
    static void GetDefaults(F32& nearWall, F32& farWall);
};

#endif
