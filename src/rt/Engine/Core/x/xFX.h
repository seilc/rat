#ifndef XFX_H
#define XFX_H

#include "xMath3.h"
#include "xColor.h"
#include "containers.h"
#include "xDebug.h"

#include <rwcore.h>
#include <rpworld.h>

struct tweak_info;

struct xFXRing
{
    U32 texture;
    F32 lifetime;
    xVec3 pos;
    F32 time;
    F32 ring_radius;
    F32 ring_radius_delta;
    F32 ring_tilt;
    F32 ring_tilt_delta;
    F32 ring_height;
    F32 ring_height_delta;
    xColor ring_color;
    U16 ring_segs;
    U8 u_repeat;
    U8 v_repeat;
    xFXRing** parent;
    RwRaster* rast_ring;
};

extern RpAtomicCallBackRender gAtomicRenderCallBack;
extern U8 ReportUVProb;

#if DEBUG || RELEASE
void xFXDebugSetSurfaceFlags(U32 flags);
U32 xFXDebugGetSurfaceFlags();
#endif
void xFXInit();
void xFXDefaultRenderCallback(RpAtomic* model); // UNUSED
void xFXRingInit();
RwRaster* xFXRingFindRWRaster(U32 aid_texture);
xFXRing* xFXRingCreate(const xVec3* pos, const xFXRing* params);
void xFXRingRender(); // UNUSED
void xFX_SceneEnter(RpWorld* world);
void xFX_SceneExit(RpWorld*);
void xFXUpdate(F32 dt);
void xFXExit(); // UNUSED
U32 xFXEnvMap(RpMaterial* mat, U32 envmapID, F32 coeff);
U32 xFXBumpMap(RpMaterial* mat, U32 bumpmapID, F32 coeff);
RpAtomic* AtomicDisableMatFX(RpAtomic* atomic);
RpMaterial* MaterialSetEnvMap2(RpMaterial* material, void* data); // UNUSED
void xFXAtomicUpdateFrame(RpAtomic* atomic, RwFrame* frame); // UNUSED
RpAtomic* xFXAtomicEnvMapSetup(RpAtomic* atomic, U32 envmapID, F32 shininess); // UNUSED
RpAtomic* xFXAtomicBumpEnvSetup(RpAtomic* atomic, U32 bumpmapID, F32 bumpiness, U32 envmapID, F32 shininess); // UNUSED
void ResetMaterialEffects(RpClump* clump); // UNUSED

extern F32 xFXanimUVRotMat0[2];
extern F32 xFXanimUVRotMat1[2];
extern F32 xFXanimUVTrans[2];
extern F32 xFXanimUVScale[2];

void xFXanimUVSetTranslation(const RwV2d* trans); // UNUSED
void xFXanimUVSetTranslation(const xVec3* trans);
void xFXanimUVSetScale(const RwV2d* scale); // UNUSED
void xFXanimUVSetScale(const xVec3* scale);
void xFXanimUVSetAngle(F32 angle);
void xFXanimUV2PSetTranslation(const RwV2d* trans); // UNUSED
void xFXanimUV2PSetTranslation(const xVec3* trans);
void xFXanimUV2PSetScale(const RwV2d* scale); // UNUSED
void xFXanimUV2PSetScale(const xVec3* scale);
void xFXanimUV2PSetAngle(F32 angle);
void xFXanimUV2PSetTexture(RwTexture* texture);
RpAtomic* xFXanimUVAtomicSetup(RpAtomic* atomic, bool dualPass);
RpWorld* xFXanimUVWorldSectorSetup(RpWorld* world); // UNUSED
S32 xFXanimUVCreate();
void xFXanimUVDestroy(); // UNUSED

#define MAX_ACTIVITIES 32

struct xFXRibbon
{
public:
    struct joint_data
    {
        S32 flags;
        U32 born;
        xVec3 loc;
        xVec3 norm;
        F32 orient;
        F32 scale;
        F32 alpha;
    };

