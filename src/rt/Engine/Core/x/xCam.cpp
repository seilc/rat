#include "xCam.h"

#include "iCamera.h"
#include "xCamTransition.h"
#include "xDebug.h"
#include "xDraw.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "containers.h"
#include "xScene.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zAutoTweak.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zPad.h"
#include "zScene.h"

#include <stdio.h>

#if DEBUG || RELEASE
bool gxCamShowDebugMessages;
#endif

static void do_transient_replacement(xCam*& curCam);

namespace {
#if DEBUG || RELEASE
    S32 blendDepth;
#endif

    namespace tweak {
#if DEBUG || RELEASE
        namespace debug {
            S32 super_sample_updates;
        }
#endif

        void add_tweaks(const char* prefix)
        {
            xAUTOTWEAK(prefix, "debug::super_sample_updates", &debug::super_sample_updates, 0, 100, NULL, NULL, 0, true);
        }
    }
}

void xCam::start()
{
    flags |= 0x1;
}

void xCam::stop()
{
    flags &= ~0x1;
}

void xCam::scene_enter()
{
    tweak::add_tweaks("Camera|\001Globals|");
}

void xCam::scene_exit()
{
    xTWEAK_REMOVE("Camera");
}

void xCam::refresh_mat(xMat4x3& mat, const xCamSpatialInfo& spatial)
{
    switch (coord_type) {
    case XCAM_COORD_CART:
        coord_to_world(mat.pos, spatial.coord.cart);
        break;
    case XCAM_COORD_CYLINDER:
        coord_to_world(mat.pos, spatial.coord.cylinder);
        break;
    case XCAM_COORD_SPHERE:
        coord_to_world(mat.pos, spatial.coord.sphere);
        break;
    default:
        xFAIL_FMT_L(102, "%s", "Uninitialized coordinate type.");
    }

    switch (orient_type) {
    case XCAM_ORIENT_QUAT:
        orient_to_world(mat, spatial.orient.quat);
        break;
    case XCAM_ORIENT_EULER:
        orient_to_world(mat, spatial.orient.euler);
        break;
    default:
        xFAIL_FMT_L(114, "%s", "Uninitialized orientation type.");
    }
}

void xCam::refresh_mat()
{
    refresh_mat(mat, spatial);
    if (flags & 0x80) {
        refresh_mat(coll_mat, coll_spatial);
    } else {
        coll_mat = mat;
        coll_spatial = spatial;
    }
}

void xCam::create()
{
    flags = 0;
    coord_type = XCAM_COORD_INVALID;
    orient_type = XCAM_ORIENT_INVALID;
    group = NULL;
    debug_init();
}

bool xCam::active()
{
    return flags & 0x1;
}

// UNUSED: speculative
bool xCam::blendingTo() const
{
    return blending() && blender->dst == this;
}

#if DEBUG || RELEASE
void xCam::debug_init()
{
    debug.flags = 0;

    static xColor colors[] = {
        g_RED, g_GREEN, g_BLUE, g_CYAN, g_MAGENTA, g_YELLOW, g_WHITE, g_MAROON, g_FORESTGREEN, g_NEON_GREEN, g_NEON_BLUE,
        g_PEACH, g_FUSCHIA, g_LAVENDER, g_MIDNIGHTBLUE, g_SEAGREEN, g_NEON_RED, g_PIMP_GOLD, g_ORANGE, g_KHAKI, g_GRAY40, g_PINK
    };

    static S32 last_color = 0;
    debug.color[0] = colors[last_color];
    last_color = (last_color + 1) % (S32)ARRAY_LENGTH(colors);
    debug.color[1] = colors[last_color];
    last_color = (last_color + 1) % (S32)ARRAY_LENGTH(colors);
    debug.color[2] = colors[last_color];
    last_color = (last_color + 1) % (S32)ARRAY_LENGTH(colors);
}

void xCam::debug_render()
{
    if (debug.flags & 0x1) {
        xDrawSetColor(debug.color[0]);
        xDrawSphere2(&mat.pos, 0.25f, 7);

        xDrawSetColor(debug.color[1]);
        F32 cone_radius = isin(0.5f * fov);
        xVec3 coneTip; coneTip.AddScale(mat.pos, mat.at, 1.0f);
        xDrawCone(mat.pos, 0.0f, coneTip, cone_radius, 10, 2);

        xDrawSetColor(debug.color[2]);
        switch (coord_type) {
        case XCAM_COORD_CYLINDER:
            xDrawSphere2(&spatial.coord.cylinder.origin, 0.25f, 7);
            break;
        case XCAM_COORD_SPHERE:
            xDrawSphere2(&spatial.coord.sphere.origin, 0.25f, 7);
        }
    }
}

