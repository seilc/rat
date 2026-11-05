#include "xFXHighDynamicRange.h"

#include "xDebug.h"
#include "xDebugTweak.h"
#include "xColor.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xRenderState.h"
#include "xutil.h"
#include "iDebug.h"
#include "iDraw.h"
#include "iSystem.h"
#include "zRenderState.h"

#include <stdio.h>

#define MAX_TEXDATA 8

bool HDR_brightening;

namespace {

    inline void refresh_blur_packets()
    {
    }

#if DEBUG || RELEASE
    void on_refresh_blur(const tweak_info&)
    {
        refresh_blur_packets();
    }
#endif

    namespace tweak {

        xTWEAKABLE S32 background_glow_default = 0;
        xTWEAKABLE S32 background_darken_default = 80;
        xTWEAKABLE S32 clamp_glow = 0;
        xTWEAKABLE S32 preserve_glow = 0;
        F32 overbrighten = 2.0f;
        F32 overbrighten_decay = 0.5f;
        xTWEAKABLE F32 overbrighten_diminish = 0.3f;
        xTWEAKABLE F32 blur_offset = 0.0f;
        xTWEAKABLE F32 recombine = 0.2f;
        xTWEAKABLE F32 random_blur_offset = 0.0f;
        S32 downsamples = 4;
        xTWEAKABLE bool render = true;
        bool blur_faster = true;
        F32 colorize_red = 1.0f;
        F32 colorize_green = 1.0f;
        F32 colorize_blue = 1.0f;
        F32 colorize_alpha = 1.0f;
        xTWEAKABLE F32 hdr_offset = 0.5f;

        namespace blit {

            xTWEAKABLE S32 stress_test = 0;
            xTWEAKABLE F32 stress_test_u = 0.0f;
            xTWEAKABLE F32 stress_test_v = 0.0f;
            xTWEAKABLE S32 optimal_dst_width = 32;
            xTWEAKABLE S32 optimal_src_width = 64;
            xTWEAKABLE S32 strips = 0;
            xTWEAKABLE F32 strip_width = 0.0f;

        }

        namespace debug {

            xTWEAKABLE bool force_background_to_default = false;
            xTWEAKABLE F32 test_fade_time = 1.0f;
            xTWEAKABLE S32 test_fade_glow = 10;
            xTWEAKABLE S32 test_fade_darken = 80;
            xTWEAKABLE bool render_blur_texture0 = false;
            xTWEAKABLE bool skip_screen_render = false;
            xTWEAKABLE F32 test_uoffset = 0.0f;
            xTWEAKABLE F32 test_voffset = 0.0f;

        }

#if DEBUG || RELEASE
        void on_test_fade(const tweak_info&)
        {
            xFXHighDynamicRangeSetBackground(debug::test_fade_glow, debug::test_fade_darken, debug::test_fade_time);
        }

        void on_reset_background(const tweak_info&)
        {
            xFXHighDynamicRangeSetBackground(debug::test_fade_glow, debug::test_fade_darken, 1.0f);
        }

        const tweak_callback cb_test_fade = { on_test_fade };
        const tweak_callback cb_reset_background = { on_reset_background };
        const tweak_callback cb_refresh_blur = { on_refresh_blur };

