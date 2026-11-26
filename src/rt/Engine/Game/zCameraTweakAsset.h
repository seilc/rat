#ifndef ZCAMERATWEAKASSET_H
#define ZCAMERATWEAKASSET_H

#include "xDynAsset.h"

struct CameraTweak_asset : xDynAsset
{
    enum { VERSION = 2 };

    S32 priority;
    F32 time;
    F32 pitch_adjust;
    F32 dist_adjust;
    F32 pivot_height_adjust;
    F32 fov_adjust;
};

#endif
