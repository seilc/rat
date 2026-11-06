#include "xParCmd.h"

#include "xDebug.h"
#include "xMath.h"
#include "xMath3.h"
#include "xParEmitter.h"
#include "xParGroup.h"
#include "xRand.h"
#include "xString.h"
#include "xutil.h"
#include "zAutoTweak.h"
#include "zParEmitter.h"

#include <stdio.h>
#include <string.h>

struct xCmdInfo
{
    U32 type;
    U32 size;
    xParCmdUpdateFunction func;
};

static xCmdInfo sCmdInfo[eParCmdCount] = { 0 };

void xParCmdInit()
{
    xParCmdRegister(eParCmdMove, sizeof(xParCmdMove), xParCmdMove_Update);
    xParCmdRegister(eParCmdMoveRandom, sizeof(xParCmdMoveRandom), xParCmdMoveRandom_Update);
    xParCmdRegister(eParCmdAccelerate, sizeof(xParCmdAccelerate), xParCmdAccelerate_Update);
    xParCmdRegister(eParCmdVelocityApply, sizeof(xParCmdVelocityApply), xParCmdVelocityApply_Update);
    xParCmdRegister(eParCmdKillOld, sizeof(xParCmdKillOld), NULL);
    xParCmdRegister(eParCmdKillSlow, sizeof(xParCmdKillSlow), xParCmdKillSlow_Update);
    xParCmdRegister(eParCmdFollow, sizeof(xParCmdFollow), xParCmdFollow_Update);
    xParCmdRegister(eParCmdOrbitPoint, sizeof(xParCmdOrbitPoint), xParCmdOrbitPoint_Update);
    xParCmdRegister(eParCmdOrbitLine, sizeof(xParCmdOrbitLine), xParCmdOrbitLine_Update);
    xParCmdRegister(eParCmdMoveRandomPar, sizeof(xParCmdMoveRandomPar), xParCmdMoveRandomPar_Update);
    xParCmdRegister(eParCmdScale3rdPolyReg, sizeof(xParCmdScale3rdPolyReg), xParCmdScale3rdPolyReg_Update);
    xParCmdRegister(eParCmdTex, sizeof(xParCmdTex), xParCmdTex_Update);
    xParCmdRegister(eParCmdTexAnim, sizeof(xParCmdTexAnim), xParCmdTexAnim_Update);
    xParCmdRegister(eParCmdRandomVelocityPar, sizeof(xParCmdRandomVelocityPar), xParCmdRandomVelocityPar_Update);
    xParCmdRegister(eParCmdAge, sizeof(xParCmdAge), xParCmdAge_Update);
    xParCmdRegister(eParCmdAlpha3rdPolyReg, sizeof(xParCmdAlpha3rdPolyReg), xParCmdAlpha3rdPolyReg_Update);
    xParCmdRegister(eParCmdApplyWind, sizeof(xParCmdApplyWind), xParCmdApplyWind_Update);
    xParCmdRegister(eParCmdRotPar, sizeof(xParCmdRotPar), xParCmdRotPar_Update);
    xParCmdRegister(eParCmdRotateAround, sizeof(xParCmdRotateAround), xParCmdRotateAround_Update);
    xParCmdRegister(eParCmdSmokeAlpha, sizeof(xParCmdSmokeAlpha), xParCmdSmokeAlpha_Update);
    xParCmdRegister(eParCmdScale, sizeof(xParCmdScale), xParCmdScale_Update);
    xParCmdRegister(eParCmdCollideFall, sizeof(xParCmdCollideFall), xParCmdCollideFall_Update);
    xParCmdRegister(eParCmdCollideFallSticky, sizeof(xParCmdCollideFallSticky), xParCmdCollideFallSticky_Update);

    xParCmdRegister(eParCmdShaper, sizeof(xParCmdShaperData), xParCmd_Shaper_Update);
    xParCmdRegister(eParCmdAlphaInOut, sizeof(xParCmdAlphaInOutData), xParCmd_AlphaInOut_Update);
    xParCmdRegister(eParCmdSizeInOut, sizeof(xParCmdSizeInOutData), xParCmd_SizeInOut_Update);
    xParCmdRegister(eParCmdDampenSpeed, sizeof(xParCmdDampenData), xParCmd_DampenSpeed_Update);
}

void xParCmdRegister(U32 parType, U32 size, xParCmdUpdateFunction func)
{
    xASSERT(parType >= 0);
    xASSERT_L(115, parType < eParCmdCount);

    sCmdInfo[parType].type = parType;
    sCmdInfo[parType].size = size;
    sCmdInfo[parType].func = func;
}

U32 xParCmdGetSize(U32 parType)
{
    xASSERT(parType >= 0);
    xASSERT_L(134, parType < eParCmdCount);

    for (S32 i = 0; i < eParCmdCount; i++) {
        if (parType == sCmdInfo[i].type)
            return sCmdInfo[i].size;
    }

    iprintf("xParCmdGetSize FAILED: parType %d not found\n", parType);
    return 0;
}

xParCmdUpdateFunction xParCmdGetUpdateFunc(U32 parType)
{
    xASSERT(parType >= 0);
    xASSERT_L(155, parType < eParCmdCount);

    for (S32 i = 0; i < eParCmdCount; i++) {
        if (parType == sCmdInfo[i].type)
            return sCmdInfo[i].func;
    }

    iprintf("xParCmdGetUpdateFunc FAILED: parType %d not found\n", parType);
    return NULL;
}

void xParCmdKillSlow_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdKillSlow* cmd = (xParCmdKillSlow*)c->tasset;

    xASSERT_L(226, cmd);
    xASSERT_L(227, ps);

    F32 speedLimit = cmd->speedLimitSqr * dt;
    p = ps->m_root;

    if (cmd->kill_less_than) {
        while (p) {
            if (xVec3Length2(&p->m_vel) < speedLimit) {
                p->m_lifetime = -1.0f;
            }
            p = p->m_next;
        }
    } else {
        while (p) {
            if (xVec3Length2(&p->m_vel) > speedLimit) {
                p->m_lifetime = -1.0f;
            }
            p = p->m_next;
        }
    }
}

void xParCmdAge_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdAge* cmd = (xParCmdAge*)c->tasset;

    xASSERT_L(270, cmd);
    xASSERT_L(271, ps);

    F32 age_rate = cmd->age_rate * dt;

    p = ps->m_root;
    while (p) {
        p->m_lifetime -= age_rate;
        p = p->m_next;
    }
}