        void add_tweaks()
        {
            const char* prefix = "FX|HDR|\1Globals|";

            xAUTOTWEAKRANGE(prefix, "background_glow_default", &background_glow_default, 0, 255, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "background_darken_default", &background_darken_default, 0, 255, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "clamp_glow", &clamp_glow, 0, 63, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "preserve_glow", &preserve_glow, 0, 255, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "overbrighten", &overbrighten, 0.0f, 100.0f, &cb_refresh_blur, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "overbrighten_decay", &overbrighten_decay, 0.0f, 1.0f, &cb_refresh_blur, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "overbrighten_diminish", &overbrighten_diminish, 0.0f, 100.0f, &cb_refresh_blur, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "blur_offset", &blur_offset, -100.0f, 100.0f, &cb_refresh_blur, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "recombine", &recombine, 0.0f, 1.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "random_blur_offset", &random_blur_offset, -100.0f, 100.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "downsamples", &downsamples, 1, 5, NULL, NULL, 0, true);
            xAUTOTWEAKBOOL(prefix, "render", &render, NULL, NULL, 0, true);
            xAUTOTWEAKBOOL(prefix, "blur_faster", &blur_faster, &cb_refresh_blur, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "colorize_red", &colorize_red, 0.0f, 2.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "colorize_green", &colorize_green, 0.0f, 2.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "colorize_blue", &colorize_blue, 0.0f, 2.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "colorize_alpha", &colorize_alpha, 0.0f, 2.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "hdr_offset", &hdr_offset, -1000000.0f, 1000000.0f, NULL, NULL, 0, true);

            xAUTOTWEAKRANGE(prefix, "blit::stress_test", &blit::stress_test, 0, 100000, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "blit::stress_test_u", &blit::stress_test_u, -1000000000.0f, 1000000000.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "blit::stress_test_v", &blit::stress_test_v, -1000000000.0f, 1000000000.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "blit::optimal_dst_width", &blit::optimal_dst_width, 1, 1000, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "blit::optimal_src_width", &blit::optimal_src_width, 1, 1000, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "blit::strips", &blit::strips, 0, 0, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "blit::strip_width", &blit::strip_width, 0.0f, 0.0f, NULL, NULL, 0, true);

            xAUTOTWEAKBOOL(prefix, "debug::force_background_to_default", &debug::force_background_to_default, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "debug::test_fade_time", &debug::test_fade_time, 0.0f, 100.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "debug::test_fade_glow", &debug::test_fade_glow, 0, 255, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "debug::test_fade_darken", &debug::test_fade_darken, 0, 255, NULL, NULL, 0, true);
            xAUTOTWEAKBOOL(prefix, "debug::render_blur_texture0", &debug::render_blur_texture0, NULL, NULL, 0, true);
            xAUTOTWEAKBOOL(prefix, "debug::skip_screen_render", &debug::skip_screen_render, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "debug::test_uoffset", &debug::test_uoffset, -100000.0f, 100000.0f, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "debug::test_voffset", &debug::test_voffset, -100000.0f, 100000.0f, NULL, NULL, 0, true);

            xAUTOTWEAKMESSAGE(prefix, "debug|test_fade", "Go!", &cb_test_fade, NULL, 0, false);
            xAUTOTWEAKMESSAGE(prefix, "debug|reset_background", "Go!", &cb_reset_background, NULL, 0, false);
        }
#else
        void add_tweaks() {}
#endif

    }

#if DEBUG || RELEASE
    xFRect downs[6] = {
        { 0.0f, 0.0f, 256.0f, 240.0f },
        { 0.0f, 240.0f, 128.0f, 120.0f },
        { 128.0f, 240.0f, 64.0f, 60.0f },
        { 128.0f, 300.0f, 32.0f, 30.0f },
        { 128.0f, 330.0f, 16.0f, 15.0f },
        { 128.0f, 345.0f, 8.0f, 7.5f }
    };
#endif

    bool enabled = true;
    S32 background_glow_normal;
    S32 background_darken_normal;

    struct interpolator
    {
        S32 value;
        F32 start;
        F32 end;
        F32 current;
        F32 t;
        F32 trate;

        void reset(S32 value)
        {
            start = current = end = this->value = value;
            t = trate = 0.0f;
        }

        void go(S32 value, F32 time)
        {
            if (time <= 0.0f || value == this->value) {
                reset(value);
                return;
            }

            start = current;
            end = value;
            t = 0.0f;
            trate = 1.0f / time;
        }

        void update(F32 dt)
        {
            if (trate == 0.0f) return;

            t += trate * dt;
            if (t >= 1.0f) {
                start = current = end;
                t = 0.0f;
                trate = 0.0f;
            } else {
                F32 s = t;
                current = 0.5f + ((1.0f - s) * start + s * end);
            }

            value = (S32)(0.5f + current);
        }
    };

    interpolator background_glow;
    interpolator background_darken;

}

void xFXHighDynamicRangeSetColorize(F32 r, F32 g, F32 b, F32 alpha)
{
    tweak::colorize_red = r;
    tweak::colorize_green = g;
    tweak::colorize_blue = b;
    tweak::colorize_alpha = alpha;
}

namespace {

    inline void set_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 u, F32 v, xColor color, F32 rcz, F32 nsz)
    {
        RwIm2DVertexSetScreenX(&vert, x);
        RwIm2DVertexSetScreenY(&vert, y);
        RwIm2DVertexSetScreenZ(&vert, nsz);

        RwIm2DVertexSetU(&vert, u, rcz);
        RwIm2DVertexSetV(&vert, v, rcz);
        RwIm2DVertexSetIntRGBA(&vert, color.r, color.g, color.b, color.a);
    }

}

