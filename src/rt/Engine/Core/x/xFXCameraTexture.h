#ifndef XFXCAMERATEXTURE_H
#define XFXCAMERATEXTURE_H

#include "xColor.h"
#include "xMath3.h"
#include "xMath2.h"

#include <rwcore.h>
#include <rpworld.h>

class xFXCameraTexture
{
public:
    RwCamera* cam;
    RwRaster* raster;
    RwRaster* bgraster;
    RpWorld* world;
    RwTexture* texture;
    size_t vert_buffer_used;
    xColor bgcolor;

    bool create(S32 w, S32 h, bool need_zwrite, S32 bpp, S32 zbpp);
    void destroy();
    void begin_render(S32 clear_flags, U32 fbmsk_write, U32, F32 near_plane, F32 far_plane);
    void end_render();
    void render_background();
    void setup_subview(RwCamera* src_cam, F32 view_center_x, F32 view_center_y, F32 view_width, F32 view_height); // UNUSED
    void setup_subview(RwCamera* src_cam, const xFRect& src_rect); // UNUSED
    void setup_subview(RwCamera* src_cam, const xFRect& src_rect, const xFRect& dst_rect);
    void commit_view(); // UNUSED
    static void create_view_matrix(xMat4x3& view_mat, const xMat4x3& ltm, const xVec2& view_window, const xVec2& view_offset); // UNUSED
    void get_subview(xMat4x3& ltm, xVec2& view_window, xVec2& view_offset, RwCamera* src_cam, const xFRect& src_rect, const xFRect& dst_rect) const;
    void get_subview(xMat4x3& view_mat, RwCamera* src_cam, const xFRect& src_rect, const xFRect& dst_rect) const; // UNUSED
    void get_subview(xMat4x3& ltm, xVec2& view_window, xVec2& view_offset, RwCamera* src_cam, F32 view_center_x, F32 view_center_y, F32 view_width, F32 view_height) const; // UNUSED
    void get_subview(xMat4x3& view_mat, RwCamera* src_cam, F32 view_center_x, F32 view_center_y, F32 view_width, F32 view_height) const; // UNUSED
    void get_subview(xMat4x3& ltm, xVec2& view_window, xVec2& view_offset, RwCamera* src_cam, const xFRect& src_rect) const; // UNUSED
    void get_subview(xMat4x3& view_mat, RwCamera* src_cam, const xFRect& src_rect) const; // UNUSED
    void shrink_to_fit(S32 w, S32 h); // UNUSED

    bool valid()
    {
        return cam != 0;
    }

    void set_background(xColor color)
    {
        bgraster = NULL;
        bgcolor = color;
    }

    void set_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 u, F32 v)
    {
        RwIm2DVertexSetScreenX(&vert, x);
        RwIm2DVertexSetScreenY(&vert, y);
        RwIm2DVertexSetScreenZ(&vert, 1.0f);
        RwIm2DVertexSetU(&vert, u, 1.0f/0.3f);
        RwIm2DVertexSetV(&vert, v, 1.0f/0.3f);
        RwIm2DVertexSetIntRGBA(&vert, 255, 255, 255, 255);
    }

private:
    RwCamera* oldcam;
    RpWorld* oldworld;
    F32 rcz;
    F32 w;
    F32 h;
    RwRaster* zraster;
    S32 max_tw;
    S32 max_th;
    F32 old_near_plane;
    F32 old_far_plane;
};

class xFXFastRaster
{
public:
    enum tcc_type
    {
        TCC_DEFAULT = -1,
        TCC_RGB = 0,
        TCC_RGBA
    };

    enum tfx_type
    {
        TFX_DEFAULT = -1,
        TFX_MODULATE = 0,
        TFX_DECAL,
        TFX_HIGHLIGHT,
        TFX_HIGHLIGHT2
    };

    enum filter_type
    {
        FILTER_NEAREST,
        FILTER_LINEAR,
        FILTER_DEFAULT = 1
    };

    enum blend_mode
    {
        BLEND_INVALID = -1,
        BLEND_AVERAGE,
        BLEND_AVERAGE_FIXED,
        BLEND_RAVERAGE,
        BLEND_RAVERAGE_FIXED,
        BLEND_ADDITIVE,
        BLEND_ADDITIVE_FIXED,
        BLEND_ADDITIVE_SCALE_FIXED,
        BLEND_COPY,
        BLEND_COPY_FIXED,
        BLEND_SUBTRACT,
        BLEND_SUBTRACT_FIXED,
        BLEND_SUBTRACT_SCALE_FIXED,
        MAX_BLEND
    };

    static bool clip_rect(xFRect& dst_rect, RwRaster* dst_raster);
    static bool clip_rect(xFRect& dst_rect, RwRaster* dst_raster, xFRect& src_rect);
    static bool clip_rect(RwRect& dst_rect, RwRaster* dst_raster); // UNUSED
    static bool clip_rect(RwRect& dst_rect, RwRaster* dst_raster, xFRect& src_rect); // UNUSED

    bool clip_rect(xFRect& dst_rect, xFRect& src_rect) const { return clip_rect(dst_rect, dst, src_rect); }

    void begin_render();
    void end_render();
    void set_source(RwRaster*, tcc_type, tfx_type, filter_type);
    void set_destination(RwRaster* dst, U32 fbmsk);
    void texflush(); // UNUSED
    void set_blend_mode(blend_mode mode, F32 fixed_alpha); // UNUSED
    void fill(xColor color); // UNUSED
    void blit_scaled(F32 u, F32 v, xColor color); // UNUSED
    void set_texa(S32 ta0, S32 aem, S32 ta1); // UNUSED
    void set_prmode(bool prmodecont, U32 prmode); // UNUSED
    void set_clamp(S32 wms, S32 wmt, S32 minu, S32 maxu, S32 minv, S32 maxv); // UNUSED
    void set_scissor(S32 scax0, S32 scax1, S32 scay0, S32 scay1); // UNUSED

private:
    RwRaster* src;
    RwRaster* dst;
    U32 fbmsk;
    U32 test;
    U32 old_alpha_1;
};

#endif
