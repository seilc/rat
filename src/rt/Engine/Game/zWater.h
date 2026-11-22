#ifndef ZWATER_H
#define ZWATER_H

#include "xBase.h"

struct xDynAsset;
struct xEnt;
struct xVec3;
struct xWaterShading;
struct tweak_info;

class zWaterBody : private xBase
{
public:
    struct asset_type;
    struct debug_info;

    S32 motion_type;

private:
    asset_type* asset;
    xEnt* body_ent;
    xEnt** refract_ents;
    S32 refract_ents_size;
    xEnt** reflect_ents;
    S32 reflect_ents_size;
    S32 body_id;
    bool visible;
    S32 motion_cfg_id;
#if DEBUG || RELEASE
    debug_info* debug;
#endif

public:
    static void load(xBase& data, xDynAsset& asset, size_t asset_size);
    static void setup_all(); // UNUSED
    static void reset_all(); // UNUSED
    static void update_all(F32 dt); // UNUSED
    void show();
    void hide();
    void load(const asset_type& a);
    void setup(); // UNUSED
    void reset(); // UNUSED
    void update(F32); // UNUSED
    xEnt* load_ent(U32 id, const char* type) const; // UNUSED
    S32 count_ents(xBase* base, const char* type, S32 depth) const; // UNUSED
    xEnt** load_ents(xEnt** ents, xBase* base) const; // UNUSED
    xEnt** load_ents(S32& size, U32 id, const char* type) const; // UNUSED
    U32 get_bodyID();
    void convert_light_dir(xVec3& dir, F32 yaw, F32 pitch) const; // UNUSED
    void load_light_dir(F32& yaw, F32& pitch, U32 pointer_id) const; // UNUSED
    void get_shading(xWaterShading& shading) const; // UNUSED
    static void cb_dispatch(xBase*, xBase* to, U32 event, const F32*, xBase*, U32);
#if DEBUG || RELEASE
    void debug_init(); // UNUSED
    void add_tweaks(); // UNUSED
    void refresh_shading(); // UNUSED
    static void on_change_visible(const tweak_info& info); // UNUSED
    static void on_change_settings(const tweak_info& info); // UNUSED
    static void on_change_light_dir(const tweak_info& info); // UNUSED
    static void on_change_motion_type(const tweak_info& info); // UNUSED
#endif
};

void zWaterStartup(); // UNUSED
void zWaterSceneEnter(); // UNUSED
void zWaterSceneExit(); // UNUSED

#endif