void xCam::add_tweaks_common(const char* prefix)
{
    zAUTOTWEAK(prefix, "priority", &cfg_common.priority, 0, 254, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "blend_time", &cfg_common.blend_time, 0.0f, 1e9f, NULL, NULL, 2);

    zAUTOTWEAK(prefix, "\001info|\001location ", &mat.pos, 0, 0, NULL, NULL, 3);
    zAUTOTWEAK(prefix, "\001info|\002facing ", &mat.at, 0, 0, NULL, NULL, 3);

    zAUTOTWEAK(prefix, "\001info|\003fov", &fov, 10.0f, 170.0f, &cb_tweak_convert_degrees, NULL, 2);

    switch (coord_type) {
    case XCAM_COORD_CART:
        zAUTOTWEAK(prefix, "\001info|coord ", &spatial.coord.cart, 0, 0, NULL, NULL, 2);
        break;
    case XCAM_COORD_CYLINDER:
        zAUTOTWEAK(prefix, "\001info|coord \001origin ", &spatial.coord.cylinder.origin, 0, 0, NULL, NULL, 3);
        zAUTOTWEAK(prefix, "\001info|coord \002dist", &spatial.coord.cylinder.dist, 0.001f, 1000.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "\001info|coord \003height", &spatial.coord.cylinder.height, -1000.0f, 1000.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "\001info|coord theta", &spatial.coord.cylinder.theta, -1000.0f, 1000.0f, NULL, NULL, 2);
        break;
    case XCAM_COORD_SPHERE:
        zAUTOTWEAK(prefix, "\001info|coord \001origin ", &spatial.coord.sphere.origin, 0, 0, NULL, NULL, 3);
        zAUTOTWEAK(prefix, "\001info|coord \002dist", &spatial.coord.sphere.dist, 0.001f, 1000.0f, NULL, NULL, 2);
    }

    switch (orient_type) {
    case XCAM_ORIENT_QUAT:
        break;
    case XCAM_ORIENT_EULER:
        zAUTOTWEAK(prefix, "\001info|orient \001yaw", &spatial.orient.euler.yaw, -1000.0f, 1000.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "\001info|orient \001pitch", &spatial.orient.euler.pitch, -0.49f * PI, 0.49f * PI, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "\001info|orient \001roll", &spatial.orient.euler.roll, -1000.0f, 1000.0f, NULL, NULL, 2);
    }

    zAUTOTWEAK_FLAG(prefix, "\001debug|render", &debug.flags, 0x9, NULL, NULL, 2);
    zAUTOTWEAK_FLAG(prefix, "\001debug|render override", &debug.flags, 0x8, NULL, NULL, 2);
}

void xCam::add_tweaks_config_follow(const char* prefix)
{
    xCamConfigFollow* fcfg = config_follow();
    if (!fcfg) return;
    zAUTOTWEAK(prefix, "zone_rest|offset ", &fcfg->zone_rest.offset, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "zone_rest|face ", &fcfg->zone_rest.face, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "zone_above|offset ", &fcfg->zone_above.offset, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "zone_above|face ", &fcfg->zone_above.face, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "zone_below|offset ", &fcfg->zone_below.offset, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "zone_below|face ", &fcfg->zone_below.face, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "speed_zone_offset", &fcfg->speed_zone_offset, 0.0f, 100000.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "speed_zone_face", &fcfg->speed_zone_face, 0.0f, 100000.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "speed_move_orbit", &fcfg->speed_move_orbit, 0.0f, 100000.0f, NULL, NULL, 2);
}
#endif

void xCamGroup::create()
{
    for (xCamBlend** it = blend_cam, **end = it + ARRAY_LENGTH(blend_cam); it != end; it++)
        *it = create_blend();
}

void xCamGroup::destroy()
{
    primary = NULL;
    for (xCamBlend** it = blend_cam, **end = it + ARRAY_LENGTH(blend_cam); it != end; it++)
        (*it)->destroy();
}

