#include "xParEmitterType.h"

#include "xEnt.h"
#include "xMath2.h"
#include "xMath3.h"
#include "xPar.h"
#include "xParEmitter.h"
#include "xRand.h"
#include "xstransvc.h"
#include "xVolume.h"
#include "iDebug.h"
#include "iMath.h"

typedef struct _xMat2x2
{
    xVec2 right;
    xVec2 up;
} xMat2x2;

static void xParEmitterAngleVariation(xPar* p, xParEmitterAsset* a);

void xParEmitterEmitPoint(xPar* p, xParEmitterAsset* a, F32)
{
    xASSERT_L(54, a);
    xASSERT_L(55, p);
    p->m_vel = a->vel;

    xParEmitterAngleVariation(p, a);
}

static void xParEmitterAngleVariation(xPar* p, xParEmitterAsset* a)
{
    if (a->vel_angle_variation != 0.0f) {
        xMat3x3 mat_rot = { 0 };

        F32 ang_debrisCone = a->vel_angle_variation;
        F32 ang[3] = { 0.0f, 0.0f, 0.0f };

        if (a->emit_flags & 0x20) {
            ang[0] = a->vel_angle_variation * (xurand() - 0.5f);
        } else {
            ang[0] = ang_debrisCone * (xurand() - 0.5f);
            ang[1] = ang_debrisCone * (xurand() - 0.5f);
            ang[2] = ang_debrisCone * (xurand() - 0.5f);
        }

        xMat3x3Euler(&mat_rot, ang[0], ang[1], ang[2]);

        xMat3x3LMulVec(&p->m_vel, &mat_rot, &p->m_vel);
    }
}

void xParEmitterEmitCircleEdge(xPar* p, xParEmitterAsset* a, F32)
{
    xASSERT_L(100, p);
    xASSERT_L(101, a);

    p->m_vel = a->vel;

    F32 rot_amount = 2.0f * PI * xurand();

    F32 ct = icos(rot_amount);
    F32 st = isin(rot_amount);
    xMat2x2 rot_mat = { 1.0f, 0.0f, 0.0f, 1.0f };
    xVec2Init(&rot_mat.right, ct, st);
    xVec2Init(&rot_mat.up, -st, ct);

    xVec2 vec2 = { 0.0f, 1.0f };
    xVec2 new_vec;
    new_vec.x = xVec2Dot(&rot_mat.right, &vec2);
    new_vec.x *= a->e_circle.radius;
    new_vec.y = xVec2Dot(&rot_mat.up, &vec2);
    new_vec.y *= a->e_circle.radius;

    p->m_pos.x += new_vec.x;
    p->m_pos.z += new_vec.y;

    xParEmitterAngleVariation(p, a);

    if (a->e_circle.deflection != 0.0f) {
        F32 dt_radius = a->e_circle.radius * a->e_circle.deflection;
        p->m_vel.x += new_vec.x * dt_radius;
        p->m_vel.z += new_vec.y * dt_radius;
    }
}

void xParEmitterEmitCircle(xPar* p, xParEmitterAsset* a, F32)
{
    xASSERT_L(147, p);
    xASSERT_L(148, a);

    p->m_vel = a->vel;

    F32 rot_amount = 2.0f * PI * xurand();

    F32 ct = icos(rot_amount);
    F32 st = isin(rot_amount);
    xMat2x2 rot_mat = { 1.0f, 0.0f, 0.0f, 1.0f };
    xVec2Init(&rot_mat.right, ct, st);
    xVec2Init(&rot_mat.up, -st, ct);

    xVec2 vec2 = { 0.0f, 1.0f };
    F32 rr = xurand();
    rr = 1.0f - rr * rr;
    vec2.y = rr * a->e_circle.radius;

    xVec2 new_vec;
    new_vec.x = xVec2Dot(&rot_mat.right, &vec2);
    new_vec.y = xVec2Dot(&rot_mat.up, &vec2);
    p->m_pos.x += new_vec.x;
    p->m_pos.z += new_vec.y;

    xParEmitterAngleVariation(p, a);

    if (a->e_circle.deflection != 0.0f) {
        F32 dt_radius = a->e_circle.radius * a->e_circle.deflection;
        p->m_vel.x += new_vec.x * dt_radius;
        p->m_vel.z += new_vec.y * dt_radius;
    }
}