S32 hdr_dump_efb;
xTWEAKABLE S32 hdr_use_raster = 1;
void* texData[MAX_TEXDATA];
U16 sizes[9][2] = {
    { 256, 240 },
    { 128, 128 },
    { 64, 64 },
    { 32, 32 },
    { 16, 16 },
    { 8, 8 },
    { 4, 4 },
    { 2, 2 },
    { 1, 1 }
};

namespace {

#if DEBUG || RELEASE
    void add_global_tweaks()
    {
        const char* prefix = "FX|HDR|\1Globals|";

        xAUTOTWEAKBOOL(prefix, "enabled", &enabled, NULL, NULL, 0, true);
    }
#else
    inline void add_global_tweaks() {}
#endif

}

void xFXHighDynamicRangeInit()
{
    enabled = true;

    background_glow_normal = tweak::background_glow_default;
    background_darken_normal = tweak::background_darken_default;

    xFXHighDynamicRangeReset();

    add_global_tweaks();
    tweak::add_tweaks();
}

void xFXHighDynamicRangeSceneEnter()
{
}

void xFXHighDynamicRangeSceneExit()
{
}

void xFXHighDynamicRangeReset()
{
    enabled = true;
    background_glow.reset(background_glow_normal);
    background_darken.reset(background_darken_normal);
}

void xFXHighDynamicRangeStartRender()
{
    if (!tweak::render || !enabled)
        return;

    HDR_brightening = false;

    U32 mask = iDrawGetFBMSK();
    iDrawSetFBMSK(0xFFFFFF, 0xFFFFFFFF, true);

    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, 0);

    _rwDlRenderStateSetZCompLoc(FALSE);

    zRenderState(eRenderStateFill);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwReal rcz = 1.0f / RwCameraGetNearClipPlane(RwCameraGetCurrentCamera());
    RwReal nsz = RwIm2DGetNearScreenZ();

    RwIm2DVertex verts[4];
    xColor clearcolor;

    background_glow.value = 0;

    clearcolor.r = 0;
    clearcolor.g = 0;
    clearcolor.b = 0;
    clearcolor.a = MAX(1, background_glow.value);

    set_vert(verts[0], 0.0f, 0.0f, 0.0f, 0.0f, clearcolor, rcz, nsz);
    set_vert(verts[1], 0.0f, FB_YRES, 0.0f, 0.0f, clearcolor, rcz, nsz);
    set_vert(verts[2], FB_XRES, 0.0f, 0.0f, 0.0f, clearcolor, rcz, nsz);
    set_vert(verts[3], FB_XRES, FB_YRES, 0.0f, 0.0f, clearcolor, rcz, nsz);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, ARRAY_LENGTH(verts));

    iDrawSetFBMSK(mask, 0xFFFFFFFF, true);

    zRenderState(eRenderStateHDRBeginRender);

    U8 alpha_glow = background_glow.value;
    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, alpha_glow);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, alpha_glow);

    _rwDlRenderStateSetZCompLoc(FALSE);
    GXSetZCompLoc(GX_FALSE);
}

static void set_sample_offsets(U16 curSizeX, U16 curSizeY)
{
    F32 isizex = 1.0f / curSizeX;
    F32 isizey = 1.0f / curSizeY;

    F32 hdr_offset = tweak::hdr_offset;
    F32 offsets[4][2] = {
        { 3.0f * hdr_offset, hdr_offset },
        { -3.0f * hdr_offset, -hdr_offset },
        { hdr_offset, -3.0f * hdr_offset },
        { -hdr_offset, 3.0f * hdr_offset }
    };

    F32 matrix[2][4] = { { 1.0f, 0.0f, 0.0f, 0.0f },
                         { 0.0f, 1.0f, 0.0f, 0.0f } };

    matrix[0][3] = offsets[0][0] * isizex; matrix[1][3] = offsets[0][1] * isizey;
    GXLoadTexMtxImm(matrix, GX_TEXMTX0, GX_MTX2x4);

    matrix[0][3] = offsets[1][0] * isizex; matrix[1][3] = offsets[1][1] * isizey;
    GXLoadTexMtxImm(matrix, GX_TEXMTX1, GX_MTX2x4);

    matrix[0][3] = offsets[2][0] * isizex; matrix[1][3] = offsets[2][1] * isizey;
    GXLoadTexMtxImm(matrix, GX_TEXMTX2, GX_MTX2x4);

    matrix[0][3] = offsets[3][0] * isizex; matrix[1][3] = offsets[3][1] * isizey;
    GXLoadTexMtxImm(matrix, GX_TEXMTX3, GX_MTX2x4);
}