void xCamGroup::reset()
{
    if (primary) {
        stop(*primary);
    }

    primary = NULL;
    flags = 0;
    fov = fov_default;
    set_child_flags(0, 0);

    coll_mat = g_I3;
    coll_atXZ = coll_mat.at;
    mat = g_I3;
    cameraPlayerAudioBias = 0.0f;
}

void xCamGroup::update(xScene& scene, F32 dt)
{
    xVec3 prev_pos = mat.pos;

    do_transient_replacement(primary);

    if (primary) {
        primary->analog = analog;
        primary->do_update(scene, dt);

        mat = primary->mat;
        coll_mat = primary->coll_mat;

        F32 len;
        xVec3NormalizeXZMacro(&coll_atXZ, &coll_mat.at, &len);
        coll_atXZ.y = 0.0f;
        fov = primary->fov;
        if (dt < SECS_PER_VBLANK)
            dt = SECS_PER_VBLANK;
        vel.ScaleDiff(mat.pos, prev_pos, 1.0f / dt);

        cameraPlayerAudioBias = primary->getCameraPlayerAudioBias();
    }
}

void xCamGroup::transition_to(xCam& cam, const xCamTransitionParams* params, bool force_cut, bool force_restart)
{
    if (!force_restart && primary && primary->owner == cam.owner)
        return;
#if DEBUG || RELEASE
    PrintCamMsg("TRANSITION():", cam.owner);
#endif

    if (force_cut || !params) {
        set_primary(&cam, force_restart);
        return;
    }

    xCamTransition* transition = params ? params->mTransitionObject : NULL;
    xCam* next = get_blend(transition, cam);
    set_primary(next, false);
}

void xCamGroup::add(xCam& cam, bool force_cut)
{
#if DEBUG || RELEASE
    PrintCamMsg("ADD():", cam.owner);
#endif

    transition_to(cam, NULL, force_cut, false);
}

void xCamGroup::remove(xCam& cam, bool)
{
#if DEBUG || RELEASE
    PrintCamMsg("REMOVE():", cam.owner);
#endif

    if (cam.group != this) {
#if DEBUG || RELEASE
        if (gxCamShowDebugMessages) {
            iprintf("\nTried to remove a camera that wasn't added");
        }
#endif
    } else
        stop(cam);
}

xCamBlend* xCamGroup::grab_blend_cam()
{
    for (xCamBlend** it = blend_cam, **end = it + ARRAY_LENGTH(blend_cam); it != end; it++) {
        if (!(*it)->group)
            return *it;
    }
    return NULL;
}

void xCamGroup::set_primary(xCam* toCamera, bool force_restart)
{
    if (!force_restart && primary && primary == toCamera) {
        return;
    }

    if (primary && !(primary->group_flags & 0x10)) {
        stop(*primary);
        mat = primary->mat;
        coll_mat = primary->coll_mat;

        F32 len;
        xVec3NormalizeXZMacro(&coll_atXZ, &coll_mat.at, &len);
        coll_atXZ.y = 0.0f;
    }

    if (!toCamera) {
        return;
    }

    primary = toCamera;
    xASSERT_L(422, !primary->started());
    primary->group = this;
    primary->mat = mat;
    primary->coll_mat = coll_mat;
    primary->fov = fov_default;
    xASSERT_L(427, primary->blender == 0);
    primary->start();
}

xCam* xCamGroup::get_blend(xCamTransition* transition, xCam& to)
{
    if (primary) {
        F32 blend_time = to.cfg_common.blend_time;

        if (transition) {
            if (transition->IsCut()) goto no_blend;
        }
        else if (blend_time <= 0.0f) goto no_blend;

        if (to.group_flags & 0x10) {
            xASSERT_L(454, to.blender != 0);
            xCamBlend* parent = to.blender;
            xCam* pTo = &to;
            while (parent) {
                if (parent->dst != pTo)
                    parent->reverse();
                pTo = parent;
                parent = parent->blender;
            }
            xASSERT_L(464, pTo == primary);
            return primary;
        }

        xCamBlend* blend = grab_blend_cam();
        xASSERT_MSG_L(473, blend != 0, "Ran out of blend cameras.");
        if (!blend) return &to;
        blend->blender = NULL;

        blend->attach(*primary, to);

        to.group = this;

        if (transition) {
            blend->bias = xCamBias::GetBiasObject(transition, blend->timeBias);
        } else {
            blend->timeBias.Init(blend_time);
            blend->bias = &blend->timeBias;
        }

        blend->transition = transition;
        return blend;
    }

no_blend:
    if (transition) {
        transition->Completed(false);
    }

    return &to;
}