    typedef tier_queue<joint_data> joint_queue;
    typedef joint_queue::iterator joint_iterator;

    struct curve_node
    {
        F32 time;
        xColor color;
        F32 scale;
    };

    struct config_type
    {
        F32 life_time;
        U32 blend_src;
        U32 blend_dst;
        F32 pivot;
        S32 flags;
        const curve_node* curve;
        S32 curve_size;
        RwRaster* raster;
    };

    F32 texture_offset;
    F32 texture_increment_scale;

    static void scene_enter();
    static void update_all(F32 dt);
    static void render_all();
    static void load_default_config(config_type& cfg);
    static RwRaster* get_raster(const char* name);
    static RwRaster* get_raster(U32 id);

    void create(S32 disableAutomaticDebugTweaks);
    void clear();
    void set_config(const config_type* cfg);
    void insert(const xVec3& loc, const xVec3& norm, F32 scale, F32 alpha, S32 flags);
    void insert(const xVec3& loc, F32 orient, F32 scale, F32 alpha, S32 flags); // UNUSED
    void activate();
    void deactivate();
    void update(F32 dt);
    void render();
    void copy_parameters(xFXRibbon* ribbon) const;

    bool valid() const { return cfg && cfg->curve_size > 0 && cfg->life_time > 0.0f; }

    bool visible() const { return act != NULL; }

    bool empty() const { return (act == NULL) ? true : act->joints.empty(); }
    joint_iterator begin() const;
    joint_iterator end() const { return (act == NULL) ? joint_queue::dummy_iterator() : act->joints.end(); }

    F32 get_age(const joint_data& joint) const
    {
        xASSERT_L(231, !empty());
        return 0.001f * (act->mtime - joint.born);
    }

    bool need_update() const;

#if DEBUG || RELEASE
    typedef void (*tweaked_callback)(xFXRibbon&, void*);

    void add_tweaks(const char* debug_name, const char* tweak_prefix, tweaked_callback tweaked, void* context);
    void add_spiral_tweaks();
    void update_curve_tweaks();
    bool debug_tweakhelp_enabled() const;
    void create_test_curve(S32 size);
    void move_spiral_to_player();
#endif

private:
    struct activity_data
    {
        xFXRibbon* owner;
        activity_data** position;
        joint_queue joints;
        S32 curve_index;
        F32 ilife;
        U32 mtime;
        U32 mlife;
    };

#if DEBUG || RELEASE
    struct debug_info;
#endif

    void get_normal(xVec3& norm, const xVec3& dir, F32 orient);
    void refresh_joint(joint_data& joint, const joint_iterator& it);
    void eval_joint(const joint_data& joint, xColor& color, F32& width);
    void render_strip(RwIm3DVertex* verts, joint_iterator first, size_t size);

    static S32 compare_ribbons(const void* e1, const void* e2);
    static void sort_active_ribbons();

#if DEBUG || RELEASE
    static void debug_scene_enter();
    void debug_init();
    void debug_update(F32 dt);
    void debug_refresh_config();
    void debug_update_curve();
    bool debug_need_update() const;
#else
    static void debug_scene_enter() {}
    void debug_update(F32) {}
    void debug_refresh_config() {}
    bool debug_need_update() const { return false; }
#endif

#if DEBUG || RELEASE
    static void on_start_spiral(const tweak_info& info);
    static void on_activate_spiral(const tweak_info& info);
    static void on_change_curve(const tweak_info& info);
    static void on_change_config(const tweak_info& info);
    static void on_change_node_time(const tweak_info& info);
    static void on_change_node_value(const tweak_info& info);
    static void on_change_texture(const tweak_info& info);
#endif

    static tier_queue_allocator joint_alloc;
    static activity_data* activities[MAX_ACTIVITIES];
    static S32 activities_used;
    static bool need_sort;

    activity_data* act;
    const config_type* cfg;
#if DEBUG || RELEASE
    debug_info* debug;
#endif
};

inline bool xFXRibbon::need_update() const { return visible() || debug_need_update(); }

#endif
