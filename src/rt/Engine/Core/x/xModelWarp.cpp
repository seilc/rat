#include "xModelWarp.h"

#include "xFXCameraTexture.h"
#include "xDebugTweak.h"
#include "xModel.h"

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

#ifdef DEBUGRELEASE
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
            xAUTOTWEAKRANGE(prefix, "default_color \1r", &default_color.r, 0.0f, 2.0f, NULL, NULL, 0, false);
            xAUTOTWEAKRANGE(prefix, "default_color \2g", &default_color.g, 0.0f, 2.0f, NULL, NULL, 0, false);
            xAUTOTWEAKRANGE(prefix, "default_color \3b", &default_color.b, 0.0f, 2.0f, NULL, NULL, 0, false);
            xAUTOTWEAKRANGE(prefix, "default_color a", &default_color.a, 0.0f, 2.0f, NULL, NULL, 0, false);
#ifdef DEBUGRELEASE
            xAUTOTWEAKRANGE(prefix, "debug::render_texture", &debug::render_texture, -1, MAX_ACTIVE, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "debug::render_bound", &debug::render_bound, -1, MAX_ACTIVE, NULL, NULL, 0, true);
            xAUTOTWEAKRANGE(prefix, "debug::render_rect", &debug::render_rect, -1, MAX_ACTIVE, NULL, NULL, 0, true);
            xAUTOTWEAKBOOL(prefix, "debug::force_default_color", &debug::force_default_color, NULL, NULL, 0, true);
#endif
        }

    }

    xFXCameraTexture camtex;
    render_context render_contexts[MAX_ACTIVE];
    S32 render_contexts_size = 0;

    render_context* get_render_context(RpAtomic* atomic)
    {
        render_context* context = render_contexts;
        render_context* end_context = context + render_contexts_size;
        while (context != end_context) {
            if (context->original_atomic == atomic) {
                return context;
            }
            context++;
        }
        return NULL;
    }

    void set_texture(RpAtomic* atomic, RwTexture* texture)
    {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        xASSERT(170, geom != 0);
        xASSERT(171, RpGeometryGetNumMaterials(geom) == 1);

        RpMaterial* material = RpGeometryGetMaterial(geom, 0);
        xASSERT(172, material != 0);

        RpMaterialSetTexture(material, texture);
    }

    void destroy_render_context(render_context& context)
    {
        RpAtomicSetRenderCallBack(context.original_atomic, context.original_render_callback);
    }

    RpAtomic* render_callback(RpAtomic*)
    {
        return NULL;
    }

}

void xModelWarpSceneEnter()
{
    camtex.create(512, 256, false, 32, 32);
    camtex.set_background(g_CLEAR);

    render_contexts_size = 0;

    tweak::add_tweaks();

#ifdef DEBUG
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
    render_context* context = render_contexts;
    render_context* end_context = context + render_contexts_size;
    while (context != end_context) {
        destroy_render_context(*context);
        context++;
    }
    render_contexts_size = 0;

    camtex.destroy();
}

void xModelWarpSetEffect(RpAtomic* atomic, const xBox* box, const xFColor* color)
{
    xASSERT(624, render_contexts_size < MAX_ACTIVE);

    if (render_contexts_size >= MAX_ACTIVE) return;

    render_context* old_context = get_render_context(atomic);
    xASSERT(628, old_context == 0);
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
    xASSERT(651, context.original_render_callback != render_callback);
    RpAtomicSetRenderCallBack(atomic, render_callback);

    set_texture(context.atomic, camtex.texture);

    if (box) {
        context.flags |= 0x1;
        context.box = *box;
    }

    if (color) {
        context.color = *color;
    } else {
        context.color = tweak::default_color;
    }
}

#ifndef NON_MATCHING
void xModelWarpRemoveEffect(RpAtomic* atomic)
{
    render_context* context = get_render_context(atomic);
    xASSERT(0, context != 0);
}
#endif

void xModelWarpSphereMapUVsByNormals(xVec2* uv, S32 verts_size, const xVec3* norm)
{
    xVec2* end_uv = uv + verts_size;
    while (uv != end_uv) {
        uv->x = xasin(norm->x) * (1/PI) + 0.5f;
        uv->y = -(xasin(norm->y) * (1/PI) + 0.5f);
        uv++, norm++;
    }
}

void xModelWarpSphereMapUVsByOffsets(xVec2* uv, S32 verts_size, const xVec3* vert, const xVec3& center)
{
    xVec2* end_uv = uv + verts_size;
    while (uv != end_uv) {
        xVec3 offset = *vert;
        offset -= center;
        offset.up_normalize();

        uv->x = xasin(offset.x) * (1/PI) + 0.5f;
        uv->y = -(xasin(offset.y) * (1/PI) + 0.5f);

        uv++, vert++;
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

    if (use_normals) {
        xModelWarpSphereMapUVsByNormals(uv, verts_size, norm);
    } else {
        xModelWarpSphereMapUVsByOffsets(uv, verts_size, vert, center);
    }

    RpGeometryUnlock(geom);
}
