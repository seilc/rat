#include "xCommon.h"
#include "xModelWarp.h"

#include "xFXCameraTexture.h"
#include "xDebugTweak.h"
#include "xModel.h"

#include "decomp.h"

namespace {

    struct render_context
    {
        xMat4x3 uvmat;
        S32 flags;
        xBox box;
        xFColor color;
        RpAtomic* atomic;
        RpAtomic* original_atomic;
        RpAtomicCallBackRender original_render_callback;
    };

    static const S32 MAX_ACTIVE = 8;

    namespace tweak {

        xFColor default_color = { 1, 1, 1, 1 };

#if DEBUG || RELEASE
        namespace debug {

            S32 render_texture = -1;
            S32 render_bound = -1;
            S32 render_rect = -1;
            bool force_default_color = false;
            
        }
#endif

        void add_tweaks()
        {
            const char* prefix = "FX|Model Warp|";
            xAUTOTWEAK(prefix, "default_color \1r", &default_color.r, 0.0f, 2.0f, NULL, NULL, 0, false);
            xAUTOTWEAK(prefix, "default_color \2g", &default_color.g, 0.0f, 2.0f, NULL, NULL, 0, false);
            xAUTOTWEAK(prefix, "default_color \3b", &default_color.b, 0.0f, 2.0f, NULL, NULL, 0, false);
            xAUTOTWEAK(prefix, "default_color a", &default_color.a, 0.0f, 2.0f, NULL, NULL, 0, false);
#if DEBUG || RELEASE
            xAUTOTWEAK(prefix, "debug::render_texture", &debug::render_texture, -1, MAX_ACTIVE, NULL, NULL, 0, true);
            xAUTOTWEAK(prefix, "debug::render_bound", &debug::render_bound, -1, MAX_ACTIVE, NULL, NULL, 0, true);
            xAUTOTWEAK(prefix, "debug::render_rect", &debug::render_rect, -1, MAX_ACTIVE, NULL, NULL, 0, true);
            xAUTOTWEAK_BOOL(prefix, "debug::force_default_color", &debug::force_default_color, NULL, NULL, 0, true);
#endif
        }

    }

    xFXCameraTexture camtex;
    render_context render_contexts[MAX_ACTIVE];
    S32 render_contexts_size = 0;

    render_context* get_render_context(RpAtomic* atomic)
    {
        render_context* context = render_contexts, *end_context = context + render_contexts_size;
        for (; context != end_context; context++) {
            if (context->original_atomic == atomic)
                return context;
        }
        return NULL;
    }

    void get_screen_rect(xFRect& rect, const xVec3& center, F32 radius); // UNUSED
    void get_screen_rect(xFRect& rect, const xMat4x3& mat, const xFRect& irect); // UNUSED
    void get_screen_rect(xFRect& rect, const xMat4x3& mat, const xBox& box); // UNUSED

    void set_texture(RpAtomic* atomic, RwTexture* texture)
    {
        RpGeometry* geom = RpAtomicGetGeometry(atomic); xASSERT_L(170, geom != 0);
        xASSERT_L(171, RpGeometryGetNumMaterials(geom) == 1);

        RpMaterial* material = RpGeometryGetMaterial(geom, 0); xASSERT_L(172, material != 0);

        RpMaterialSetTexture(material, texture);
    }

    void destroy_render_context(render_context& context)
    {
        RpAtomicSetRenderCallBack(context.original_atomic, context.original_render_callback);
    }

    void sphere_map_normals(xVec2* uv, S32 verts_size, const xVec3* norm); // UNUSED
    void sphere_map_offsets(xVec2* uv, S32 verts_size, const xVec3* vert, const xVec3& center); // UNUSED

    RpAtomic* render_callback(RpAtomic*)
    {
        return NULL;
    }

    U8 write_screen(xFRect& scr_rect, const xFColor& color); // UNUSED
}

