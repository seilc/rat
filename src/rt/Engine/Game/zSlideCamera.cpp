#include "xCommon.h"
#include "zSlideCamera.h"

#include "zAutoTweak.h"

void zSlideCam::create()
{
    xCam::create();

    coord_type = XCAM_COORD_CART;
    orient_type = XCAM_ORIENT_QUAT;

    cfg_common.priority = 128;
    cfg_common.blend_time = 1.0f;

    load_cfg();
}

void zSlideCam::load_cfg()
{
    cfg.spring_len = 2.5f;
    cfg.spring_const = 3.5f;
    cfg.damp_const = 3.2f;
    cfg.vertical_offset = 2.0f;
}

void zSlideCam::start()
{
    xCam::start();

    prev_tgt_pos = curr_tgt_pos;
    cam_pos = curr_tgt_pos;
}

#if DEBUG || RELEASE
void zSlideCam::add_tweaks(const char* prefix)
{
    add_tweaks_common(prefix);
    add_tweaks_config_follow(prefix);

    zAUTOTWEAK(prefix, "spring const", &cfg.spring_const, 0.001f, 10.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "spring len", &cfg.spring_len, 0.001f, 10.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "damp const", &cfg.damp_const, 0.001f, 10.0f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "vertical offset", &cfg.vertical_offset, -10.0f, 10.0f, NULL, NULL, 2);
}
#endif

static xVec3 SpringDamp(const xVec3& currPos, const xVec3& trgPos, const xVec3& prevTrgPos, F32 deltaTime, F32 springConst, F32 dampConst, F32 springLen)
{
    xVec3 disp, velocity;
    F32 forceMag;

    disp.Sub(currPos, trgPos);
    velocity.ScaleDiff(prevTrgPos, trgPos, deltaTime);
    F32 len = xVec3Length(&disp);
    F32 dot = xVec3Dot(&disp, &velocity);
    forceMag = springConst * (springLen - len) + dampConst * (dot / len);

    xVec3Normalize(&disp, &disp);
    disp *= forceMag * deltaTime;

    xVec3 r;
    xVec3Add(&r, &currPos, &disp);
    return r;
}

void zSlideCam::setCurrTgtPos(const xVec3& pos, const xVec3& up)
{
    curr_tgt_pos = pos;
    curr_tgt_pos.AddScale(up, cfg.vertical_offset);
}

void zSlideCam::setPrevTgtPos(const xVec3& pos, const xVec3& up)
{
    prev_tgt_pos = pos;
    prev_tgt_pos.AddScale(up, cfg.vertical_offset);
}

void zSlideCam::update(xScene&, F32 dt)
{
    setCurrTgtPos(target->pos, target->up);

    F32 len = xVec3Length(curr_tgt_pos.x - prev_tgt_pos.x, curr_tgt_pos.y - prev_tgt_pos.y, curr_tgt_pos.z - prev_tgt_pos.z);

    if (len > 0.001f) {
        cam_pos = SpringDamp(cam_pos, curr_tgt_pos, prev_tgt_pos, dt, cfg.spring_const, cfg.damp_const, cfg.spring_len);
    }

    xVec3 y_axis = { 0.0f, 1.0f, 0.0f };
    mat.up = y_axis;

    mat.at.Sub(prev_tgt_pos, cam_pos);
    xVec3Normalize(&mat.at, &mat.at);

    xVec3Cross(&mat.right, &mat.up, &mat.at);
    xVec3Normalize(&mat.right, &mat.right);

    xVec3Cross(&mat.up, &mat.at, &mat.right);
    xVec3Normalize(&mat.up, &mat.up);

    spatial.coord.cart = cam_pos;
    mat.pos = spatial.coord.cart;

    xQuatFromMat(&spatial.orient.quat, &mat);
}
