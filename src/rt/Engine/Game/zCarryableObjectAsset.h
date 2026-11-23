#ifndef ZCARRYABLEOBJECTASSET_H
#define ZCARRYABLEOBJECTASSET_H

#include "xDynAsset.h"
#include "xEntAsset.h"
#include "xSimpleObjAsset.h"

struct zCarryableObjectAsset : xDynAsset
{
    enum { VERSION = 1 };

    xEntAsset entAsset;
    xSimpleObjAsset simpleObjAsset;
    U32 propertyAsset;
};

#endif