void xCamGroup::notify_released(xCam& cam)
{
    cam.group_flags &= ~0x10;
    cam.blender = NULL;
}

void xCamGroup::set_child_flags(S32 add, S32 remove)
{
    child_flags_mask = ~remove;
    child_flags = add & child_flags_mask;
}

static void do_transient_replacement(xCam*& curCam)
{
    xCam* next;
    while (curCam && (curCam->flags & 0x4) && (next = curCam->get_next()) != curCam) {
        if (curCam->started()) {
            curCam->stop();
        }
        curCam->group = NULL;
        curCam = next;
    }
}

#if DEBUG || RELEASE
void xCamGroup::debug_render(bool force, bool sparse)
{
    if (!primary) return;
    if (!(primary->debug.flags & 0x8)) {
        if (!force)
            primary->debug.flags &= ~0x7;
        else if (sparse) {
            primary->debug.flags &= ~0x7;
            primary->debug.flags |= 0x1;
        } else
            primary->debug.flags |= 0x7;
    }
    primary->debug_render();
}
#endif

xCamBlend* xCamGroup::create_blend()
{
    xCamBlend* cam = new (eMemStaticType_0, eMemMgrTag_Camera) xCamBlend();
    if (!cam) return NULL;
    cam->create();
    cam->owner = 'XCBL';
    cam->cfg_common.priority = 0xff;
    return cam;
}

#if DEBUG || RELEASE
void xCamGroup::PrintCamMsg(const char* msg, U32 ownerID) const
{
    if (gxCamShowDebugMessages)
        if (this == globals.cam)
            iprintf("XCAM: %s (%x)%s\n", msg, ownerID, ownerID == 'XCBL' ? "BLENDCAM" : xSTAssetName(ownerID));
}
#endif

void xCamScreen::create(S32 w, S32 h)
{
    icam = iCameraCreate(w, h, 1);
    xASSERT_L(595, icam != 0);
}

void xCamScreen::destroy()
{
    iCameraDestroy(icam);
}

void xCamScreen::set_scene(xScene& scene)
{
    iCameraAssignEnv(icam, scene.env->geom);
}

void xCamScreen::prepare_render()
{
    xASSERT_L(610, icam != 0);
    UpdateGlobalFrustumPlanes(icam);
}

void xCamScreen::begin_render(bool clear)
{
    xASSERT_L(616, icam != 0);
    iCameraBegin(icam, clear);
    iCameraUpdateFog(icam, 0);
}

void xCamScreen::end_render()
{
    iCameraEnd(icam);
}

void xCamScreen::set_world_matrix(const xMat4x3& mat)
{
    iCameraUpdatePos(icam, (xMat4x3*)&mat);
}

void xCamScreen::get_world_matrix(xMat4x3& mat)
{
    RwFrame* frame = RwCameraGetFrame(icam);
    mat = *(const xMat4x3*)RwFrameGetMatrix(frame);
}

void xCamScreen::set_fov(F32 fov)
{
    this->fov = fov;
    iCameraSetFOV(icam, (180.0f/PI) * fov);
}

F32 xCamScreen::world_to_screen(xVec3& screen_loc, const xVec3& world_loc) const
{
    const xMat4x3& view_mat = *(const xMat4x3*)RwCameraGetViewMatrix(icam);
    xVec3 view_loc;
    xMat4x3ToWorld(&view_loc, &view_mat, &world_loc);
    F32 rz = 1.0f / view_loc.z;
    screen_loc.assign(view_loc.x * rz, view_loc.y * rz, 1.0f);
    return rz;
}

// UNUSED: speculative
struct plane_type
{
    xVec3 norm;
    F32 dist;
};

