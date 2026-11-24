#ifndef ZFLAMEEMITTER_H
#define ZFLAMEEMITTER_H

#include "xBase.h"
#include "zFire.h"

struct xDynAsset;
struct xEnt;
struct xScene;
struct tweak_info;

struct zFlameEmitter : public xBase
{
    struct asset_type;

#if DEBUG || RELEASE
    struct debug_data
    {
        char flame_heat[128];
    };

    static zFlameEmitter* hilighted;
#endif

    const asset_type* asset;
    xMat4x3 mat;

    void load(const asset_type& a);
#if DEBUG || RELEASE
    void debug_reset();
#endif
    void reset();
    void hide();
    void show();
    void spawn();
    void kill(F32);
    void kill_flames(F32 fuel_remaining); // UNUSED
    void update(F32 dt);
    static void update(xEnt* ent, xScene*, F32 dt); // UNUSED
    void render();
    static void render(xEnt* ent); // UNUSED
    static void load(xBase& data, xDynAsset& asset, size_t asset_size);
    void get_bounding_box(xBox& box) const;
    static void scene_enter();
    static void scene_exit();
    static void setup_all();
    static void update_all(F32 dt);
    static void render_all();
    static void fire_reset();
    void refresh_mat();
    void refresh_properties();
    void setup();
    void destroy();
    void create_flames(S32 flags);
    void destroy_flames();
    void dump_flames();
    void start_flames_sound();
    void end_flames_sound();
    static void cb_dispatch(xBase*, xBase* to, U32 event, const F32* argf, xBase*, U32);
    void debug_init();
    void debug_render();
#if DEBUG || RELEASE
    void add_tweaks();
    static void enable_hilight(const tweak_info& info);
    static void disable_hilight(const tweak_info&);
    static void on_change_visible(const tweak_info& info);
    static void on_move(const tweak_info& info);
    static void tweak_rtod(const tweak_info&, void* value);
    static void tweak_dtor(const tweak_info&, void* value);
    void update_heat_stats();
    static void on_update_flame_heat(const tweak_info& info);
    static void on_refresh_props(const tweak_info& info);
#endif

private:
    S32 flags;
    zFire::handle* flames;
    zFire::property_set props;
    S32 flames_size;
    S32 flames_allocated;
    iSndGroupHandle sound_group;
    xSndHandle sound_playing;
    F32 oscil;
    xVec3 driverOffset;
#if DEBUG || RELEASE
    debug_data* debug;
#endif
};

#endif
