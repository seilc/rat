#ifndef ZCONTEXTOBJECTTIGHTROPE_H
#define ZCONTEXTOBJECTTIGHTROPE_H

#include "xDynAsset.h"
#include "zContextObject.h"

class xNurbs;
struct xEnt;

struct zContextObjectTightropeAsset : xDynAsset
{
    enum { VERSION = 3 };

    U32 curveID;
    U32 objectID;
    U8 fallBelow;
    U8 flags;

    bool NoRender() const { return flags & 0x1; }
};

class zContextObjectTightrope : public zContextObject
{
public:
#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

    void Draw() const;
#if DEBUG || RELEASE
    void DebugDraw() const; // UNUSED
#endif

    zContextObjectTightrope(zContextObjectTightropeAsset* theAsset);

    void Setup();
    bool Advance(F32 dist, F32& u, bool forward) const;
    bool CanMount(zCommonPlayer* player, MountData& mountData, F32& priority) const;
    bool Mount(zCommonPlayer* player, MountData& mountData, F32);
    zContextObjectType GetType() const { return ezContextObjectType_Tightrope; }
    U8 ClearChkby() const;
    void RestoreChkby(U8 prevValue) const;

protected:
    xBox bound;
    const xNurbs* curve;
    xEnt* object;
    zContextObjectTightropeAsset* asset;
};

#endif
