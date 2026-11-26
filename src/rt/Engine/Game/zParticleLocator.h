#ifndef ZPARTICLELOCATOR_H
#define ZPARTICLELOCATOR_H

#include "types.h"

struct tweak_info;
struct xEnt;
struct xMat4x3;
struct xSphere;
struct xVec3;
struct zParticleAsset;

class zParticleLocator
{
public:
    struct activity_data;
#if DEBUG || RELEASE
    struct debug_info;
#endif

    activity_data* activity;
    xEnt* ent;
#if DEBUG || RELEASE
    debug_info* debug;
#endif

    static void scene_enter();
    static void scene_exit();
    void create();
    void restart();
    bool activate(const zParticleAsset& asset, xEnt* attach_to);
    void deactivate();
    void setEmitFromEdge(bool edgeEmit);
    void update(F32 dt);
    void make_locations(void* data, S32 stride, S32 elements, S32 loc_offset);
    xMat4x3& get_frame() const;
    void get_center(xVec3& center) const;
    void get_bounding_sphere(xSphere& sphere) const;
    F32 get_height() const;
    F32 get_area() const;
    bool active() const { return activity != NULL; }

#if DEBUG || RELEASE
    void add_tweaks(const char* prefix, zParticleAsset& asset, xEnt* attach_to);
    void debug_render();
    void debug_init();
    void copy_asset(const zParticleAsset&); // UNUSED
    xEnt* get_attach_to() const;
    static void on_tweaked(const tweak_info& info);
    void on_tweaked();
    void refresh_attach_tweaks();
    void refresh_motion_tweaks();
    void refresh_volume_tweaks();
#endif
};

#endif