void xParEmitterEmitRect(xPar* p, xParEmitterAsset* a, F32)
{
    xASSERT_L(192, p);
    xASSERT_L(193, a);
    p->m_vel = a->vel;

    p->m_pos.x += 2.0f * (a->e_rect.x_len * xurand()) - a->e_rect.x_len;
    p->m_pos.z += 2.0f * (a->e_rect.z_len * xurand()) - a->e_rect.z_len;

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitRectEdge(xPar* p, xParEmitterAsset* a, F32)
{
    xASSERT_L(208, p);
    xASSERT_L(209, a);

    p->m_vel = a->vel;

    F32 rand = xurand();

    F32 val = 2.0f * (rand * a->e_rect.x_len);
    if (val > a->e_rect.x_len)
        p->m_pos.x += a->e_rect.x_len;
    else
        p->m_pos.x -= a->e_rect.x_len;

    rand = xurand();

    val = 2.0f * (rand * a->e_rect.z_len);
    if (val >= a->e_rect.z_len)
        p->m_pos.z += a->e_rect.z_len;
    else
        p->m_pos.z -= a->e_rect.z_len;

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitLine(xPar* p, xParEmitterAsset* a, F32)
{
    xASSERT_L(239, p);
    xASSERT_L(240, a);

    p->m_vel = a->vel;

    xVec3 vec;
    xVec3Sub(&vec, &a->e_line.pos2, &a->e_line.pos1);
    F32 len = xVec3Normalize(&vec, &vec);

    F32 rand_len = len * xurand();
    p->m_pos.x = vec.x * rand_len + a->e_line.pos1.x;
    p->m_pos.y = vec.y * rand_len + a->e_line.pos1.y;
    p->m_pos.z = vec.z * rand_len + a->e_line.pos1.z;

    if (a->e_line.radius > 0.0f) {
        F32 yaw = 2.0f * PI * xurand(), pitch = 2.0f * PI * xurand();
        F32 sy = isin(yaw), cy = icos(yaw), sp = isin(pitch), cp = icos(pitch);
        xVec3 dir = { -sy, cy * sp, cy * cp };

        F32 rr = xurand();
        rr = 1.0f - rr * rr * rr;
        p->m_pos.AddScale(dir, a->e_line.radius * rr);
    }

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitSphere(xPar* p, xParEmitterAsset* a, F32)
{
    xASSERT_L(279, (p != 0) && (a != 0));

    p->m_vel = a->vel;

    F32 yaw = 2.0f * PI * xurand(), pitch = 2.0f * PI * xurand();
    F32 sy = isin(yaw), cy = icos(yaw), sp = isin(pitch), cp = icos(pitch);
    xVec3 dir = { -sy, cy * sp, cy * cp };
    p->m_pos.AddScale(dir, a->e_sphere.radius * xurand());

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitSphereEdge(xPar* p, xParEmitterAsset* a, F32, S32 subtype)
{
    xASSERT_L(298, p);
    xASSERT_L(299, a);

    xVec3 dirvec = { 0.0f, 0.0f, 1.0f };
    xVec3 offset = { 0.0f, 0.0f, 0.0f };
    xMat3x3 mat_rot = { 0 };

    p->m_vel = a->vel;

    switch (subtype) {
    case 7:
        xMat3x3Euler(&mat_rot, 2.0f * PI * xurand(), 2.0f * PI * xurand(), 2.0f * PI * xurand());
        break;
    case 10:
        xMat3x3Euler(&mat_rot, 2.0f * PI * xurand(), PI * xurand() + PI, 0.0f);
        break;
    case 11:
        xMat3x3Euler(&mat_rot, 2.0f * PI * xurand(), PI * xurand(), 0.0f);
        break;
    default:
        xFAIL_L(322);
    }

    dirvec = mat_rot.at;

    F32 mag = xVec3Length(&p->m_vel);
    xVec3SMul(&p->m_vel, &dirvec, mag);

    xVec3SMul(&offset, &dirvec, a->e_sphere.radius);
    xVec3AddTo(&p->m_pos, &offset);

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitVolume(xPar* p, xParEmitterAsset* a, F32, xVolume* vol)
{
    xASSERT_L(347, p);
    xASSERT_L(348, a);

    p->m_vel = a->vel;

    if (vol) {
        const xBound* b = vol->GetBound();
        xASSERT_L(363, b);

        if (b->type == k_XBOUNDTYPE_BOX) {
            xVec3 diff;
            xVec3Sub(&diff, &b->box.box.upper, &b->box.box.lower);

            diff.x *= xurand();
            diff.y *= xurand();
            diff.z *= xurand();

            xVec3Add(&p->m_pos, &diff, &b->box.box.lower);
        } else {
            iprintf("ah hell, guess we should write some code for other volumes eh?\n");
            xFAIL_L(380);
        }
    }

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitEntity(xPar* p, xParEmitterAsset* a, F32, xEnt* ent)
{
    xASSERT_L(393, ent != 0);
    xASSERT_L(394, ent->model != 0);

    U32 size = 0;
    for (xModelInstance* model = ent->model; model; model = model->Next) {
        if (model->Flags & 0x8000) continue;
        RpGeometry* geom = RpAtomicGetGeometry(model->Data);
        xASSERT_FMT_L(404, RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0)) != 0,
                      "Can't volume emit from a pre-instanced model. Emitter: %s, Volume entity: %s",
                      xSTAssetNameOrID(a), xSTAssetNameOrID(a->e_volume.emit_volumeID));
        size += RpGeometryGetNumVertices(geom);
    }

    xVec3 loc;
    U32 which = (xrand_RandomInt32() >> 13) % size;
    for (xModelInstance* model = ent->model; model; model = model->Next) {
        if (model->Flags & 0x8000) continue;
        RpGeometry* geom = RpAtomicGetGeometry(model->Data);
        U32 size = RpGeometryGetNumVertices(geom);
        if (which < size) {
            iModelVertEval(model->Data, which, 1, model->Mat, NULL, &loc);
            break;
        }
        which -= size;
    }

    p->m_pos = loc;
    p->m_vel = a->vel;
    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitOffsetPoint(xParEmitter* pe, xPar* p, xParEmitterAsset* a, F32, xEnt* ent)
{
    xASSERT_L(438, p);
    xASSERT_L(439, a);

    if (ent) {
        RpGeometry* geom = RpAtomicGetGeometry(ent->model->Data);
        if (RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0)))
            iModelTagEval(ent->model->Data, &pe->tag, ent->model->Mat, &p->m_pos);
    } else {
        p->m_pos += a->e_offsetp.offset;
    }

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitVCylEdge(xPar* p, xParEmitterAsset* a, F32)
{
    F32 ang = 2.0f * PI * xurand();
    xVec2 dir = { isin(ang), icos(ang) };
    xVec2 offset = dir * a->e_vcyl.radius;

    p->m_pos.x += offset.x;
    p->m_pos.y += a->e_vcyl.height * xurand();
    p->m_pos.z += offset.y;

    F32 deflect = a->e_vcyl.deflection;
    p->m_vel.x += offset.x * deflect;
    p->m_vel.z += offset.y * deflect;

    xParEmitterAngleVariation(p, a);
}

namespace {

inline void ocircle_emit(xPar& p, xParEmitterAsset& a, F32, F32 radius)
{
    xMat3x3 mat;
    xMat3x3LookVec(&mat, &a.e_circle.dir);

    F32 theta = 2.0f * PI * xurand();
    F32 ct = icos(theta);
    F32 st = isin(theta);
    xVec3 offset = { ct, st, 0.0f };
    offset *= radius;

    xMat3x3RMulVec(&offset, &mat, &offset);

    p.m_pos += offset;

    p.m_vel.Scale(a.e_circle.dir, a.vel.y);
    xParEmitterAngleVariation(&p, &a);

    if (a.e_circle.deflection != 0.0f) {
        p.m_vel.AddScale(offset, radius * a.e_circle.deflection);
    }
}

}

void xParEmitterEmitOCircleEdge(xPar* p, xParEmitterAsset* a, F32 dt)
{
    xASSERT_L(514, p != 0);
    xASSERT_L(515, a != 0);
    ocircle_emit(*p, *a, dt, a->e_circle.radius);
}

void xParEmitterEmitOCircle(xPar* p, xParEmitterAsset* a, F32 dt)
{
    xASSERT_L(521, p != 0);
    xASSERT_L(522, a != 0);

    F32 rr = xurand();
    rr = 1.0f - rr * rr;
    ocircle_emit(*p, *a, dt, rr * a->e_circle.radius);
}

namespace {

inline xVec3 get_random_offset(const xPEEntBone& region, const xMat4x3& mat)
{
    xVec3 offset;

    switch (region.type) {
    case 0:
        offset = 0.0f;
        break;
    case 1: {
        F32 ang = 2.0f * PI * xurand();
        F32 uz = 2.0f * xurand() - 1.0f;
        F32 r = xsqrt(1.0f - uz * uz);
        F32 rr = xurand(); rr = 1.0f - rr * rr * rr;
        rr *= region.radius;
        offset.assign(rr * r * icos(ang), rr * r * isin(ang), rr * uz);
    }
        break;
    case 2: {
        F32 ang = 2.0f * PI * xurand();
        F32 rr = region.radius * xsqrt(xurand());
        offset.assign(rr * isin(ang), 0.0f, rr * icos(ang));
    }
        break;
    case 3: {
        F32 ang = 2.0f * PI * xurand();
        F32 rr = region.radius * xsqrt(xurand());
        offset.Combine(mat.up, rr * isin(ang), mat.at, rr * icos(ang));
    }
        break;
    case 4: {
        F32 ang = 2.0f * PI * xurand();
        F32 rr = region.radius * xsqrt(xurand());
        offset.Combine(mat.right, rr * isin(ang), mat.at, rr * icos(ang));
    }
        break;
    case 5: {
        F32 ang = 2.0f * PI * xurand();
        F32 rr = region.radius * xsqrt(xurand());
        offset.Combine(mat.right, rr * isin(ang), mat.up, rr * icos(ang));
    }
        break;
    case 6: {
        F32 ang = 2.0f * PI * xurand();
        F32 uz = 2.0f * xurand() - 1.0f;
        F32 r = region.radius * xsqrt(1.0f - uz * uz);
        offset.assign(r * icos(ang), r * isin(ang), uz * region.radius);
    }
        break;
    case 7: {
        F32 ang = 2.0f * PI * xurand();
        offset.assign(region.radius * isin(ang), 0.0f, region.radius * icos(ang));
    }
        break;
    case 8: {
        F32 ang = 2.0f * PI * xurand();
        F32 rr = region.radius;
        offset.Combine(mat.up, rr * isin(ang), mat.at, rr * icos(ang));
    }
        break;
    case 9: {
        F32 ang = 2.0f * PI * xurand();
        F32 rr = region.radius;
        offset.Combine(mat.right, rr * isin(ang), mat.at, rr * icos(ang));
    }
        break;
    case 10: {
        F32 ang = 2.0f * PI * xurand();
        F32 rr = region.radius;
        offset.Combine(mat.right, rr * isin(ang), mat.up, rr * icos(ang));
    }
        break;
    }

    return offset;
}

inline xVec3 get_random_offset(const xBound& bound, F32 expand, U32 vtype)
{
    xVec3 offset;
    xVec3 expandVec = { expand, expand, expand };

    switch (bound.type | (vtype << 3)) {
    case k_XBOUNDTYPE_SPHERE: {
        F32 ang = 2.0f * PI * xurand();
        F32 uz = 2.0f * xurand() - 1.0f;
        F32 r = xsqrt(1.0f - uz * uz);
        F32 rr = xurand(); rr = 1.0f - rr * rr * rr;
        rr *= bound.sph.r + expand;
        offset.assign(rr * r * icos(ang), rr * r * isin(ang), rr * uz);
    }
        break;
    case k_XBOUNDTYPE_BOX:
        offset.SubAdd(bound.box.box.upper, bound.box.center, expandVec);
        offset.x *= 2.0f * xurand() - 1.0f;
        offset.y *= 2.0f * xurand() - 1.0f;
        offset.z *= 2.0f * xurand() - 1.0f;
        break;
    case k_XBOUNDTYPE_OBB:
        offset.Add(bound.box.box.upper, expandVec);
        offset.x *= 2.0f * xurand() - 1.0f;
        offset.y *= 2.0f * xurand() - 1.0f;
        offset.z *= 2.0f * xurand() - 1.0f;
        xMat3x3RMulVec(&offset, bound.mat, &offset);
        break;
    case k_XBOUNDTYPE_SPHERE | (1 << 3): {
        F32 ang = 2.0f * PI * xurand();
        F32 uz = 2.0f * xurand() - 1.0f;
        F32 r = (bound.sph.r + expand) * xsqrt(1.0f - uz * uz);
        offset.assign(r * icos(ang), r * isin(ang), uz * (bound.sph.r + expand));
    }
        break;
    case k_XBOUNDTYPE_BOX | (1 << 3): {
        U32 pick = xrand_RandomInt32() >> 13;
        U32 side = pick % 3;
        F32 sign = (pick & 4) ? 1.0f : -1.0f;

        xVec3 scale;
        if (side == 0) {
            scale.assign(sign, 2.0f * xurand() - 1.0f, 2.0f * xurand() - 1.0f);
        }
        else if (side == 1) {
            scale.assign(2.0f * xurand() - 1.0f, sign, 2.0f * xurand() - 1.0f);
        } else {
            scale.assign(2.0f * xurand() - 1.0f, 2.0f * xurand() - 1.0f, sign);
        }

        offset.SubAdd(bound.box.box.upper, bound.box.center, expandVec);
        offset.ScaleComponents(scale);
    }
        break;
    case k_XBOUNDTYPE_OBB | (1 << 3): {
        U32 pick = xrand_RandomInt32() >> 13;
        U32 side = pick % 3;
        F32 sign = (pick & 4) ? 1.0f : -1.0f;

        xVec3 scale;
        if (side == 0) {
            scale.assign(sign, 2.0f * xurand() - 1.0f, 2.0f * xurand() - 1.0f);
        }
        else if (side == 1) {
            scale.assign(2.0f * xurand() - 1.0f, sign, 2.0f * xurand() - 1.0f);
        } else {
            scale.assign(2.0f * xurand() - 1.0f, 2.0f * xurand() - 1.0f, sign);
        }

        offset.Add(bound.box.box.upper, expandVec);
        offset.ScaleComponents(scale);
        xMat3x3RMulVec(&offset, bound.mat, &offset);
    }
        break;
    }

    return offset;
}

inline void transform_ent_bone(xVec3& loc, xVec3& vel, const xParEmitterAsset& a, const xMat4x3& mat)
{
    const xPEEntBone& region = a.e_entbone;

    if (region.flags & 0x1)
        xMat3x3RMulVec(&vel, &mat, &a.vel);
    else vel = a.vel;

    if (region.flags & 0x2) {
        if (region.flags & 0x4) {
            xVec3 offset = { region.offset.x, 0.0f, region.offset.z };
            xMat4x3ToWorld(&loc, &mat, &offset);
            loc.y = region.offset.y;
        } else {
            xMat4x3ToWorld(&loc, &mat, &region.offset);
        }
    } else {
        loc.Add(mat.pos, region.offset);
        if (region.flags & 0x4) {
            loc.y = region.offset.y;
        }
    }

    if (region.flags & 0x4)
        loc.y = region.offset.y;
}

}

xMat4x3* xParEmitterTransformEntBone(xVec3& loc, xVec3& vel, const xParEmitterAsset& a, const xEnt& ent)
{
    const xPEEntBone& region = a.e_entbone;
    const xModelInstance& model = *ent.model;

    xASSERT_L(750, region.bone <= model.BoneCount);

    static xMat4x3 buffer_mat;
    const xMat4x3& root_mat = *(const xMat4x3*)model.Mat;
    const xMat4x3* mat;
    if (region.bone == 0) mat = &root_mat;
    else {
        const xMat4x3& anim_mat = ((const xMat4x3*)model.Mat)[region.bone];
        xMat4x3Mul(&buffer_mat, &anim_mat, &root_mat);
        mat = &buffer_mat;
    }

    transform_ent_bone(loc, vel, a, *mat);
    return (xMat4x3*)mat;
}

void xParEmitterTransformEntBone(xVec3& loc, xVec3& vel, const xParEmitterAsset& a, const xMat4x3& mat)
{
    transform_ent_bone(loc, vel, a, mat);
}

void xParEmitterEmitEntBone(xPar* p, xParEmitterAsset* a, F32, const xMat4x3& mat)
{
    const xPEEntBone& region = a->e_entbone;

    xVec3 offset = get_random_offset(region, mat);
    p->m_pos += offset;

    if (region.deflection != 0.0f) {
        p->m_vel.AddScale(offset, region.deflection);
    }

    xParEmitterAngleVariation(p, a);
}

void xParEmitterEmitEntBound(xPar* p, xParEmitterAsset* a, F32, const xEnt* ent)
{
    const xPEEntBound& region = a->e_entbound;
    const xModelInstance& model = *ent->model;
    const xMat4x3& mat = *(const xMat4x3*)model.Mat;

    if (region.flags & 0x1)
        xMat3x3RMulVec(&p->m_vel, &mat, &a->vel);
    else p->m_vel = a->vel;

    xVec3 offset = get_random_offset(ent->bound, region.expand, region.type);
    p->m_pos.Add(mat.pos, offset);

    if (region.deflection != 0.0f) {
        p->m_vel.AddScale(offset, region.deflection);
    }

    xParEmitterAngleVariation(p, a);
}
