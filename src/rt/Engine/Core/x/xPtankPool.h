#ifndef XPTANKPOOL_H
#define XPTANKPOOL_H

#include "xMath2.h"
#include "xMath3.h"
#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>

enum ptank_group_type
{
    PGT_COLOR_MAT,
    PGT_COLOR_MAT_UV2,
    PGT_POS_COLOR_SIZE,
    PGT_POS_COLOR_SIZE_UV2,
    PGT_POS_COLOR_SIZE_ROT,
    PGT_POS_COLOR_SIZE_ROT_UV2,
    MAX_PGT
};

struct ptank_pool
{
    struct render_state
    {
        RwTexture* texture;
        U32 src_blend;
        U32 dst_blend;
        S32 flags;
    };

    render_state rs;
    U32 order_group;
    S32 order_index;
    size_t used;
    RpAtomic* ptank;

    bool valid() const;
    void reset();
    void flush();
    void unlock_block();

    void grab_block(ptank_group_type type);
};

struct ptank_pool__pos_color_size_uv2 : ptank_pool
{
    xVec3* pos;
    xColor* color;
    xVec2* size;
    xVec2* uv;
    RwInt32 stride_pos;
    RwInt32 stride_color;
    RwInt32 stride_size;
    RwInt32 stride_uv;

    void next()
    {
        if (used >= 80) {
            if (valid()) {
                unlock_block();
            }
            grab_block(PGT_POS_COLOR_SIZE_UV2);
            if (!valid()) {
                return;
            }
            lock_block();
        } else {
            pos = (xVec3*)((U8*)pos + stride_pos);
            color = (xColor*)((U8*)color + stride_color);
            size = (xVec2*)((U8*)size + stride_size);
            uv = (xVec2*)((U8*)uv + stride_uv);
        }
        
        used++;
    }

    void lock_block()
    {
        used = 0;

        RpPTankLockStruct ls_color, ls_pos, ls_size, ls_uv;
        RpPTankAtomicLock(ptank, &ls_pos, rpPTANKLFLAGPOSITION, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_color, rpPTANKLFLAGCOLOR, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_size, rpPTANKLFLAGSIZE, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_uv, rpPTANKLFLAGVTX2TEXCOORDS, rpPTANKLOCKWRITE);

        pos = (xVec3*)ls_pos.data;
        color = (xColor*)ls_color.data;
        size = (xVec2*)ls_size.data;
        uv = (xVec2*)ls_uv.data;
        stride_pos = ls_pos.stride;
        stride_color = ls_color.stride;
        stride_size = ls_size.stride;
        stride_uv = ls_uv.stride;
    }
};

#endif
