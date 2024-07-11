#ifndef XSTRANSVC_H
#define XSTRANSVC_H

#include "xRegionSupport.h"
#include "xpkrsvc.h"

#include <stdio.h>

const char* xSTAssetName(U32 aid);
const char* xSTAssetName(void* raw_HIP_asset);
void* xSTFindAsset(U32 aid, U32* size);
S32 xSTAssetCountByType(U32 type);
void* xSTFindAssetByType(U32 type, S32 idx, U32* size);
S32 xSTGetAssetInfoByType(U32 type, S32 idx, PKRAssetTOCInfo* tocainfo);
xRegion xSTGetLocalizationEnum();

inline const char* xSTAssetNameOrID(U32 aid)
{
    const char* name = xSTAssetName(aid);
    if (name) {
        return name;
    }

    static char buffer[16];
    sprintf(buffer, "ID:0x%x", aid);
    return buffer;
}

inline const char* xSTAssetNameOrID(void* raw_HIP_asset)
{
    const char* name = xSTAssetName(raw_HIP_asset);
    if (name) {
        return name;
    }

    static char buffer[16];
    sprintf(buffer, "ADDR:0x%x", (U32)raw_HIP_asset);
    return buffer;
}

#endif
