#ifndef ZCAMERAFLYASSET_H
#define ZCAMERAFLYASSET_H

#include "xDynAsset.h"

struct CameraFly_asset : xDynAsset
{
    enum { VERSION = 1 };

    U32 flyID;
};

#endif