// UNUSED: speculative
void xCamScreen::create_frustum_planes(xVec4* frust_planes, const xMat4x3& ltm, const xVec2& view_offset, const xVec2& view_window, F32 near_plane, F32 far_plane)
{
    plane_type* planes = (plane_type*)frust_planes;

    xVec3 temp, temp2;
    temp.assign(view_offset.x, view_offset.y, 1.0f);
    xMat3x3RMulVec(&temp2, &ltm, &temp);

    xVec3 right, up;
    right.Scale(ltm.right, view_window.x);
    up.Scale(ltm.up, view_window.y);

    xVec3 cop = ltm.pos;
    F32 scale = far_plane / near_plane;

    xVec3 corners[8];
    S32 i;
    corners[0].Add(temp2, right);
    corners[0] += up;

    right *= 2.0f;
    up *= 2.0f;

    corners[1] = corners[0];
    corners[1] -= right;
    corners[2] = corners[1];
    corners[2] -= up;
    corners[3] = corners[0];
    corners[3] -= up;

    for (i = 0; i < 4; i++) {
        xVec3* target = &corners[i];
        temp = *target;
        *target = cop;
        corners[i + 4] = cop;
        target->AddScale(temp, near_plane);
        corners[i + 4].AddScale(temp, near_plane * scale);
    }

    planes[0].norm.Negate(ltm.at);
    planes[0].dist = planes[0].norm.dot(corners[0]);
    planes[1].norm = ltm.at;
    planes[1].dist = planes[1].norm.dot(corners[4]);

    for (i = 0; i < 4; i++) {
        temp.Sub(corners[(i + 1) & 3], cop);
        temp2.Sub(corners[i], cop);
        planes[i + 2].norm.cross(temp, temp2);
        planes[i + 2].norm.normalize();
        planes[i + 2].dist = planes[i + 2].norm.dot(cop);
    }
}

void xCamTimeBias::Init(F32 blendTime)
{
    this->blendTime = blendTime;
}

void xCamTimeBias::Start()
{
    time = 0.0f;
    bias = 0.0f;
}

void xCamTimeBias::Update(F32 dt)
{
    time += dt;
    CalcBias();
}

void xCamTimeBias::Reverse()
{
    time = blendTime - time; if (time < 0.0f)
        time = 0.0f;
    CalcBias();
}

F32 xCamTimeBias::GetBias()
{
    return bias;
}

bool xCamTimeBias::IsActive()
{
    return time < blendTime;
}

bool xCamTimeBias::DidComplete()
{
    return time >= blendTime;
}

void xCamTimeBias::CalcBias()
{
    F32 t = time / blendTime; if (t > 1.0f) t = 1.0f;
    bias = xSCurve(t);
}

xCamBias* xCamBias::GetBiasObject(xCamTransition* transition, xCamTimeBias& timeBias)
{
    if (transition->BlendBias()) {
        return transition->BlendBias();
    }

    timeBias.Init(transition->BlendTime());
    return &timeBias;
}

void xCamBlend::create()
{
    xCam::create();
    flags |= 0x4;
    coord_type = XCAM_COORD_INVALID;
    orient_type = XCAM_ORIENT_INVALID;
}

void xCamBlend::destroy()
{
}

void xCamBlend::start()
{
#if DEBUG || RELEASE
    blendDepth++;
    if (blendDepth > 1 && gxCamShowDebugMessages) {
        iprintf("WARNING: Starting level %d blend\n", blendDepth);
    }

    char levelMsg[128];
    sprintf(levelMsg, "Starting Blend Cam (Level %d)", blendDepth);
    group->PrintCamMsg(levelMsg, owner);
#endif

    static const xCamCoordType coord_table[3][3] = {
        { XCAM_COORD_CART, XCAM_COORD_CART, XCAM_COORD_CART },
        { XCAM_COORD_CART, XCAM_COORD_CYLINDER, XCAM_COORD_SPHERE },
        { XCAM_COORD_CART, XCAM_COORD_SPHERE, XCAM_COORD_SPHERE }
    };

    static const xCamOrientType orient_table[2][2] = {
        { XCAM_ORIENT_QUAT, XCAM_ORIENT_QUAT },
        { XCAM_ORIENT_QUAT, XCAM_ORIENT_EULER }
    };

    xASSERT_L(893, (src != 0) && (dst != 0));
    if (!src->started()) {
        src->group = group;
        src->mat = mat;
        src->coll_mat = coll_mat;
        src->fov = fov;
        src->start();
    }

    if (!dst->started()) {
        dst->group = group;
        dst->mat = mat;
        dst->coll_mat = coll_mat;
        dst->fov = fov;
        dst->start();
    }

    cfg_common.priority = MAX(src->cfg_common.priority, dst->cfg_common.priority);

    xASSERT_L(913, (src->coord_type > XCAM_COORD_INVALID) && (src->coord_type < XCAM_COORD_MAX));
    xASSERT_L(914, (dst->coord_type > XCAM_COORD_INVALID) && (dst->coord_type < XCAM_COORD_MAX));
    xASSERT_L(915, (src->orient_type > XCAM_ORIENT_INVALID) && (src->orient_type < XCAM_ORIENT_MAX));
    xASSERT_L(916, (dst->orient_type > XCAM_ORIENT_INVALID) && (dst->orient_type < XCAM_ORIENT_MAX));

    coord_type = coord_table[src->coord_type][dst->coord_type];
    orient_type = orient_table[src->orient_type][dst->orient_type];

    S32 combined_sprung_flags = (src->flags | dst->flags) & 0x80;
    if (combined_sprung_flags)
        flags |= 0x80;
    else
        flags &= ~0x80;

    bias->Start();
    xCam::start();
}