void xParCmdFollow_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdFollow* cmd = (xParCmdFollow*)c->tasset;
    xVec3 delta;

    xASSERT_L(300, cmd);
    xASSERT_L(301, ps);

    F32 mdt = cmd->gravity * dt;

    p = ps->m_root;
    while (p && p->m_next) {
        xVec3Sub(&delta, &p->m_next->m_pos, &p->m_pos);

        F32 lenSqr2 = xVec3Length2(&delta);
        F32 lenSqr = xVec3LengthFast(delta.x, delta.y, delta.z);

        F32 force = mdt / (lenSqr * (lenSqr2 + cmd->epsilon));

        p->m_vel.x += delta.x * force;
        p->m_vel.y += delta.y * force;
        p->m_vel.z += delta.z * force;

        p = p->m_next;
    }
}

void xParCmdOrbitPoint_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdOrbitPoint* cmd = (xParCmdOrbitPoint*)c->tasset;
    xVec3 dir;

    xASSERT_L(338, cmd);
    xASSERT_L(339, ps);

    F32 mdt = cmd->gravity * dt;

    p = ps->m_root;
    while (p) {
        xVec3Sub(&dir, &cmd->center, &p->m_pos);

        F32 rSqr2 = xVec3Length2(&dir);

        if (rSqr2 < cmd->maxRadiusSqr) {
            F32 rSqr = xVec3LengthFast(dir.x, dir.y, dir.z);

            F32 force = mdt / (rSqr + (rSqr2 + cmd->epsilon));
            p->m_vel.x += dir.x * force;
            p->m_vel.y += dir.y * force;
            p->m_vel.z += dir.z * force;
        }

        p = p->m_next;
    }
}

void xParCmdOrbitLine_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdOrbitLine* cmd = (xParCmdOrbitLine*)c->tasset;
    xVec3 f, w, tmp;

    xASSERT_L(382, cmd);
    xASSERT_L(383, ps);

    F32 mdt = cmd->gravity * dt;

    p = ps->m_root;
    while (p) {
        xVec3Sub(&f, &p->m_pos, &cmd->p);

        xVec3Cross(&tmp, &f, &cmd->axis);
        xVec3Cross(&w, &cmd->axis, &tmp);

        xVec3 into;
        xVec3Sub(&into, &w, &f);

        F32 rSqr2 = xVec3Length2(&into);

        if (rSqr2 < cmd->maxRadiusSqr) {
            F32 rSqr = xVec3LengthFast(into.x, into.y, into.z);

            F32 force = mdt / (rSqr + (rSqr2 + cmd->epsilon));
            p->m_vel.x += into.x * force;
            p->m_vel.y += into.y * force;
            p->m_vel.z += into.z * force;
        }

        p = p->m_next;
    }
}

void xParCmdAccelerate_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdAccelerate* cmd = (xParCmdAccelerate*)c->tasset;
    xVec3 delta;

    xASSERT_L(433, cmd);
    xASSERT_L(434, ps);

    delta = cmd->acc;
    xVec3SMulBy(&delta, dt);

    p = ps->m_root;
    while (p) {
        p->m_vel.x += delta.x;
        p->m_vel.y += delta.y;
        p->m_vel.z += delta.z;
        p = p->m_next;
    }
}

void xParCmdMove_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdMove* cmd = (xParCmdMove*)c->tasset;
    xVec3 delta;

    xASSERT_L(463, cmd);
    xASSERT_L(464, ps);

    delta = cmd->dir;
    xVec3SMulBy(&delta, dt);

    p = ps->m_root;
    while (p) {
        xVec3Add(&p->m_pos, &p->m_pos, &delta);
        p = p->m_next;
    }
}

void xParCmdMoveRandom_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdMove* cmd = (xParCmdMove*)c->tasset;
    xVec3 delta;

    xASSERT_L(493, cmd);
    xASSERT_L(494, ps);

    delta = cmd->dir;
    delta.x *= 2.0f * xurand();
    delta.y *= 2.0f * xurand();
    delta.z *= 2.0f * xurand();
    xVec3Sub(&delta, &delta, &cmd->dir);
    xVec3SMulBy(&delta, dt);

    p = ps->m_root;
    while (p) {
        xVec3Add(&p->m_pos, &p->m_pos, &delta);
        p = p->m_next;
    }
}

void xParCmdMoveRandomPar_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdMoveRandomPar* cmd = (xParCmdMoveRandomPar*)c->tasset;
    xVec3 delta;

    xASSERT_L(527, cmd);
    xASSERT_L(528, ps);

    dt *= 0.5f;
    delta.x = cmd->dim.x * dt;
    delta.y = cmd->dim.y * dt;
    delta.z = cmd->dim.z * dt;

    p = ps->m_root;
    while (p) {
        p->m_pos.x += delta.x * (xurand() - 0.5f);
        p->m_pos.z += delta.z * (xurand() - 0.5f);
        p = p->m_next;
    }
}

void xParCmdScale3rdPolyReg_Update(xParCmd*, xParGroup*, F32)
{
}

void xParCmdSmokeAlpha_Update(xParCmd*, xParGroup*, F32)
{
}

void xParCmdScale_Update(xParCmd*, xParGroup*, F32)
{
}

void xParCmdAlpha3rdPolyReg_Update(xParCmd*, xParGroup*, F32)
{
}

void xParCmdRandomVelocityPar_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdRandomVelocityPar* cmd = (xParCmdRandomVelocityPar*)c->tasset;

    xASSERT_L(799, cmd);
    xASSERT_L(800, ps);

    xVec3 rot;
    rot.x = cmd->x * dt;
    rot.y = cmd->y * dt;
    rot.z = cmd->z * dt;

    xMat3x3 rotmat;

    p = ps->m_root;
    while (p) {
        F32 x = 2.0f * (rot.x * xurand()) - rot.x;
        F32 y = 2.0f * (rot.y * xurand()) - rot.y;
        F32 z = 2.0f * (rot.z * xurand()) - rot.z;

        xMat3x3Euler(&rotmat, y, x, z);

        xMat3x3LMulVec(&p->m_vel, &rotmat, &p->m_vel);

        p = p->m_next;
    }
}

