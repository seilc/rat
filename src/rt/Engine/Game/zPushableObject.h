#ifndef ZPUSHABLEOBJECT_H
#define ZPUSHABLEOBJECT_H

#include "xDynAsset.h"
#include "xEnt.h"

class zPushableObject : public xEnt
{
public:
    struct Asset : xDynAsset
    {
        enum { VERSION = 1 };

        U32 modelID;
        S32 locationX;
        F32 locationY;
        S32 locationZ;
    };

#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif
    static F32 GetGridCellSize();
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    static F32 RoundToGrid(F32 pos);

    zPushableObject(const Asset& theAsset);

    bool CheckPos(const xVec3& destPos);
    void Move(xScene*, F32 dt, xEntFrame* frame);
    void Reset();
    void EntityHit(xEnt* hit);
    void HandleEvent(xBase* from, U32 toEvent, const F32*, xBase*, U32);

protected:
    const Asset& asset;
    F32 speed;
    xVec3 dir;
};

#endif