void xCamBlend::stop()
{
#if DEBUG || RELEASE
    char levelMsg[128];
    sprintf(levelMsg, "Stopping Blend Cam (Level %d)", blendDepth);
    group->PrintCamMsg(levelMsg, owner);
    blendDepth--;
#endif

    group->notify_released(*src);
    group->notify_released(*dst);
    blender = NULL;

    if (bias->DidComplete()) {
        if (bias->GetBias() < 0.5f) {
            group->stop(*dst);
        } else {
            group->stop(*src);
        }
    } else {
        group->stop(*src);
        group->stop(*dst);
    }

    if (transition) {
        transition->Completed(bias->DidComplete());
        transition = NULL;
    }

    xCam::stop();
}

void xCamBlend::pre_update(xScene& scene)
{
    src->pre_update(scene);
    dst->pre_update(scene);
}

void xCamBlend::post_update(xScene& scene)
{
    src->post_update(scene);
    dst->post_update(scene);
}

void xCamBlend::update(xScene& scene, F32 dt)
{
    bias->Update(dt);
    F32 s = bias->GetBias();

    do_transient_replacement(src);
    do_transient_replacement(dst);

    src->analog = analog;
    src->update(scene, dt);

    dst->analog = analog;
    dst->update(scene, dt);

    blend_coord(s);
    blend_orient(s);
    blend(fov, src->fov, dst->fov, s);

    refresh_mat();
}

xCam* xCamBlend::get_next()
{
    return bias->IsActive() ? this : (bias->GetBias() < 0.5f ? src : dst);
}

F32 xCamBlend::getCameraPlayerAudioBias() const
{
    F32 srcBias = src->getCameraPlayerAudioBias();
    F32 dstBias = dst->getCameraPlayerAudioBias();
    return srcBias + (dstBias - srcBias) * bias->GetBias();
}

xCam* xCamBlend::find_camera(U32 ownerID)
{
    xCam* found = src->find_camera(ownerID);
    return found ? found : dst->find_camera(ownerID);
}

void xCamBlend::attach(xCam& src, xCam& dst)
{
    src.blender = dst.blender = this;
    src.group_flags |= 0x10;
    dst.group_flags |= 0x10;
    group_flags = 0x1;
    this->src = &src;
    this->dst = &dst;
}

void xCamBlend::reverse()
{
    xCam* temp = src;
    src = dst;
    dst = temp;
    bias->Reverse();
}

void xCamBlend::blend_coord(F32 s)
{
    switch (coord_type) {
    case XCAM_COORD_CART: {
        xVec3 c1, c2;
        blend(spatial.coord.cart, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.coord.cart, coll_convert(*src, c1), coll_convert(*dst, c2), s);
        break;
    }
    case XCAM_COORD_CYLINDER: {
        xCamCoordCylinder c1, c2;
        blend(spatial.coord.cylinder, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.coord.cylinder, coll_convert(*src, c1), coll_convert(*dst, c2), s);
        break;
    }
    case XCAM_COORD_SPHERE: {
        xCamCoordSphere c1, c2;
        blend(spatial.coord.sphere, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.coord.sphere, coll_convert(*src, c1), coll_convert(*dst, c2), s);
        break;
    }
    }
}

