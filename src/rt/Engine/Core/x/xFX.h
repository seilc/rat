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

    bool empty() const
    {
        return (act == NULL) ? true : act->joints.empty();
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

public:
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

    void set_config(const config_type* cfg);
    void insert(const xVec3& loc, const xVec3& norm, F32 scale, F32 alpha, S32 flags);
    void copy_parameters(xFXRibbon* ribbon) const;

    static RwRaster* get_raster(const char* name);

private:
#if DEBUG || RELEASE
    struct debug_info;
#endif

    activity_data* act;
    const config_type* cfg;

#if DEBUG || RELEASE
    debug_info* debug;
#endif
};

void xFX_SceneEnter(RpWorld* world);
void xFX_SceneExit(RpWorld*);
void xFXUpdate(F32 dt);
void xFXanimUV2PSetTexture(RwTexture* texture);

#endif