void xParCmdApplyWind_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdApplyWind* cmd = (xParCmdApplyWind*)c->tasset;

    xASSERT_L(914, cmd);
    xASSERT_L(915, ps);

    xVec3 wind_vec;
    wind_vec.x = 1.0f;
    wind_vec.y = 0.0f;
    wind_vec.z = 1.0f;

    wind_vec.x *= cmd->strength * dt;
    wind_vec.z *= cmd->strength * dt;

    p = ps->m_root;
    while (p) {
        p->m_vel.x += wind_vec.x;
        p->m_vel.z += wind_vec.z;
        p = p->m_next;
    }
}

void xParCmdRotPar_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdRotPar* cmd = (xParCmdRotPar*)c->tasset;

    xASSERT_L(949, cmd);
    xASSERT_L(950, ps);

    xVec3 range;
    range.x = 255.0f * ((cmd->max.x - cmd->min.x) / 360.0f);
    range.y = 255.0f * ((cmd->max.y - cmd->min.y) / 360.0f);
    range.z = 255.0f * ((cmd->max.z - cmd->min.z) / 360.0f);

    xVec3 min;
    min.x = 255.0f * (cmd->min.x / 360.0f);
    min.y = 255.0f * (cmd->min.y / 360.0f);
    min.z = 255.0f * (cmd->min.z / 360.0f);

    p = ps->m_root;
    while (p) {
        p->m_rotdeg[0] += dt * (range.x * xurand() + min.x);
        p->m_rotdeg[1] += dt * (range.y * xurand() + min.y);
        p->m_rotdeg[2] += dt * (range.z * xurand() + min.z);

        p = p->m_next;
    }
}

void xParCmdVelocityApply_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
#if DEBUG
    xParCmdVelocityApply* cmd = (xParCmdVelocityApply*)c->tasset;

    xASSERT_L(986, cmd);
#endif
    xASSERT_L(988, ps);

    xVec3 vel;

    p = ps->m_root;
    while (p) {
        vel.Scale(p->m_vel, dt);
        xVec3Add(&p->m_pos, &p->m_pos, &vel);

        p = p->m_next;
    }
}

void xParCmdRotateAround_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdRotateAround* cmd = (xParCmdRotateAround*)c->tasset;

    xASSERT_L(1015, cmd);
    xASSERT_L(1016, ps);

    F32 yaw = dt * cmd->yaw;
    yaw = (PI / 180.0f) * yaw;
    F32 radius_growth = dt * cmd->radius_growth;

    p = ps->m_root;
    while (p) {
        xVec3 at;
        xVec3Sub(&at, &cmd->pos, &p->m_pos);
        at.y = 0.0f;

        xMat3x3 lookmat;
        F32 radius = xMat3x3LookVec(&lookmat, &at);

        xVec3 angles;
        xMat3x3GetEuler(&lookmat, &angles);

        angles.x += yaw;

        xMat3x3 rotmat;
        xMat3x3Euler(&rotmat, angles.x, angles.y, angles.z);

        radius += radius_growth;

        xVec3 offset, out_offset;
        offset.x = 0.0f;
        offset.y = 0.0f;
        offset.z = radius;

        xMat3x3RMulVec(&out_offset, &rotmat, &offset);

        p->m_pos.x = out_offset.x + cmd->pos.x;
        p->m_pos.z = out_offset.z + cmd->pos.z;

        p = p->m_next;
    }
}

void xParCmdTex_Update(xParCmd* c, xParGroup* ps, F32)
{
#if DEBUG
    xParCmdTex* cmd = (xParCmdTex*)c->tasset;

    xASSERT_L(1088, cmd);
#endif
    xASSERT_L(1090, ps);
}

#define TEXIDX_NEXT(idx, count) idx++; if (idx >= count) idx = 0
#define TEXIDX_PREV(idx, count) if (idx == 0) idx = count - 1; else idx--

void xParCmdTexAnim_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdTexAnim* cmd = (xParCmdTexAnim*)c->tasset;

    xASSERT_L(1134, cmd);
    xASSERT_L(1135, ps);

    xParCmdTex* tex = ps->m_cmdTex;
    if (!tex) {
        return;
    }

    if (cmd->throttle_time > 0.0f) {
        cmd->throttle_time_elapsed -= dt;
        if (cmd->throttle_time_elapsed > 0.0f) {
            return;
        }
        cmd->throttle_time_elapsed = cmd->throttle_time;
    }

    p = ps->m_root;

    if (cmd->anim_mode == 0) {
        while (p) {
            p->m_texIdx[0] = xrand_RandomChoice(tex->cols);
            p->m_texIdx[1] = xrand_RandomChoice(tex->rows);
            p = p->m_next;
        }
    }
    else if (cmd->anim_mode == 1) {
        if (tex->cols > 1) {
            while (p) {
                p->m_texIdx[0]++;
                if (p->m_texIdx[0] >= tex->cols) {
                    p->m_texIdx[0] = 0;
                    if (cmd->anim_wrap_mode == 1) {
                        TEXIDX_NEXT(p->m_texIdx[1], tex->rows);
                    }
                    else if (cmd->anim_wrap_mode == 2) {
                        TEXIDX_PREV(p->m_texIdx[1], tex->rows);
                    }
                    else if (cmd->anim_wrap_mode == 3) {
                        p->m_texIdx[1]++;
                        if (p->m_texIdx[1] >= tex->rows) {
                            p->m_texIdx[1] = tex->rows - 1;
                            p->m_texIdx[0] = tex->cols - 1;
                        }
                    }
                }
                p = p->m_next;
            }
        }
    }
    else if (cmd->anim_mode == 2) {
        if (tex->cols > 1) {
            while (p) {
                if (p->m_texIdx[0] == 0) {
                    p->m_texIdx[0] = tex->cols - 1;
                    if (cmd->anim_wrap_mode == 1) {
                        TEXIDX_NEXT(p->m_texIdx[1], tex->rows);
                    }
                    else if (cmd->anim_wrap_mode == 2) {
                        TEXIDX_PREV(p->m_texIdx[1], tex->rows);
                    }
                } else {
                    p->m_texIdx[0]--;
                }
                p = p->m_next;
            }
        }
    }
    else if (cmd->anim_mode == 3) {
        if (tex->rows > 1) {
            while (p) {
                if (p->m_texIdx[1] == 0) {
                    p->m_texIdx[1] = tex->rows - 1;
                    if (cmd->anim_wrap_mode == 1) {
                        TEXIDX_NEXT(p->m_texIdx[0], tex->cols);
                    }
                    else if (cmd->anim_wrap_mode == 2) {
                        TEXIDX_PREV(p->m_texIdx[0], tex->cols);
                    }
                } else {
                    p->m_texIdx[1]--;
                }
                p = p->m_next;
            }
        }
    }
    else if (cmd->anim_mode == 4) {
        if (tex->rows > 1) {
            while (p) {
                p->m_texIdx[1]++;
                if (p->m_texIdx[1] >= tex->rows) {
                    p->m_texIdx[1] = 0;
                    if (cmd->anim_wrap_mode == 1) {
                        TEXIDX_NEXT(p->m_texIdx[0], tex->cols);
                    }
                    else if (cmd->anim_wrap_mode == 2) {
                        TEXIDX_PREV(p->m_texIdx[0], tex->cols);
                    }
                }
                p = p->m_next;
            }
        }
    }
    else if (cmd->anim_mode == 5) {
        xFAIL_L(1288);
    }
}