static void debug_render_card(void* tex, F32 x, F32 y, F32 w, F32 h)
{
    GXTexObj tobj ATTRIBUTE_ALIGN(16);
    GXInitTexObj(&tobj, tex, w, h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&tobj, GX_TEXMAP0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
    GXPosition2f32(x, y);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition2f32(x, y + h);
    GXTexCoord2f32(0.0f, 1.0f);
    GXPosition2f32(x + w, y);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition2f32(x + w, y + h);
    GXTexCoord2f32(1.0f, 1.0f);
    GXEnd();
}

void xFXHighDynamicRangeRender(RwCamera*)
{
    S32 i;
    U16 sX, sY;

    if (!tweak::render || !enabled)
        return;

    GXDrawDone();
    if (hdr_dump_efb)
        iDebugDumpEFB("efb1");

    zRenderState(eRenderStateFill);

    sizes[0][1] = FB_YRES / 2;

    iXFBreset();

    for (i = 0; i < MAX_TEXDATA; i++) {
        sX = sizes[i][0];
        sY = sizes[i][1];
        texData[i] = iXFBalloc(sX * sY * 4);
    }

    U16 curSizeX = FB_XRES / 2, curSizeY = FB_YRES / 2;
    GXSetTexCopySrc(0, 0, FB_XRES, FB_YRES);
    GXSetTexCopyDst(curSizeX, curSizeY, GX_TF_RGBA8, GX_TRUE);
    GXCopyTex(texData[0], GX_FALSE);

    void* frame;
    void* unaligned_frame = xMEMPUSHTEMP_L(curSizeX * curSizeY * 4 + 31, 0, 1791);
    frame = (void*)(((U32)unaligned_frame + 31) & ~31);
    DCInvalidateRange(unaligned_frame, curSizeX * curSizeY * 4 + 31);

    GXSetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);
    GXSetTexCopySrc(0, 0, curSizeX, curSizeY);
    GXSetTexCopyDst(curSizeX, curSizeY, GX_TF_RGBA8, GX_FALSE);
    GXCopyTex(frame, GX_FALSE);

    GXDrawDone();
    GXPixModeSync();

    GXSetCopyFilter(renderObject.aa, renderObject.sample_pattern, GX_TRUE, renderObject.vfilter);

    GXInvalidateTexAll();

    Mtx44 mProjection;
    MTXOrtho(mProjection, 0.0f, FB_YRES, 0.0f, FB_XRES, 0.0f, 1.0f);
    GXSetProjection(mProjection, GX_ORTHOGRAPHIC);

    GXSetCurrentMtx(GX_PNMTX0);

    Mtx44 mIdentity;
    MTXIdentity(mIdentity);
    GXLoadPosMtxImm(mIdentity, GX_PNMTX0);

    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GXSetNumChans(0);
    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetCullMode(GX_CULL_NONE);

    GXSetViewport(0.0f, 0.0f, FB_XRES, FB_YRES, 0.0f, 1.0f);
    GXSetScissor(0, 0, FB_XRES, FB_YRES);

    GXSetAlphaUpdate(GX_TRUE);
    GXPokeAlphaRead(GX_READ_NONE);

    GXTexObj texObject ATTRIBUTE_ALIGN(16);
    GXTexObj texObject2 ATTRIBUTE_ALIGN(16);

    for (S32 i = 0; i < 5; i++) {
        GXSetTevColorIn((GXTevStageID)i, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_CPREV);
        GXSetTevColorOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn((GXTevStageID)i, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_APREV);
        GXSetTevAlphaOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevOrder((GXTevStageID)i, (GXTexCoordID)i, GX_TEXMAP0, GX_COLOR_NULL);
    }

    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);

    GXColor clamp_color;
    clamp_color.r = tweak::clamp_glow;
    clamp_color.g = tweak::clamp_glow;
    clamp_color.b = tweak::clamp_glow;
    clamp_color.a = 255;
    GXSetTevColor(GX_TEVREG1, clamp_color);
    GXSetTevColorIn(GX_TEVSTAGE5, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE5, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE5, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
    GXSetTevAlphaOp(GX_TEVSTAGE5, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL);

    const S32 scale_back = 0.5f + 16384.0f / (128.0f - tweak::clamp_glow);

    GXColor colorize;
    colorize.r = tweak::colorize_red * scale_back;
    colorize.g = tweak::colorize_green * scale_back;
    colorize.b = tweak::colorize_blue * scale_back;
    colorize.a = tweak::colorize_alpha * scale_back;
    GXSetTevColor(GX_TEVREG2, colorize);
    GXSetTevColorIn(GX_TEVSTAGE6, GX_CC_ZERO, GX_CC_CPREV, GX_CC_C2, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE6, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE6, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
    GXSetTevAlphaOp(GX_TEVSTAGE6, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL);

    GXSetTevColorIn(GX_TEVSTAGE7, GX_CC_ZERO, GX_CC_CPREV, GX_CC_APREV, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE7, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE7, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
    GXSetTevAlphaOp(GX_TEVSTAGE7, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL);

    GXSetNumTexGens(5);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
    GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1);
    GXSetTexCoordGen(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX2);
    GXSetTexCoordGen(GX_TEXCOORD4, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX3);

    GXPixModeSync();

    GXInitTexObj(&texObject, texData[0], curSizeX, curSizeY, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

    F32 overbrighten = tweak::overbrighten;
    F32 alpha, resample_alpha;
    S32 resamples;
    GXColor scaleColor;

    for (S32 i = 1; i < tweak::downsamples; i++) {
        GXSetNumTevStages(i == 1 ? (U8)8 : (U8)5);

        GXLoadTexObj(&texObject, GX_TEXMAP0);

        resamples = 5;
        resample_alpha = 1.0f / resamples;
        alpha = overbrighten * (1.0f - resample_alpha) + resample_alpha;
        overbrighten *= tweak::overbrighten_decay;
        overbrighten -= tweak::overbrighten_diminish;
        if (overbrighten < 0.0f)
            overbrighten = 0.0f;

        alpha = 0.2f * (255.0f * alpha) + 0.5f;
        alpha = CLAMP(alpha, 0.0f, 255.0f);

        U8 ualpha = alpha;
        scaleColor.r = ualpha;
        scaleColor.g = ualpha;
        scaleColor.b = ualpha;
        scaleColor.a = ualpha;
        GXSetTevColor(GX_TEVREG0, scaleColor);

        curSizeX = sizes[i][0];
        curSizeY = sizes[i][1];

        set_sample_offsets(curSizeX, curSizeY);

        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
        GXPosition2f32(0.0f, 0.0f);
        GXTexCoord2f32(0.0f, 0.0f);
        GXPosition2f32(0.0f, curSizeY);
        GXTexCoord2f32(0.0f, 1.0f);
        GXPosition2f32(curSizeX, 0.0f);
        GXTexCoord2f32(1.0f, 0.0f);
        GXPosition2f32(curSizeX, curSizeY);
        GXTexCoord2f32(1.0f, 1.0f);
        GXEnd();

        GXSetTexCopySrc(0, 0, curSizeX, curSizeY);
        GXSetTexCopyDst(curSizeX, curSizeY, GX_TF_RGBA8, GX_FALSE);
        GXCopyTex(texData[i], GX_FALSE);

        if (hdr_dump_efb)
            iprintf("flushed tex %d:  %dx%d\n", i, curSizeX, curSizeY);

        GXDrawDone();
        GXPixModeSync();
        GXInvalidateTexAll();

#if DEBUG || RELEASE
        if (tweak::debug::render_blur_texture0)
            debug_render_card(texData[i], downs[i].x, downs[i].y, downs[i].w, downs[i].h);
#endif

        GXInitTexObj(&texObject, texData[i], curSizeX, curSizeY, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    }

    if (hdr_dump_efb)
        iDebugDumpEFB("efb2");

    F32 oy = 0.0f;
    S32 final_raster = 0;
    if (tweak::recombine > 0.0f) {
        F32 recombine = 255.0f * tweak::recombine;

        for (i = tweak::downsamples - 1; i > hdr_use_raster; i--) {
            F32 recombine_alpha = recombine / (tweak::downsamples - i);
            recombine_alpha = CLAMP(recombine_alpha, 0.0f, 255.0f);

            GXColor rwalpha;
            rwalpha.r = recombine_alpha;
            rwalpha.g = recombine_alpha;
            rwalpha.b = recombine_alpha;
            rwalpha.a = recombine_alpha;
            GXSetTevColor(GX_TEVREG0, rwalpha);

            xASSERT_ONCE_L(2144, i-1 > 0);

            curSizeX = sizes[i][0];
            curSizeY = sizes[i][1];
            GXInitTexObj(&texObject, texData[i], curSizeX, curSizeY, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
            GXLoadTexObj(&texObject, GX_TEXMAP0);

            curSizeX = sizes[i - 1][0];
            curSizeY = sizes[i - 1][1];
            GXInitTexObj(&texObject2, texData[i - 1], curSizeX, curSizeY, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
            GXLoadTexObj(&texObject2, GX_TEXMAP1);

            GXSetNumTexGens(2);

            GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
            GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

            GXSetNumTevStages(2);

            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_A0, GX_CC_ZERO);
            GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_A0, GX_CA_ZERO);
            GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);

            GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_A0, GX_CC_TEXC, GX_CC_CPREV);
            GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A0, GX_CA_TEXA, GX_CA_APREV);
            GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);

            if (curSizeY) {
            }

            GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
            GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
            GXPosition2f32(0.0f, oy);
            GXTexCoord2f32(0.0f, 0.0f);
            GXPosition2f32(0.0f, curSizeY + oy);
            GXTexCoord2f32(0.0f, 1.0f);
            GXPosition2f32(curSizeX, oy);
            GXTexCoord2f32(1.0f, 0.0f);
            GXPosition2f32(curSizeX, curSizeY + oy);
            GXTexCoord2f32(1.0f, 1.0f);
            GXEnd();

            GXSetTexCopySrc(0, oy, curSizeX, curSizeY);
            GXSetTexCopyDst(curSizeX, curSizeY, GX_TF_RGBA8, GX_FALSE);
            GXCopyTex(texData[i - 1], GX_FALSE);
            GXPixModeSync();
            GXInvalidateTexAll();

            if (hdr_dump_efb) {
                char name[32];
                sprintf(name, "efb_r%d", i - 1);
                iDebugDumpEFB(name);
            }
        }

        final_raster = hdr_use_raster;
    } else {
        final_raster = tweak::downsamples - 1;
    }

    GXSetViewport(0.0f, 0.0f, FB_XRES, FB_YRES, 0.0f, 1.0f);
    GXSetScissor(0, 0, FB_XRES, FB_YRES);

    GXTexObj screenBack ATTRIBUTE_ALIGN(16);
    GXInitTexObj(&screenBack, frame, FB_XRES / 2, FB_YRES / 2, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&screenBack, GX_TEXMAP0);

    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    GXSetNumTevStages(1);

    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
    GXPosition2f32(0.0f, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition2f32(0.0f, FB_YRES / 2);
    GXTexCoord2f32(0.0f, 1.0f);
    GXPosition2f32(FB_XRES / 2, 0.0f);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition2f32(FB_XRES / 2, FB_YRES / 2);
    GXTexCoord2f32(1.0f, 1.0f);
    GXEnd();

    GXSetAlphaUpdate(GX_TRUE);
    GXInitTexObj(&texObject, texData[final_raster], sizes[final_raster][0], sizes[final_raster][1], GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&texObject, GX_TEXMAP0);

    if (!tweak::debug::skip_screen_render) {
        U8 darken_alpha = 255 - background_darken.value;
        scaleColor.r = darken_alpha;
        scaleColor.g = darken_alpha;
        scaleColor.b = darken_alpha;
        scaleColor.a = darken_alpha;
        GXSetTevColor(GX_TEVREG0, scaleColor);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);

        GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_SRCALPHA, GX_LO_SET);

        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
        GXPosition2f32(0.0f, 0.0f);
        GXTexCoord2f32(0.0f, 0.0f);
        GXPosition2f32(0.0f, FB_YRES);
        GXTexCoord2f32(0.0f, 1.0f);
        GXPosition2f32(FB_XRES, 0.0f);
        GXTexCoord2f32(1.0f, 0.0f);
        GXPosition2f32(FB_XRES, FB_YRES);
        GXTexCoord2f32(1.0f, 1.0f);
        GXEnd();
    }

    if (tweak::debug::render_blur_texture0)
        debug_render_card(texData[0], 20.0f, 20.0f, 20.0f + FB_XRES / 2, 20.0f + FB_YRES / 2);

    xMEMPOPTEMP(unaligned_frame);

    GXSetDither(GX_TRUE);

    if (hdr_dump_efb)
        iDebugDumpEFB("efb3");

    hdr_dump_efb = 0;
}

