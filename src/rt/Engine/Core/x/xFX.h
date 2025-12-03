#ifndef XFX_H
#define XFX_H

#include "xMath3.h"
#include "xColor.h"
#include "containers.h"

#include <rwcore.h>
#include <rpworld.h>

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

    F32 texture_offset;
    F32 texture_increment_scale;

    joint_iterator begin() const;

    joint_iterator end() const
    {
        return (act == NULL) ? joint_queue::dummy_iterator() : act->joints.end();
    }

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

#ifdef DEBUGRELEASE
    struct debug_info
    {
        char tweak_prefix[128];
        char name[32];
        curve_node curve[32];
        U32 texture;
        S32 active_joints;
        config_type cfg;
        void(*tweaked)(xFXRibbon&, void*);
        void* context;
        struct
        {
            bool active;
            bool repeat;
            bool bind_to_player;
            F32 time;
            xVec3 center;
            xVec3 offset;
            F32 radius_inner;
            F32 radius_outer;
            F32 freq;
            F32 duration;
            F32 orient;
            F32 random_orient_increase;
        } spiral;
    };
#endif

    activity_data* act;
    const config_type* cfg;

#ifdef DEBUGRELEASE
    debug_info* debug;
#endif
};

void xFX_SceneEnter(RpWorld* world);
void xFX_SceneExit(RpWorld*);
void xFXUpdate(F32 dt);
void xFXanimUV2PSetTexture(RwTexture* texture);

#endif