void xParCmdCollideFall_Update(xParCmd* c, xParGroup* ps, F32)
{
    xParCmdCollideFall& cmd = *(xParCmdCollideFall*)c->tasset;

    for (xPar* p = ps->m_root; p; p = p->m_next) {
        F32& loc = p->m_pos.y;
        F32& vel = p->m_vel.y;
        F32 dloc = cmd.y - loc;
        if (dloc < 0.0f) continue;
        loc = dloc * cmd.bounce + cmd.y;
        if (vel < 0.0f) vel = -vel * cmd.bounce;
    }
}

void xParCmdCollideFallSticky_Update(xParCmd* c, xParGroup* ps, F32)
{
    xParCmdCollideFallSticky& cmd = *(xParCmdCollideFallSticky*)c->tasset;
    F32 xzdamp = 1.0f - cmd.sticky;

    for (xPar* p = ps->m_root; p; p = p->m_next) {
        F32& loc = p->m_pos.y;
        F32& vel = p->m_vel.y;
        F32 dloc = cmd.y - loc;
        if (dloc < 0.0f) continue;
        loc = dloc * cmd.bounce + cmd.y;
        if (vel < 0.0f) vel = -vel * cmd.bounce;
        p->m_vel.x *= xzdamp;
        p->m_vel.z *= xzdamp;
    }
}

void xParCmd_DampenSpeed_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdDampenData* cmd = (xParCmdDampenData*)c->tasset;

    xASSERT_L(1337, cmd);
    xASSERT_L(1338, ps);

    if (cmd->enabled) {
        F32 damp = dt * cmd->dampSpeed;

        p = ps->m_root;
        while (p) {
            p->m_vel.AddScale(p->m_vel, damp);
            p = p->m_next;
        }
    }
}

#define NUM_SEGS 3

void xParCmd_SizeInOut_Update(xParCmd* c, xParGroup* ps, F32)
{
    xPar* p;
    xParCmdSizeInOutData* cmd = (xParCmdSizeInOutData*)c->tasset;

    xASSERT_L(1366, cmd);
    xASSERT_L(1367, ps);

    if (cmd->enabled) {
        S32 i = 0, seg;
        F32 slope_size[NUM_SEGS];

        for (i = 0; i < NUM_SEGS; i++) {
            slope_size[i] = 3.0f * (cmd->custSize[i + 1] - cmd->custSize[i]);
        }

        p = ps->m_root;
        while (p) {
            F32 frac = 1.0f - p->m_lifetime / p->totalLifespan;
            frac = CLAMP(frac, 0.0f, 1.0f);

            if (frac < 1.0f / 3.0f) seg = 0;
            else if (frac < 2.0f / 3.0f) seg = 1;
            else seg = 2;

            i = seg;
            while (i-- > 0) frac -= 1.0f / 3.0f;

            p->m_size = frac * slope_size[seg] + cmd->custSize[seg];

            p = p->m_next;
        }
    }
}

void xParCmd_AlphaInOut_Update(xParCmd* c, xParGroup* ps, F32)
{
    xPar* p;
    xParCmdAlphaInOutData* cmd = (xParCmdAlphaInOutData*)c->tasset;

    xASSERT_L(1444, cmd);
    xASSERT_L(1445, ps);

    if (cmd->enabled) {
        S32 i = 0, seg;
        F32 slope_alfa[NUM_SEGS];

        for (i = 0; i < NUM_SEGS; i++) {
            slope_alfa[i] = 3.0f * (cmd->custAlpha[i + 1] - cmd->custAlpha[i]);
        }

        p = ps->m_root;
        while (p) {
            F32 frac = 1.0f - p->m_lifetime / p->totalLifespan;
            frac = CLAMP(frac, 0.0f, 1.0f);

            if (frac < 1.0f / 3.0f) seg = 0;
            else if (frac < 2.0f / 3.0f) seg = 1;
            else seg = 2;

            i = seg;
            while (i-- > 0) frac -= 1.0f / 3.0f;

            F32 alfa = frac * slope_alfa[seg] + cmd->custAlpha[seg];
            p->m_cfl[3] = CLAMP(alfa, 0.0f, 255.0f);
            p->m_c[3] = p->m_cfl[3];

            p = p->m_next;
        }
    }
}

void xParCmd_Shaper_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdShaperData* cmd = (xParCmdShaperData*)c->tasset;

    xASSERT_L(1530, cmd);
    xASSERT_L(1531, ps);

    if (cmd->enabled) {
        F32 damp = dt * cmd->dampSpeed;
        F32 grav = dt * cmd->gravity;

        S32 doalpha = TRUE;
        S32 dosize = TRUE;
        if (cmd->custAlpha[0] < 0.0f) doalpha = FALSE;
        if (cmd->custSize[0] < 0.0f) dosize = FALSE;

        S32 i, seg;
        F32 slope_alfa[NUM_SEGS], slope_size[NUM_SEGS];

        for (i = 0; i < NUM_SEGS; i++) {
            slope_size[i] = 3.0f * (cmd->custSize[i + 1] - cmd->custSize[i]);
            slope_alfa[i] = 3.0f * (cmd->custAlpha[i + 1] - cmd->custAlpha[i]);
        }

        xASSERT_MSG_L(1569, (doalpha||dosize), "Empty Shaper Particle Command");

        p = ps->m_root;
        while (p) {
            p->m_vel.AddScale(p->m_vel, damp);
            p->m_vel.y -= grav;

            xASSERT_L(1590, p->totalLifespan>EPSILON);
            if (p->totalLifespan < EPSILON || (!dosize && !doalpha)) {
                p = p->m_next;
                continue;
            }

            F32 frac = 1.0f - p->m_lifetime / p->totalLifespan;
            frac = CLAMP(frac, 0.0f, 1.0f);

            if (frac < 1.0f / 3.0f) seg = 0;
            else if (frac < 2.0f / 3.0f) seg = 1;
            else seg = 2;

            i = seg;
            while (i-- > 0) frac -= 1.0f / 3.0f;

            if (dosize) {
                p->m_size = frac * slope_size[seg] + cmd->custSize[seg];
            }

            if (doalpha) {
                F32 alfa = frac * slope_alfa[seg] + cmd->custAlpha[seg];
                p->m_cfl[3] = CLAMP(alfa, 0.0f, 255.0f);
                p->m_c[3] = p->m_cfl[3];
            }

            p = p->m_next;
        }
    }
}