void xCamBlend::blend_orient(F32 s)
{
    switch (orient_type) {
    case XCAM_ORIENT_QUAT: {
        xQuat c1, c2;
        blend(spatial.orient.quat, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.orient.quat, coll_convert(*src, c1), coll_convert(*dst, c2), s);
        break;
    }
    case XCAM_ORIENT_EULER: {
        xCamOrientEuler c1, c2;
        blend(spatial.orient.euler, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.orient.euler, coll_convert(*src, c1), coll_convert(*dst, c2), s);
        break;
    }
    }
}

#if DEBUG || RELEASE
void xCamBlend::debug_render()
{
    xCam::debug_render();
    src->debug.flags |= debug.flags;
    src->debug_render();
    dst->debug.flags |= debug.flags;
    dst->debug_render();
}

void xCamBlend::add_tweaks(const char* prefix)
{
    xCam::add_tweaks(prefix);
}
#endif

void xCamDebug::create()
{
    xCam::create();
    coord_type = XCAM_COORD_CART;
    orient_type = XCAM_ORIENT_EULER;
}

void xCamDebug::start()
{
    xCam::start();
    xASSERT_L(1143, group != 0);
    coord_to_local(spatial.coord.cart, mat.pos);
    orient_to_local(spatial.orient.euler, mat);
    dpad = 0.0f;
    slow_input = 0.0f;
    spatial.orient.euler.roll = 0.0f;
}

void xCamDebug::update(xScene&, F32 dt)
{
    F32 turn_mult = 1.0f + (cfg.slow_turn - 1.0f) * slow_input;
    F32 move_mult = 1.0f + (cfg.slow_move - 1.0f) * slow_input;

    F32 dyaw = dt * (turn_mult * (cfg.stick_vel.x * zPad::GetDebugPad()->GetAnalog2()->GetX()));
    spatial.orient.euler.yaw += dyaw;
    spatial.orient.euler.yaw = xrmod(spatial.orient.euler.yaw);

    F32 dpitch = dt * (turn_mult * (cfg.stick_vel.y * zPad::GetDebugPad()->GetAnalog2()->GetY()));
    spatial.orient.euler.pitch += dpitch;
    spatial.orient.euler.pitch = range_limit(spatial.orient.euler.pitch, -89.0f * (PI/180.0f), 89.0f * (PI/180.0f));

    orient_to_world(mat, spatial.orient.euler);

    xVec3 dright = mat.right;
    dright *= dt * (move_mult * (cfg.move_vel.x * zPad::GetDebugPad()->GetAnalog1()->GetX()));
    spatial.coord.cart += dright;

    F32 dy = dt * (move_mult * (-dpad.y * cfg.move_vel.y));
    spatial.coord.cart.y += dy;

    xVec3 dat = mat.at;
    dat *= dt * (move_mult * (cfg.move_vel.z * zPad::GetDebugPad()->GetAnalog1()->GetY()));
    spatial.coord.cart += dat;

    coord_to_world(mat.pos, spatial.coord.cart);
}

void xCamDebug::refresh_input()
{
#if DEBUG || RELEASE
    switch (xDebugModeGet()) {
    case 1:
    case 2:
    case 13:
        dpad.x = dpad.y = 0.0f;
        return;
    }
#endif

    if (zPad::GetDebugPad()->On(0x80))
        dpad.x = -1.0f;
    else if (zPad::GetDebugPad()->On(0x20))
        dpad.x = 1.0f;
    else dpad.x = 0.0f;

    if (zPad::GetDebugPad()->On(0x10))
        dpad.y = -1.0f;
    else if (zPad::GetDebugPad()->On(0x40))
        dpad.y = 1.0f;
    else dpad.y = 0.0f;
}

#if DEBUG || RELEASE
void xCamDebug::debug_render()
{
    xCam::debug_render();
}

void xCamDebug::add_tweaks(const char* prefix)
{
    add_tweaks_common(prefix);

    zAUTOTWEAK(prefix, "stick_vel_x", &cfg.stick_vel.x, -100000.0f, 100000.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "stick_vel_y", &cfg.stick_vel.y, -100000.0f, 100000.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "move_vel_", &cfg.move_vel, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "slow_turn", &cfg.slow_turn, 0.0f, 1.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "slow_move", &cfg.slow_move, 0.0f, 1.0f, NULL, NULL, 2);
}
#endif
