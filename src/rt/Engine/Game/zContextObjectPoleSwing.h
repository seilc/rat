#ifndef ZCONTEXTOBJECTPOLESWING_H
#define ZCONTEXTOBJECTPOLESWING_H

#include "xDynAsset.h"
#include "zContextObject.h"

struct xEnt;

struct zContextObjectPoleSwingAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 poleID;
    F32 length;
};

class zContextObjectPoleSwing : public zContextObject
{
public:
#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

    zContextObjectPoleSwing(zContextObjectPoleSwingAsset* theAsset);

    void Setup();
    bool CanMount(zCommonPlayer* player, MountData& mountData, F32& priority) const;
    bool Mount(zCommonPlayer* player, MountData& mountData, F32 dt);
    zContextObjectType GetType() const { return ezContextObjectType_PoleSwing; }

protected:
    xEnt* pole;
    F32 length;
    zContextObjectPoleSwingAsset* asset;
};

#endif