void xFXHighDynamicRangeRestoreSettings()
{
}

U32 xFXHighDynamicRangeBrightenPtank(RpAtomic*, bool bright, S32)
{
    if (!tweak::render)
        return 0;

    U8 old_alpha_discard_value_gc;
    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &old_alpha_discard_value_gc);

    if (bright) {
        iDrawSetFBMSK(0, 0xFFFFFFFF, true);
        RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, background_glow.value);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, background_glow.value);
        if (background_glow.value > 0) {
            _rwDlRenderStateSetZCompLoc(FALSE);
            GXSetZCompLoc(GX_FALSE);
        } else {
            _rwDlRenderStateSetZCompLoc(TRUE);
            GXSetZCompLoc(GX_TRUE);
        }
    } else {
        iDrawSetFBMSK(0xFF000000, 0xFFFFFFFF, true);
        RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, 0);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, 0);

        _rwDlRenderStateSetZCompLoc(TRUE);
        GXSetZCompLoc(GX_TRUE);
    }

    return old_alpha_discard_value_gc;
}

void xFXHighDynamicRangeRestorePtank(RpAtomic*, U32 oldtest)
{
    if (!tweak::render)
        return;

    iDrawSetFBMSK(0, 0xFFFFFFFF, true);
    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, oldtest);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, oldtest);
    if ((U8)oldtest > 0) {
        _rwDlRenderStateSetZCompLoc(FALSE);
        GXSetZCompLoc(GX_FALSE);
    } else {
        _rwDlRenderStateSetZCompLoc(TRUE);
        GXSetZCompLoc(GX_TRUE);
    }
}

