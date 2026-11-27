#ifndef ZCAMPRESET_H
#define ZCAMPRESET_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xVec3.h"

struct xMat4x3;

struct zCamPreset : xBase
{
    struct asset_type : xDynAsset
    {
        static const char* type_name() { return "camera:preset"; }

        enum { VERSION = 1 };

        // flags
        enum
        {
            FLAG_APPLY_ON_SCENE_PREPARE = 0x1,
            FLAG_APPLY_ON_RESET = 0x2,
            FLAG_RELATIVE = 0x4
        };

        U32 flags;
        U32 checkpoint;
        xVec3 loc;
        struct
        {
            F32 yaw;
            F32 pitch;
            F32 roll;
        } dir;
    };

#if DEBUG || RELEASE
    struct debug_info;
#endif

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);
    static void update_all(F32 dt);
    static void apply_on_scene_prepare();
    static void apply_on_reset();
    static zCamPreset* find_preset(bool scene_prepare);
    void load(const asset_type& a);
    static void setup(); // UNUSED
    static void reset(); // UNUSED
    bool get_checkpoint(xVec3& loc, F32& yaw) const;
    bool at_checkpoint() const;
    bool applicable(bool scene_prepare) const; // UNUSED
    F32 distance2(const xVec3& to) const;
    void apply();
    bool get_relative(xVec3& offset, F32& yaw) const;
    void get_matrix(xMat4x3& mat) const;
    void event_handler(xBase*, U32 event, const F32*, xBase*, U32);
    static void event_handler_callback(xBase* from, xBase* to, U32 event, const F32* argf, xBase* argw, U32 argwi);
#if DEBUG || RELEASE
    void debug_init();
    void add_tweaks();
#else
    void debug_init() {}
#endif

    asset_type* asset;

private:
    S32 flags;
#if DEBUG || RELEASE
    debug_info* debug;
#endif
};

#endif
