#include "xCommon.h"
#include "zWater.h"

#include "containers.h"
#include "xDynAsset.h"
#include "xFile.h"
#include "xIni.h"
#include "xPad.h"
#include "zBase.h"
#include "zEvent.h"
#include "zGlobals.h"

#include "decomp.h"

struct zWaterBody::asset_type : xDynAsset
{
    enum { VERSION = 1 };

    // flags
    enum
    {
        FLAG_VISIBLE = 0x1,
        FLAG_NO_OVERFILL = 0x2
    };

    U32 flags;
    U32 motion_type;
    U32 body;
    U32 facade_refract;
    U32 facade_reflect;
    U32 light_dir;

    static const char* type_name() { return "Effect:water_body"; }
};

namespace {

struct motion_config
{
    char name[64];
    U32 hash_id;
    S32 motion_type;
    U8 active;
    F32 lod_dist_min;
    F32 lod_dist_interval;
    S32 shimmer;
    S32 refract_glow;
    U32 random_seed;
    F32 gravity;
    F32 wind_speed;
    F32 wind_dir;
    F32 scale_speed;
    F32 phil_const;
    F32 wave_scale;
    F32 convergence;
    F32 choppiness;
    F32 max_height;
    struct
    {
        F32 color_mul_red;
        F32 color_mul_green;
        F32 color_mul_blue;
        F32 light_ambient;
        F32 light_diffuse;
        F32 light_transmissive;
        F32 reflectance_min;
        F32 reflectance_max;
        F32 reflect_intensity;
        F32 reflect_magnitude;
        F32 refract_min;
        F32 refract_index;
    } shading;
};

struct compare_ini_values
{
    inline S32 operator()(const char* v1, const xIniValue& v2); // UNUSED
};

#if DEBUG || RELEASE
namespace tweak {
void add_tweaks(); // UNUSED
} // namespace tweak
#endif

S32 find_motion_config(U32 id); // UNUSED
void WaterSkyDomeSimpleObjCB(xBase* from, xBase* to, U32 event, const F32* argf, xBase* argw, U32 argwi); // UNUSED
void banish_entity(xEnt* ent); // UNUSED
void stay_hidden_dammit(xEnt* ent); // UNUSED
void no_really_stay_hidden_dammit(xBase*, xBase* to, U32, const F32*, xBase*, U32); // UNUSED
void hide_entities(xEnt** ents, S32 ents_size); // UNUSED
const xIniValue* get_value(const char* name, const xIniValue* values, S32 values_size); // UNUSED
void read_value(F32& v, const char* name, const xIniValue* values, S32 values_size); // UNUSED
void read_value(S32& v, const char* name, const xIniValue* values, S32 values_size); // UNUSED
void read_value(U32& v, const char* name, const xIniValue* values, S32 values_size); // UNUSED
void load_motion_config(motion_config& cfg, const xIniValue* values, S32 values_size); // UNUSED

#if DEBUG || RELEASE
void on_change_motion_config(const tweak_info&); // UNUSED
void on_randomize_motion_config(const tweak_info&); // UNUSED
void on_change_shading(const tweak_info& info); // UNUSED
void add_motion_tweaks(motion_config& cfg); // UNUSED
#endif

} // namespace

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "FX|Water|\001Globals|",
    "default_light_yaw",
    "default_light_pitch",
    "zWater.cpp",
    "motions_size > 0",
    "%s",
    "lod_dist_min",
    "lod_dist_interval",
    "shimmer",
    "refract_glow",
    "random_seed",
    "gravity",
    "wind_speed",
    "wind_dir",
    "scale_speed",
    "phil_const",
    "wave_scale",
    "convergence",
    "choppiness",
    "max_height",
    "shading.color_mul_red",
    "shading.color_mul_green",
    "shading.color_mul_blue",
    "shading.light_ambient",
    "shading.light_diffuse",
    "shading.light_transmissive",
    "shading.reflectance_min",
    "shading.reflectance_max",
    "shading.reflect_intensity",
    "shading.reflect_magnitude",
    "shading.refract_min",
    "shading.refract_index",
    "FX|Water|\001Motion Types|%s|",
    "Randomize Seed",
    "Go!",
    "cfg. lod_dist_min",
    "cfg. lod_dist_interval",
    "cfg. shimmer",
    "cfg. refract_glow",
    "cfg. random_seed",
    "cfg. gravity",
    "cfg. wind_speed",
    "cfg. wind_dir",
    "cfg. scale_speed",
    "cfg. phil_const",
    "cfg. wave_scale",
    "cfg. convergence",
    "cfg. choppiness",
    "cfg. max_height",
    "cfg. shading.color_mul_red",
    "cfg. shading.color_mul_green",
    "cfg. shading.color_mul_blue",
    "cfg. shading.light_ambient",
    "cfg. shading.light_diffuse",
    "cfg. shading.light_transmissive",
    "cfg. shading.reflectance_min",
    "cfg. shading.reflectance_max",
    "cfg. shading.reflect_intensity",
    "cfg. shading.reflect_magnitude",
    "cfg. shading.refract_min",
    "cfg. shading.refract_index"
)
#endif

void zWaterBody::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(590, asset, asset_size, asset_type, asset_type::type_name());
    ((zWaterBody&)data).load((asset_type&)asset);
}

void zWaterBody::show()
{
    visible = true;
}

void zWaterBody::hide()
{
    visible = false;
}

void zWaterBody::load(const asset_type& a)
{
    asset = (asset_type*)&a;
    xBaseInit(this, asset);
    baseType = eBaseTypeWaterBody;
    eventFunc = cb_dispatch;
    if (linkCount) link = (xLinkAsset*)(asset + 1);

    body_id = -1;
    visible = asset->flags & asset_type::FLAG_VISIBLE;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "facade_refract",
    "facade_reflect",
    "ent != 0",
    "Water Body %s: Can't find %s %s",
    "ent->baseFlags & k_XBASE_IS_ENTITY",
    "Water Body %s: Invalid %s %s",
    "ent->model != 0",
    "Water Body %s: Have %s %s, but no model.",
    "ID:0x%x",
    "depth < 10",
    "Water Body %s: Probable recursive %s group %s",
    "*child != 0",
    "Water Body %s can't find %s element %s in group %s",
    "base->baseFlags & k_XBASE_IS_ENTITY",
    "Water Body %s: Invalid %s element %s",
    "((xEnt*)base)->model != 0",
    "Water Body %s: Have %s element %s, but no model.",
    "base != 0",
    "Water Body %s can't find %s element %s",
    "size > 0",
    "Water Body %s: Have no %s elements",
    DEBUG ? "xMemMgr.h" : 0
)
#endif

U32 zWaterBody::get_bodyID() { return asset->body; }

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("pointer")
#endif

void zWaterBody::cb_dispatch(xBase*, xBase* to, U32 event, const F32*, xBase*, U32)
{
    xASSERT_L(896, to != 0);
    zWaterBody& e = *(zWaterBody*)to;
    xASSERT_L(898, e.baseType == eBaseTypeWaterBody);

    switch (event) {
    case eEventVisible:
    case eEventVisibilityCullOff:
        e.show();
        break;
    case eEventInvisible:
    case eEventVisibilityCullOn:
        e.hide();
        break;
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "FX|Water|%s|",
    "visible",
    "no_overfill",
    "motion_cfg_id",
    "debug-> light_dir_yaw",
    "debug-> light_dir_pitch"
)
#endif
