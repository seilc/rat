#ifndef ZCHECKPOINT_H
#define ZCHECKPOINT_H

#include "xBase.h"
#include "xDynAsset.h"

struct pointer_asset;
struct tweak_info;
struct zEntTrigger;
struct zScene;

struct zCheckpointAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 number;
    U32 triggerID;
    U32 scriptID;
    U32 cameraID;
    U32 pointerMrIID;
    U32 pointerFrozoneID;
};

class zCheckpoint : private xBase
{
public:
    static void SceneInit(zScene* scene);
    static void SceneReset(zScene* scene);
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    U32 GetNumber() { return asset->number; }
    void Init(zCheckpointAsset* asset);
    void Reset();
    static void EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
    void Activate();
    static void TweakFinishEditCB(const tweak_info&);

private:
    static U32 currentActive;
    static U32 lastCheckpointNumber;

    zCheckpointAsset* asset;
    zEntTrigger* trigger;
    pointer_asset* pointerMrI;
    pointer_asset* pointerFrozone;
    xLinkAsset triggerLink;
};

#endif
