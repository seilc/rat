#ifndef ZSCREENWARP_H
#define ZSCREENWARP_H

#include "xBase.h"

struct tweak_info;
struct xDynAsset;
struct zScreenWarpAsset;

class zScreenWarp : private xBase
{
    // flags
    enum zScreenWarpFlags
    {
        SW_FLAG_ENABLED = 0x1,
        SW_FLAG_RUNNING = 0x2
    };

public:
#if DEBUG || RELEASE
    bool renderDebugSphere;
#endif

    static void SceneEnter();
    static void ResetAll();
    static void UpdateAll(F32 dt);
#if DEBUG || RELEASE
    static void RenderAll();
#else
    static void RenderAll() {}
#endif
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

private:
    zScreenWarp* next;
    zScreenWarpAsset* asset;
    F32 timer;
    U32 flags;

#if DEBUG || RELEASE
    void addDebugTweaks();
    static void on_enable(const tweak_info& info);
#else
    void addDebugTweaks() {}
#endif
    void Reset();
    void Enable(bool bEnable);
    void Update(F32 dt);
    void UpdateState();
#if DEBUG || RELEASE
    void Render();
#endif
    static void EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
};

#endif
