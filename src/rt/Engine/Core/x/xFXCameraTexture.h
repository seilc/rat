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

#ifndef NON_MATCHING
    void __unused();
#endif

    void setup_subview(RwCamera* src_cam, const xFRect& src_rect, const xFRect& dst_rect);
    void get_subview(xMat4x3& ltm, xVec2& view_window, xVec2& view_offset, RwCamera* src_cam, const xFRect& src_rect, const xFRect& dst_rect) const;

    bool valid()
    {
        return cam != 0;
    }

    void set_background(xColor color);

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

    static bool clip_rect(xFRect& dst_rect, RwRaster* dst_raster);
    static bool clip_rect(xFRect& dst_rect, RwRaster* dst_raster, xFRect& src_rect);

    void begin_render();
    void end_render();
    void set_source(RwRaster*, tcc_type, tfx_type, filter_type);
    void set_destination(RwRaster* dst, U32 fbmsk);

private:
    RwRaster* src;
    RwRaster* dst;
    U32 fbmsk;
    U32 test;
    U32 old_alpha_1;
};

#endif