#if DEBUG || RELEASE
namespace {

const char* command_labels[] = {
    "Move",
    "MoveRandom",
    "Accelerate",
    "VelocityApply",
    "Jet",
    "KillOld",
    "KillSlow",
    "Follow",
    "OrbitPoint",
    "OrbitLine",
    "MoveRandomPar",
    "Scale3rdPolyReg",
    "Tex",
    "TexAnim",
    "PlayerCollision",
    "RandomVelocityPar",
    "Custom",
    "KillDistance",
    "Age",
    "Alpha3rdPolyReg",
    "ApplyWind",
    "RotPar",
    "ApplyCamMat",
    "RotateAround",
    "SmokeAlpha",
    "Scale",
    "ClipVolumes",
    "AnimalMagentism",
    "DamagePlayer",
    "CollideFall",
    "Shapers: Combo Shaper",
    "Shapers: Vary Alpha",
    "Shapers: Vary Size",
    "Shapers: Scale Speed",
    "CollideFallSticky",
};

void on_change_tex_command(const tweak_info& info)
{
    xASSERT_L(1718, info.context != 0);
    xParCmdTex& cmd = *(xParCmdTex*)info.context;

    cmd.unit_count = cmd.rows * cmd.cols;
    cmd.unit_width = (cmd.x2 - cmd.x1) / cmd.cols;
    cmd.unit_height = (cmd.y2 - cmd.y1) / cmd.rows;
}

void add_command_tweaks(const char* prefix, xParCmdAsset& cmd)
{
    switch (cmd.type) {
    case eParCmdDamagePlayer:
    {
        xParCmdDamagePlayer& ccmd = (xParCmdDamagePlayer&)cmd;
        zAUTOTWEAK(prefix, "Granularity ", &ccmd.granular, 1, 1000, NULL, NULL, 2);
    }
        break;
    case eParCmdVelocityApply:
    case eParCmdClipVolumes:
        break;
    case eParCmdMove:
    case eParCmdMoveRandom:
    case eParCmdAccelerate:
    case eParCmdMoveRandomPar:
    case eParCmdRandomVelocityPar:
    case eParCmdApplyCamMat:
    {
        xParCmdMove& ccmd = (xParCmdMove&)cmd;
        zAUTOTWEAK(prefix, "", &ccmd.dir, 0, 0, NULL, NULL, 2);
    }
        break;
    case eParCmdJet:
    {
        xParCmdJet& ccmd = (xParCmdJet&)cmd;
        zAUTOTWEAK(prefix, "Center ", &ccmd.center, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Acceleration ", &ccmd.acc, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Gravity", &ccmd.gravity, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Epsilon", &ccmd.epsilon, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Radius Squared", &ccmd.radiusSqr, 0.0f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdKillOld:
    {
        xParCmdKillOld& ccmd = (xParCmdKillOld&)cmd;
        zAUTOTWEAK(prefix, "Max Age", &ccmd.ageMax, 0.0f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdKillSlow:
    {
        xParCmdKillSlow& ccmd = (xParCmdKillSlow&)cmd;
        zAUTOTWEAK(prefix, "Max Speed Squared", &ccmd.speedLimitSqr, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Kill Below Max", &ccmd.kill_less_than, 0, 1, NULL, NULL, 2);
    }
        break;
    case eParCmdFollow:
    {
        xParCmdFollow& ccmd = (xParCmdFollow&)cmd;
        zAUTOTWEAK(prefix, "Gravity", &ccmd.gravity, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Epsilon", &ccmd.epsilon, -1e9f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdOrbitPoint:
    {
        xParCmdOrbitPoint& ccmd = (xParCmdOrbitPoint&)cmd;
        zAUTOTWEAK(prefix, "Center ", &ccmd.center, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Gravity", &ccmd.gravity, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Epsilon", &ccmd.epsilon, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Radius Squared", &ccmd.maxRadiusSqr, 0.0f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdOrbitLine:
    {
        xParCmdOrbitLine& ccmd = (xParCmdOrbitLine&)cmd;
        zAUTOTWEAK(prefix, "Location ", &ccmd.p, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Axis ", &ccmd.axis, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Gravity", &ccmd.gravity, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Epsilon", &ccmd.epsilon, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Radius Squared", &ccmd.maxRadiusSqr, 0.0f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdScale3rdPolyReg:
    case eParCmdAlpha3rdPolyReg:
    {
        xParCmdScale3rdPolyReg& ccmd = (xParCmdScale3rdPolyReg&)cmd;
        zAUTOTWEAK(prefix, "Constant", &ccmd.constant, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "a", &ccmd.a, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "a2", &ccmd.a2, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "a3", &ccmd.a3, -1e9f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdTex:
    {
        xParCmdTex& ccmd = (xParCmdTex&)cmd;
        static const char* birth_mode_labels[] = {
            "Default",
            "Random",
            "Previous Particle",
            "Previous Particle Left",
            "Previous Particle Right",
            "Previous Particle Up",
            "Previous Particle Down",
        };
        static tweak_callback cb_tex = tweak_callback::create_change(on_change_tex_command);
        zAUTOTWEAK(prefix, "\001x1", &ccmd.x1, 0.0f, 1.0f, &cb_tex, &cmd, 2);
        zAUTOTWEAK(prefix, "\001y1", &ccmd.y1, 0.0f, 1.0f, &cb_tex, &cmd, 2);
        zAUTOTWEAK(prefix, "\002x2", &ccmd.x2, 0.0f, 1.0f, &cb_tex, &cmd, 2);
        zAUTOTWEAK(prefix, "\002y2", &ccmd.y2, 0.0f, 1.0f, &cb_tex, &cmd, 2);
        zAUTOTWEAK_SELECT(prefix, "\003Birth Mode", &ccmd.birthMode, birth_mode_labels, NULL, ARRAY_LENGTH(birth_mode_labels), NULL, NULL, 2);
        zAUTOTWEAK(prefix, "\004Rows", &ccmd.rows, 1, 15, &cb_tex, &cmd, 2);
        zAUTOTWEAK(prefix, "\005Columns", &ccmd.cols, 1, 15, &cb_tex, &cmd, 2);
        zAUTOTWEAK(prefix, "\006Unit Count", &ccmd.unit_count, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "\006Unit \001Width", &ccmd.unit_width, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "\006Unit \002Height", &ccmd.unit_height, 0, 0, NULL, NULL, 2);
    }
        break;
    case eParCmdTexAnim:
    {
        xParCmdTexAnim& ccmd = (xParCmdTexAnim&)cmd;
        static const char* mode_labels[] = {
            "Random",
            "Cycle Right",
            "Cycle Left",
            "Cycle Up",
            "Cycle Down",
            "Scroll UV Right",
        };
        static const char* wrap_mode_labels[] = {
            "Same",
            "Next",
            "Previous",
            "Next Hold",
        };
        zAUTOTWEAK_SELECT(prefix, "Animation Mode", &ccmd.anim_mode, mode_labels, NULL, ARRAY_LENGTH(mode_labels), NULL, NULL, 2);
        zAUTOTWEAK_SELECT(prefix, "Wrap Mode", &ccmd.anim_wrap_mode, wrap_mode_labels, NULL, ARRAY_LENGTH(wrap_mode_labels), NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Stop Below Max Speed", &ccmd.throttle_spd_less_than, 0, 1, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Max Speed Squared", &ccmd.throttle_spd_sqr, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Delay", &ccmd.throttle_time, 0.0f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdPlayerCollision:
    {
        xParCmdPlayerCollision& ccmd = (xParCmdPlayerCollision&)cmd;
        zAUTOTWEAK(prefix, "Min Time", &ccmd.min, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Max Time", &ccmd.max, 0.0f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdCustom:
    {
        xParCmdCustom& ccmd = (xParCmdCustom&)cmd;
        zAUTOTWEAK(prefix, "ID", &ccmd.user_id, 0U, 0xFFFFFFFFU, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Value 0", &ccmd.user_val[0], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Value 1", &ccmd.user_val[1], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Value 2", &ccmd.user_val[2], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Value 3", &ccmd.user_val[3], -1e9f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdKillDistance:
    {
        xParCmdKillDistance& ccmd = (xParCmdKillDistance&)cmd;
        zAUTOTWEAK(prefix, "Max Distance Squared", &ccmd.dSqr, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Kill Beyond Max", &ccmd.kill_greater_than, 0, 1, NULL, NULL, 2);
    }
        break;
    case eParCmdAge:
    {
        xParCmdAge& ccmd = (xParCmdAge&)cmd;
        zAUTOTWEAK(prefix, "Age Rate", &ccmd.age_rate, 0.0f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdApplyWind:
    {
        xParCmdApplyWind& ccmd = (xParCmdApplyWind&)cmd;
        zAUTOTWEAK(prefix, "Strength", &ccmd.strength, -1e9f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdRotPar:
    {
        xParCmdRotPar& ccmd = (xParCmdRotPar&)cmd;
        zAUTOTWEAK(prefix, "Min ", &ccmd.min, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Max ", &ccmd.max, 0, 0, NULL, NULL, 2);
    }
        break;
    case eParCmdRotateAround:
    {
        xParCmdRotateAround& ccmd = (xParCmdRotateAround&)cmd;
        zAUTOTWEAK(prefix, "Center ", &ccmd.pos, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Radius Growth", &ccmd.radius_growth, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Yaw", &ccmd.yaw, -1e9f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdSmokeAlpha:
    {
        xParCmdSmokeAlpha& ccmd = (xParCmdSmokeAlpha&)cmd;
        static const char* labels[] = {
            "Pit",
            "Chimney",
        };
        zAUTOTWEAK_SELECT(prefix, "Type", &ccmd.type, labels, NULL, ARRAY_LENGTH(labels), NULL, NULL, 2);
    }
        break;
    case eParCmdScale:
    {
        xParCmdScale& ccmd = (xParCmdScale&)cmd;
        static const char* labels[] = {
            "Smoke",
            "Fire",
        };
        zAUTOTWEAK_SELECT(prefix, "Type", &ccmd.type, labels, NULL, ARRAY_LENGTH(labels), NULL, NULL, 2);
    }
        break;
    case eParCmdAnimalMagentism:
    {
        xParCmdAnimalMagentism& ccmd = (xParCmdAnimalMagentism&)cmd;
        zAUTOTWEAK(prefix, "Magnetism", &ccmd.magnetism, -1e9f, 1e9f, NULL, NULL, 2);
    }
        break;
    case eParCmdCollideFall:
    {
        xParCmdCollideFall& ccmd = (xParCmdCollideFall&)cmd;
        zAUTOTWEAK(prefix, "Y", &ccmd.y, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Bounce", &ccmd.bounce, 0.0f, 1000.0f, NULL, NULL, 2);
    }
        break;
    case eParCmdCollideFallSticky:
    {
        xParCmdCollideFallSticky& ccmd = (xParCmdCollideFallSticky&)cmd;
        zAUTOTWEAK(prefix, "Y", &ccmd.y, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Bounce", &ccmd.bounce, 0.0f, 1000.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Sticky", &ccmd.sticky, 0.0f, 1.0f, NULL, NULL, 2);
    }
        break;
    case eParCmdShaper:
    {
        xParCmdShaperData& ccmd = (xParCmdShaperData&)cmd;
        zAUTOTWEAK(prefix, "Alpha \001Birth", &ccmd.custAlpha[0], -1.0f, 255.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Alpha \001life=1/3", &ccmd.custAlpha[1], -1.0f, 255.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Alpha \001life=2/3", &ccmd.custAlpha[2], -1.0f, 255.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Alpha Death", &ccmd.custAlpha[3], -1.0f, 255.0f, NULL, NULL, 2);

        zAUTOTWEAK(prefix, "Size \001Birth", &ccmd.custSize[0], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Size \001life=1/3", &ccmd.custSize[1], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Size \001life=2/3", &ccmd.custSize[2], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Size Death", &ccmd.custSize[3], -1e9f, 1e9f, NULL, NULL, 2);

        zAUTOTWEAK(prefix, "Scale Speed", &ccmd.dampSpeed, -1e9f, 1e9f, NULL, NULL, 2);

        zAUTOTWEAK(prefix, "Gravity", &ccmd.gravity, -1e9f, 1e9f, NULL, NULL, 2);
    }
    case eParCmdAlphaInOut:
    {
        xParCmdAlphaInOutData& ccmd = (xParCmdAlphaInOutData&)cmd;
        zAUTOTWEAK(prefix, "Alpha \001Birth", &ccmd.custAlpha[0], -1.0f, 255.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Alpha \001life=1/3", &ccmd.custAlpha[1], -1.0f, 255.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Alpha \001life=2/3", &ccmd.custAlpha[2], -1.0f, 255.0f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Alpha Death", &ccmd.custAlpha[3], -1.0f, 255.0f, NULL, NULL, 2);
    }
        break;
    case eParCmdSizeInOut:
    {
        xParCmdSizeInOutData& ccmd = (xParCmdSizeInOutData&)cmd;
        zAUTOTWEAK(prefix, "Size \001Birth", &ccmd.custSize[0], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Size \001life=1/3", &ccmd.custSize[1], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Size \001life=2/3", &ccmd.custSize[2], -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Size Death", &ccmd.custSize[3], -1e9f, 1e9f, NULL, NULL, 2);
    }
    case eParCmdDampenSpeed:
    {
        xParCmdDampenData& ccmd = (xParCmdDampenData&)cmd;
        zAUTOTWEAK(prefix, "Scale Speed", &ccmd.dampSpeed, -1e9f, 1e9f, NULL, NULL, 2);
    }
    }
}

void reset_command(xParEmitter& emit, xParCmdAsset& cmd)
{
    switch (cmd.type) {
    case eParCmdDamagePlayer:
    {
        xParCmdDamagePlayer& ccmd = (xParCmdDamagePlayer&)cmd;
        ccmd.damage = 0;
        ccmd.granular = 10;
    }
        break;
    case eParCmdVelocityApply:
    case eParCmdClipVolumes:
        break;
    case eParCmdMove:
    case eParCmdMoveRandom:
    case eParCmdAccelerate:
    case eParCmdMoveRandomPar:
    case eParCmdRandomVelocityPar:
    case eParCmdApplyCamMat:
    {
        xParCmdMove& ccmd = (xParCmdMove&)cmd;
        ccmd.dir = 0.0f;
    }
        break;
    case eParCmdJet:
    {
        xParCmdJet& ccmd = (xParCmdJet&)cmd;
        ccmd.center = 0.0f;
        ccmd.acc = 1.0f;
        ccmd.gravity = 1.0f;
        ccmd.epsilon = 0.001f;
        ccmd.radiusSqr = 4.0f;
    }
        break;
    case eParCmdKillOld:
    {
        xParCmdKillOld& ccmd = (xParCmdKillOld&)cmd;
        ccmd.ageMax = 5.0f;
    }
        break;
    case eParCmdKillSlow:
    {
        xParCmdKillSlow& ccmd = (xParCmdKillSlow&)cmd;
        ccmd.speedLimitSqr = 2.0f;
        ccmd.kill_less_than = 1;
    }
        break;
    case eParCmdFollow:
    {
        xParCmdFollow& ccmd = (xParCmdFollow&)cmd;
        ccmd.gravity = 1.0f;
        ccmd.epsilon = 0.001f;
    }
        break;
    case eParCmdOrbitPoint:
    {
        xParCmdOrbitPoint& ccmd = (xParCmdOrbitPoint&)cmd;
        ccmd.center = 0.0f;
        ccmd.gravity = 1.0f;
        ccmd.epsilon = 0.001f;
        ccmd.maxRadiusSqr = 9.0f;
    }
        break;
    case eParCmdOrbitLine:
    {
        xParCmdOrbitLine& ccmd = (xParCmdOrbitLine&)cmd;
        ccmd.p = 0.0f;
        ccmd.axis.assign(0.0f, 1.0f, 0.0f);
        ccmd.gravity = 1.0f;
        ccmd.epsilon = 0.001f;
        ccmd.maxRadiusSqr = 9.0f;
    }
        break;
    case eParCmdScale3rdPolyReg:
    case eParCmdAlpha3rdPolyReg:
    {
        xParCmdScale3rdPolyReg& ccmd = (xParCmdScale3rdPolyReg&)cmd;
        ccmd.constant = ccmd.a = ccmd.a2 = ccmd.a3 = 0.0f;
    }
        break;
    case eParCmdTex:
    {
        xParCmdTex& ccmd = (xParCmdTex&)cmd;
        ccmd.x1 = ccmd.y1 = 0.0f;
        ccmd.x2 = ccmd.y2 = 1.0f;
        ccmd.birthMode = 0;
        ccmd.rows = ccmd.cols = 1;
        ccmd.unit_count = 1;
        ccmd.unit_width = 1.0f;
        ccmd.unit_height = 1.0f;
    }
        break;
    case eParCmdTexAnim:
    {
        xParCmdTexAnim& ccmd = (xParCmdTexAnim&)cmd;
        ccmd.anim_mode = 0;
        ccmd.anim_wrap_mode = 0;
        ccmd.throttle_spd_less_than = 1;
        ccmd.throttle_spd_sqr = 0.0f;
        ccmd.throttle_time = 0.5f;
    }
        break;
    case eParCmdPlayerCollision:
    {
        xParCmdPlayerCollision& ccmd = (xParCmdPlayerCollision&)cmd;
        ccmd.min = 0.0f;
        ccmd.max = 100.0f;
    }
        break;
    case eParCmdCustom:
    {
        xParCmdCustom& ccmd = (xParCmdCustom&)cmd;
        ccmd.user_id = 0;
        ccmd.user_val[0] = ccmd.user_val[1] = ccmd.user_val[2] = ccmd.user_val[3] = 0.0f;
    }
        break;
    case eParCmdKillDistance:
    {
        xParCmdKillDistance& ccmd = (xParCmdKillDistance&)cmd;
        ccmd.dSqr = 100.0f;
        ccmd.kill_greater_than = 1;
    }
        break;
    case eParCmdAge:
    {
        xParCmdAge& ccmd = (xParCmdAge&)cmd;
        ccmd.age_rate = 1.0f;
    }
        break;
    case eParCmdApplyWind:
    {
        xParCmdApplyWind& ccmd = (xParCmdApplyWind&)cmd;
        ccmd.strength = 0.0f;
    }
        break;
    case eParCmdRotPar:
    {
        xParCmdRotPar& ccmd = (xParCmdRotPar&)cmd;
        ccmd.min = ccmd.max = 0.0f;
    }
        break;
    case eParCmdRotateAround:
    {
        xParCmdRotateAround& ccmd = (xParCmdRotateAround&)cmd;
        ccmd.pos = 0.0f;
        ccmd.radius_growth = 0.0f;
        ccmd.yaw = 0.0f;
    }
        break;
    case eParCmdSmokeAlpha:
    {
        xParCmdSmokeAlpha& ccmd = (xParCmdSmokeAlpha&)cmd;
        ccmd.type = 0;
    }
        break;
    case eParCmdScale:
    {
        xParCmdScale& ccmd = (xParCmdScale&)cmd;
        ccmd.type = 0;
    }
        break;
    case eParCmdAnimalMagentism:
    {
        xParCmdAnimalMagentism& ccmd = (xParCmdAnimalMagentism&)cmd;
        ccmd.magnetism = 1.0f;
    }
        break;
    case eParCmdCollideFall:
    {
        xParCmdCollideFall& ccmd = (xParCmdCollideFall&)cmd;
        ccmd.y = 0.0f;
        ccmd.bounce = 1.0f;
    }
        break;
    case eParCmdCollideFallSticky:
    {
        xParCmdCollideFallSticky& ccmd = (xParCmdCollideFallSticky&)cmd;
        ccmd.y = 0.0f;
        ccmd.bounce = 1.0f;
        ccmd.sticky = 0.0f;
    }
        break;
    case eParCmdShaper:
    {
        xParCmdShaperData& ccmd = (xParCmdShaperData&)cmd;
        ccmd.custAlpha[0] = 0.0f;
        ccmd.custAlpha[1] = 255.0f;
        ccmd.custAlpha[2] = 255.0f;
        ccmd.custAlpha[3] = 0.0f;
        ccmd.custSize[0] = -1.0f;
        ccmd.custSize[1] = -1.0f;
        ccmd.custSize[2] = -1.0f;
        ccmd.custSize[3] = -1.0f;
        ccmd.dampSpeed = 0.0f;
        ccmd.gravity = 0.0f;
    }
        break;
    case eParCmdAlphaInOut:
    {
        xParCmdAlphaInOutData& ccmd = (xParCmdAlphaInOutData&)cmd;
        ccmd.custAlpha[0] = 0.0f;
        ccmd.custAlpha[1] = 255.0f;
        ccmd.custAlpha[2] = 255.0f;
        ccmd.custAlpha[3] = 0.0f;
    }
        break;
    case eParCmdSizeInOut:
    {
        xParCmdSizeInOutData& ccmd = (xParCmdSizeInOutData&)cmd;
        ccmd.custSize[0] = -1.0f;
        ccmd.custSize[1] = -1.0f;
        ccmd.custSize[2] = -1.0f;
        ccmd.custSize[3] = -1.0f;
    }
        break;
    case eParCmdDampenSpeed:
    {
        xParCmdDampenData& ccmd = (xParCmdDampenData&)cmd;
        ccmd.dampSpeed = 0.0f;
    }
    }

    if (!emit.parSys) return;

    emit.parSys->group->m_cmdTex = NULL;
    for (xParCmd* it = emit.parSys->cmd, *end = it + emit.parSys->cmdCount; it != end; it++) {
        if (it->tasset->type == eParCmdTex) {
            emit.parSys->group->m_cmdTex = (xParCmdTex*)it->tasset;
            break;
        }
    }
}

bool omit_one_tweak(const tweak_info& info, void* context)
{
    const substr& omit = *(const substr*)context;
    return icompare(info.name, omit) != 0;
}

substr emitter_name(const substr& name)
{
    char prefix[] = "Particle Emitters|";
    substr ret = {};
    if (memcmp(name.text, prefix, sizeof(prefix) - 1) != 0) return ret;
    ret.text = name.text + sizeof(prefix) - 1;
    const char* end = strchr(ret.text, '|');
    if (!end) return ret;
    ret.size = end - ret.text;
    return ret;
}

void on_change_type(const tweak_info& info)
{
    xASSERT_L(2257, info.context != 0);
    xParCmd& cmd = *(xParCmd*)info.context;

    const tweak_info* parent_info = xDebugGetParentTweak(info);
    if (!parent_info) return;

    substr emitname = emitter_name(info.name);
    xParEmitter* emit = zParEmitterFind(xStrHash(emitname.text, emitname.size));
    if (!emit) return;

    {
        char buffer[128];
        sprintf(buffer, "%.*s|", parent_info->name.size, parent_info->name.text);
        substr name = { buffer, strlen(buffer) }, omit = info.name;
        xDebugRemoveTweaks(name, omit_one_tweak, &omit);
    }

    reset_command(*emit, *cmd.tasset);

    char prefix[128];
    sprintf(prefix, "%.*s|", parent_info->name.size, parent_info->name.text);
    add_command_tweaks(prefix, *cmd.tasset);
}

}

void xParCmdAddTweaks(const char* prefix, xParCmd& cmd)
{
#if DEBUG
    static bool checked = false;
    if (!checked) {
        for (S32 i = 0; i < eParCmdCount; i++) {
            xASSERT_MSG_L(2292, (command_labels[i] != 0), "You added a particle command - you must also add it to xParCmd.cpp/command_labels");
        }
        checked = true;
    }
#endif

    static tweak_callback cb_type = tweak_callback::create_change(on_change_type);
    zAUTOTWEAK_SELECT(prefix, "\001\001Type", &cmd.tasset->type, command_labels, NULL, eParCmdCount, &cb_type, &cmd, 2);
    add_command_tweaks(prefix, *cmd.tasset);
}
#endif