void xModelWarpSceneEnter()
{
    camtex.create(512, 256, false, 32, 32);
    camtex.set_background(g_CLEAR);

    render_contexts_size = 0;

    tweak::add_tweaks();

#if DEBUG
    U32 test_atomic_id = xStrHash("00_test_torus");
    RpAtomic* test_atomic = xModelFindAtomic(test_atomic_id, NULL);
    if (test_atomic) {
        xModelWarpSphereMapUVs(test_atomic, false);
        xModelWarpSetEffect(test_atomic, NULL, NULL);
    }
#endif
}

void xModelWarpSceneExit()
{
    render_context* context = render_contexts, *end_context = context + render_contexts_size;
    for (; context != end_context; context++)
        destroy_render_context(*context);
    render_contexts_size = 0;

    camtex.destroy();
}

void xModelWarpSetEffect(RpAtomic* atomic, const xBox* box, const xFColor* color)
{
    xASSERT_L(624, render_contexts_size < MAX_ACTIVE);

    if (render_contexts_size >= MAX_ACTIVE) return;

    render_context* old_context = get_render_context(atomic);
    xASSERT_L(628, old_context == 0);
    if (old_context != 0) return;

    render_context& context = render_contexts[render_contexts_size];
    render_contexts_size++;
    context.flags = 0;
    
    RpGeometry* geom = RpAtomicGetGeometry(atomic);
    RpMaterial* material = RpGeometryGetMaterial(geom, 0);

    context.uvmat = g_I3;
    context.original_atomic = atomic;
    context.atomic = atomic;

    context.original_render_callback = RpAtomicGetRenderCallBack(atomic);
    xASSERT_L(651, context.original_render_callback != render_callback);
    RpAtomicSetRenderCallBack(atomic, render_callback);

    set_texture(context.atomic, camtex.texture);

    if (box) {
        context.flags |= 0x1;
        context.box = *box;
    }

    if (color)
        context.color = *color;
    else
        context.color = tweak::default_color;
}

DECOMP_FORCEACTIVE("context != 0")

void xModelWarpSphereMapUVsByNormals(xVec2* uv, S32 verts_size, const xVec3* norm)
{
    xVec2* end_uv = uv + verts_size;
    for (; uv != end_uv; uv++, norm++) {
        uv->x = xasin(norm->x) * (1/PI) + 0.5f;
        uv->y = -(xasin(norm->y) * (1/PI) + 0.5f);
    }
}

void xModelWarpSphereMapUVsByOffsets(xVec2* uv, S32 verts_size, const xVec3* vert, const xVec3& center)
{
    xVec2* end_uv = uv + verts_size;
    for (; uv != end_uv; uv++, vert++) {
        xVec3 offset = *vert;
        offset -= center;
        offset.up_normalize();

        uv->x = xasin(offset.x) * (1/PI) + 0.5f;
        uv->y = -(xasin(offset.y) * (1/PI) + 0.5f);
    }
}

void xModelWarpSphereMapUVs(RpAtomic* atomic, bool use_normals)
{
    RpGeometry* geom = RpAtomicGetGeometry(atomic);
    S32 verts_size = RpGeometryGetNumVertices(geom);
    RpMorphTarget* mt = RpGeometryGetMorphTarget(geom, 0);

    RpGeometryLock(geom, rpGEOMETRYLOCKTEXCOORDS);

    const xVec3* vert = (const xVec3*)RpMorphTargetGetVertices(mt);
    const xVec3* norm = (const xVec3*)RpMorphTargetGetVertexNormals(mt);
    xVec2* uv = (xVec2*)RpGeometryGetVertexTexCoords(geom, rwTEXTURECOORDINATEINDEX0);
    const xVec3& center = *(const xVec3*)&atomic->boundingSphere.center;

    if (use_normals)
        xModelWarpSphereMapUVsByNormals(uv, verts_size, norm);
    else
        xModelWarpSphereMapUVsByOffsets(uv, verts_size, vert, center);

    RpGeometryUnlock(geom);
}
