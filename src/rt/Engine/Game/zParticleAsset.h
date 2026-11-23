#ifndef ZPARTICLEASSET_H
#define ZPARTICLEASSET_H

#include "xDynAsset.h"
#include "xVec3.h"

struct zParticleAsset : xDynAsset
{
    enum
    {
        SYSTEM_DUMMY,
        SYSTEM_WATERFALL,
        SYSTEM_WATERFALL_MIST,
        SYSTEM_WATERFALL_SPLASH,
        SYSTEM_SNOW,
        SYSTEM_RAIN,
        SYSTEM_EARTH_DUST,
        SYSTEM_POP,
        SYSTEM_BURROW_KICKUP,
        SYSTEM_PUFFER,
        MAX_SYSTEM
    };

    struct attach_fixed_data
    {
    };

    struct attach_entity_data
    {
        U32 entity;
        U8 bone;
        U8 pad1;
        U8 pad2;
        U8 pad3;
    };

    struct attach_entity_tag_data
    {
        U32 entity;
        xVec3 tag;
    };

    union attach_data
    {
        attach_fixed_data fixed;
        attach_entity_data entity;
        attach_entity_tag_data entity_tag;
    };

    struct motion_none_data
    {
    };

    struct motion_spiral_data
    {
        U8 flags;
        U8 points;
        U8 pad1;
        U8 pad2;
        F32 radius_inner;
        F32 radius_outer;
        F32 duration;
        F32 frequency;
    };

    union motion_data
    {
        motion_none_data none;
        motion_spiral_data spiral;
    };

    struct volume_point_data
    {
    };

    struct volume_sphere_data
    {
        F32 radius;
    };

    struct volume_circle_data
    {
        F32 radius;
        F32 arc_length;
        F32 height;
    };

    struct volume_line_data
    {
        U8 flags;
        U8 pad1;
        U8 pad2;
        U8 pad3;
        F32 radius;
        F32 length;
    };

    struct volume_model_data
    {
        U8 flags;
        U8 exclude;
        U8 pad1;
        U8 pad2;
        F32 expand;
    };

    struct volume_box_data
    {
        xVec3 dimensions;
    };

    union volume_data
    {
        volume_point_data point;
        volume_sphere_data sphere;
        volume_circle_data circle;
        volume_line_data line;
        volume_model_data model;
        volume_box_data box;
    };

    U8 flags;
    U8 attach_flags;
    U8 motion_flags;
    U8 volume_flags;
    F32 rate;
    U32 texture;
    F32 emitDistance;
    U8 attach_type;
    U8 motion_type;
    U8 volume_type;
    U8 system_type;
    xVec3 location;
    struct
    {
        F32 yaw;
        F32 pitch;
        F32 roll;
    } axis;
    attach_data attach;
    motion_data motion;
    volume_data volume;
};

#endif