void xFXHighDynamicRangeTunePalette(RwRaster*, F32, bool)
{
}

void xFXHighDynamicRangeUpdate(F32 dt)
{
    background_glow.update(dt);
    background_darken.update(dt);

    if (tweak::debug::force_background_to_default) {
        background_glow_normal = tweak::background_glow_default;
        background_darken_normal = tweak::background_darken_default;
        background_glow.reset(background_glow_normal);
        background_darken.reset(background_darken_normal);
    }
}

S32 xFXHighDynamicRangeGetBackgroundGlow() { return background_glow.value; }

void xFXHighDynamicRangeSetConfiguration(const xFXHighDynamicRangeConfiguration& configuration)
{
    background_glow.value = configuration.glow;
    background_darken.value = configuration.darken;
    tweak::overbrighten = configuration.overbrighten;
    tweak::overbrighten_decay = configuration.overbrighten_decay;
    tweak::downsamples = configuration.downsamples;
    tweak::blur_faster = configuration.blur_faster;
    refresh_blur_packets();
}

void xFXHighDynamicRangeGetConfiguration(xFXHighDynamicRangeConfiguration& configuration)
{
    configuration.glow = background_glow.value;
    configuration.darken = background_darken.value;
    configuration.overbrighten = tweak::overbrighten;
    configuration.overbrighten_decay = tweak::overbrighten_decay;
    configuration.downsamples = tweak::downsamples;
    configuration.blur_faster = tweak::blur_faster;
}

void xFXHighDynamicRangeSetGlowClamp(S32, bool)
{
}

void xFXHighDynamicRangeResetGlowClamp()
{
}

void xFXHighDynamicRangeSetBackground(S32 glow, S32 darken, F32 time)
{
    if (glow < 0)
        glow = background_glow_normal;
    else if (glow > 255)
        glow = 255;
    background_glow.go(glow, time);

    if (darken < 0)
        darken = background_darken_normal;
    else if (darken > 255)
        darken = 255;
    background_darken.go(darken, time);
}

void xFXHighDynamicRangeSetBackgroundDefault(S32 glow, S32 darken, bool apply)
{
    background_glow_normal = CLAMP(glow, 0, 255);
    background_darken_normal = CLAMP(darken, 0, 255);

    if (apply) {
        background_glow.reset(background_glow_normal);
        background_darken.reset(background_darken_normal);
    }
}
