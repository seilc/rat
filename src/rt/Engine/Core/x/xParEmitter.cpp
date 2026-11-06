#include "xParEmitter.h"

#include "xCam.h"
#include "xColor.h"
#include "xCommon.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xGroup.h"
#include "xMath.h"
#include "xMovePoint.h"
#include "xParEmitterType.h"
#include "xRand.h"
#include "xstransvc.h"
#include "xutil.h"
#include "xVolume.h"
#include "zAutoTweak.h"
#include "zCam.h"
#include "zEvent.h"
#include "zPlayer.h"
#include "zScene.h"
#include "iMath.h"
#include "iModel.h"

#include "decomp.h"

#include <rpworld.h>
#include <stdio.h>
#include <string.h>

#define CONSTA_ID 0x48E48E7A // xStrHash("ConstA")
#define CONSTB_ID 0x48E48E7B // xStrHash("ConstB")
#define SINE_ID 0x0B326F01 // xStrHash("Sine")
#define COSINE_ID 0x498D7119 // xStrHash("Cosine")
#define LINEAR_ID 0xB7353B79 // xStrHash("Linear")
#define STEP_ID 0x0B354BD4 // xStrHash("Step")
#define RANDOM_ID 0x0FE111BF // xStrHash("Random")

#define PROP_INTERP_COUNT 14

static xParEmitterAsset sSaveEmmiterSettings;
static xParEmitterPropsAsset sSaveEmmiterPropSettings;
static xParEmitterPropsAsset sDummyProp;

#if DEBUG || RELEASE
static void add_tweaks(xParEmitter& emit);
#else
static void add_tweaks(xParEmitter& emit)
{
}
#endif

S32 xParInterpConvertInterpMode(xParInterp* p)
{
    if (p->interp < 8)
        return p->interp;

    if (p->interp == CONSTA_ID)
        return 0;
    if (p->interp == CONSTB_ID)
        return 1;
    if (p->interp == SINE_ID)
        return 4;
    if (p->interp == COSINE_ID)
        return 5;
    if (p->interp == LINEAR_ID)
        return 3;
    if (p->interp == STEP_ID)
        return 7;
    if (p->interp == RANDOM_ID)
        return 2;

    return 0;
}

void xParEmitterInit(void* b, void* tasset)
{
    xParEmitterInit((xBase*)b, (xParEmitterAsset*)tasset);
}

void xParEmitterInit(xBase* b, xParEmitterAsset* pea)
{
    xParEmitterPropsAsset* prop;

    xBaseInit(b, pea);

    xParEmitter* t = (xParEmitter*)b;

    t->eventFunc = xParEmitterEventCB;
    t->tasset = pea;
    t->prop = (xParEmitterPropsAsset*)xSTFindAsset(pea->propID, NULL);
    if (!t->prop) {
        t->prop = &sDummyProp;
    }

    if (t->linkCount)
        t->link = (xLinkAsset*)(t->tasset + 1);
    else
        t->link = NULL;

    prop = t->prop;

    t->emit_flags = pea->emit_flags;

    t->parSys = t->prop->parSysID ? (xParSys*)zSceneFindObject(t->prop->parSysID) : NULL;

    t->attachTo = NULL;

    t->emit_volume = NULL;

    for (S32 i = 0; i < PROP_INTERP_COUNT; i++) {
        if (prop->value[i].val[0] == prop->value[i].val[1])
            prop->value[i].interp = 0;
        else
            prop->value[i].interp = xParInterpConvertInterpMode(&prop->value[i]);
    }

    t->rate_mode = prop->rate.interp;

    if (t->rate_mode == 2) {
        if (prop->rate.val[0] == prop->rate.val[1]) {
            t->rate_mode = 0;
        }
    }

    if (t->rate_mode == 2) {
        prop->rate.order();
    }

    t->rate = t->rate_mode == 2 ? (prop->rate.val[1] - prop->rate.val[0]) * xurand() + prop->rate.val[0] : prop->rate.val[0];
    t->rate_time = 0.0f;
    t->rate_fraction = 0.0f;
    t->rate_fraction_cull = 0.0f;
    t->oocull_distance_sqr = (pea->emit_flags & 0x2) ? 1.0f / pea->cull_dist_sqr : 0.0f;

    t->emit_volume = NULL;

    if (pea->emit_type == 14 || pea->emit_type == 13) {
        F32 mag2 = pea->e_circle.dir.length2();
        if (mag2 < 0.001f) pea->e_circle.dir.assign(0.0f, 1.0f, 0.0f);
        else pea->e_circle.dir *= 1.0f / xsqrt(mag2);
    }

    t->last_attach_loc = 1e38f;

    t->sortID = 0;

#if DEBUG || RELEASE
    t->tweak_flags = 0;
#endif
}

void xParEmitterSetup(xParEmitter* t)
{
    xASSERT_L(252, t);

    if (t->parSys) {
        t->group = t->parSys->group;
    }

    if (t->tasset->attachToID && t->tasset->emit_type == 9) {
        xEnt* ent = (xEnt*)zSceneFindObject(t->tasset->attachToID);
        xASSERT_L(275, ent);

        iModelTagSetup(&t->tag, ent->model->Data, t->tasset->e_offsetp.offset.x, t->tasset->e_offsetp.offset.y, t->tasset->e_offsetp.offset.z);
    }

    t->attachTo = t->tasset->attachToID ? zSceneFindObject(t->tasset->attachToID) : NULL;

    switch (t->tasset->emit_type) {
    case 6:
        t->emit_volume = zSceneFindObject(t->tasset->e_volume.emit_volumeID);
        break;
    case 15:
    case 16:
    default:
        break;
    }

    add_tweaks(*t);
}

void xParEmitterReset(xParEmitter* t)
{
    xASSERT_L(312, t);
    xASSERT_L(313, t->tasset);

    xBaseReset(t, t->tasset);

    t->emit_flags = t->tasset->emit_flags;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("s")
#endif

void xParEmitterEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xParEmitter* t = (xParEmitter*)to;

    xASSERT_L(375, to);

    switch (toEvent) {
    case eEventDebugReset:
    case eEventReset:
        xParEmitterReset(t);
        break;
    case eEventOn:
        t->emit_flags |= 0x1;
        break;
    case eEventOff:
        FLAG_REMOVE(t->emit_flags, 0x1);
        break;
    case eEventEmit:
        xParEmitterCustomSettings fake_settings;
        memset(&fake_settings, 0, sizeof(fake_settings));
        xParEmitterEmitCustom(t, 1.0f / 30.0f, &fake_settings);
        break;
    }
}

xPar* xParEmitterEmitCustom(xParEmitter* p, F32 dt, xParEmitterCustomSettings* info)
{
    xASSERT_L(422, p);
    xASSERT_L(423, info);
    xASSERT_L(424, dt >= 0);

    U32 flags = info->custom_flags;
    xParEmitterAsset* pe = p->tasset;
    if (!pe) {
        return NULL;
    }

    if (flags & 0x1) {
        memcpy(&sSaveEmmiterSettings, pe, sizeof(xParEmitterAsset));
        memcpy(&sSaveEmmiterPropSettings, p->prop, sizeof(xParEmitterPropsAsset));
    }

    if (flags & 0x100) {
        pe->pos = info->pos;
        p->last_emit_position = info->last_emit_position;
    }
    if (flags & 0x10) {
        p->prop->rate = info->rate;
    }
    if (flags & 0x2) {
        p->prop->life = info->life;
    }
    if (flags & 0x2000) {
        p->emit_volume = info->emit_volume;
    }
    if (flags & 0x4) {
        p->prop->size_birth = info->size_birth;
    }
    if (flags & 0x8) {
        p->prop->size_death = info->size_death;
    }

    if (flags & 0x200) {
        pe->vel = info->vel;
        p->last_emit_velocity = info->last_emit_velocity;
        p->prop->vel = info->vel;
    }
    if (flags & 0x40) {
        pe->vel_angle_variation = info->vel_angle_variation;
    }

    if (flags & 0x400) {
        p->prop->color_birth[0] = info->color_birth[0];
        p->prop->color_birth[1] = info->color_birth[1];
        p->prop->color_birth[2] = info->color_birth[2];
        p->prop->color_birth[3] = info->color_birth[3];
    }
    if (flags & 0x800) {
        p->prop->color_death[0] = info->color_death[0];
        p->prop->color_death[1] = info->color_death[1];
        p->prop->color_death[2] = info->color_death[2];
        p->prop->color_death[3] = info->color_death[3];
    }

    if (flags & 0x1000) {
        switch (p->tasset->emit_type) {
        case 1:
        case 2:
        case 13:
        case 14:
            p->tasset->e_circle.radius = info->radius;
            break;
        case 7:
        case 8:
        case 10:
        case 11:
            p->tasset->e_sphere.radius = info->radius;
            break;
        case 0:
            break;
        }
    }

    xPar* emitted_par = xParEmitterEmit(p, dt);

    if (flags & 0x1) {
        memcpy(pe, &sSaveEmmiterSettings, sizeof(xParEmitterAsset));
        memcpy(p->prop, &sSaveEmmiterPropSettings, sizeof(xParEmitterPropsAsset));
    }

    return emitted_par;
}

static S32 xParEmitterCull(xParEmitter* pe, xPar* p)
{
    xASSERT_L(537, pe);
    xASSERT_L(538, p);

    xParEmitterAsset* a = pe->tasset;
    if (a->emit_flags & 0x2) {
        xVec3 camera_pos;
        camera_pos = xglobals->cam->mat.pos;

        xVec3 d;
        xVec3Sub(&d, &camera_pos, &p->m_pos);

        F32 dmag = xVec3Length2(&d);

        pe->distance_to_cull_sqr = dmag;

        if (a->cull_mode == 3)
            return dmag > a->cull_dist_sqr;
        else
            return dmag < a->cull_dist_sqr;
    }
    return 0;
}

F32 xParInterpCompute(S32 interp_mode, xParInterp* r, F32 time, S32 time_has_elapsed, F32 lastVal)
{
    F32 val = 0.0f;

    switch (interp_mode) {
    case 0:
        val = r->val[0];
        break;
    case 1:
        val = r->val[1];
        break;
    case 2:
        if (time_has_elapsed) {
            val = (r->val[1] - r->val[0]) * xurand() + r->val[0];
        } else {
            val = lastVal;
        }
        break;
    case 3:
        xASSERT_FMT_L(604, (r->freq>0.0f), "Linear Interp requires non-zero 'Freq' (eg use '1')");

        if (r->freq < 0.00001f) val = r->val[0];
        else val = (r->val[1] - r->val[0]) * (time / r->freq) + r->val[0];
        break;
    case 4:
        val = (r->val[1] - r->val[0]) * (0.5f * isin(2.0f * PI * (time * r->oofreq)) + 0.5f) + r->val[0];
        break;
    case 5:
        val = (r->val[1] - r->val[0]) * (0.5f * icos(2.0f * PI * (time * r->oofreq)) + 0.5f) + r->val[0];
        break;
    case 7:
        if (time * r->freq >= 0.5f)
            val = r->val[1];
        else
            val = r->val[0];
        break;
    }

    return val;
}

xPar* xParEmitterEmitSetTexIdxs(xPar* p, const xParSys* ps)
{
    xASSERT_L(635, p);
    xASSERT_L(636, ps);
    xASSERT_L(637, ps->group);

    xParCmdTex* tex = ps->group->m_cmdTex;
    if (!tex) {
        return NULL;
    }

    switch (tex->birthMode) {
    case 1:
        p->m_texIdx[0] = (xrand_RandomInt32() >> 17) % tex->cols;
        p->m_texIdx[1] = (xrand_RandomInt32() >> 17) % tex->rows;
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        p->m_texIdx[0] = 0;
        p->m_texIdx[1] = 0;
        break;
    }

    return p;
}

xPar* xParEmitterEmit(xParEmitter* pe, F32 emit_dt, F32 par_dt)
{
    static xTWEAKABLE S32 doDebugDraw = 0;
    static xTWEAKABLE F32 rad_cullDebugDraw = 30.0f;

    if (!pe->parSys) {
        return NULL;
    }

    xPar* last_p = NULL;

    xASSERT_L(680, pe);

    xParEmitterAsset* pea = pe->tasset;
    xParEmitterAsset* a = pea;
    xParEmitterPropsAsset* prop = pe->prop;

    pe->rate_time += emit_dt;
    S32 rate_has_elapsed = pe->rate_time > prop->rate.freq;

    if (prop->rate.freq == 0.0f) {
        pe->rate_time = 0.0f;
    }
    xASSERT_L(694, prop->rate.freq>=0.0f);
    while (pe->rate_time > prop->rate.freq)
        pe->rate_time -= prop->rate.freq;

    F32 rate;
    xParInterp* r = &prop->rate;

    rate = xParInterpCompute(pe->rate_mode, r, pe->rate_time, rate_has_elapsed, pe->rate);

    pe->rate = rate;

    pe->rate_fraction += rate * emit_dt;
    pe->rate_fraction_cull += rate * emit_dt;

    S32 count = std::floorf(pe->rate_fraction);

    if (count > 0) {
        pe->rate_fraction -= count;
    }
    if (count == 0) {
        return NULL;
    }
    xASSERT_FMT_L(719, (count<0xFFFF), "Somewhat large particle emit count: %d\n", count);

    xParSys* ps = pe->parSys;
    if (!ps) {
        xASSERT_L(729, ps);
        return NULL;
    }

    xParGroup* g = ps->group;
    if (!g) {
        return NULL;
    }

    if (ps->tasset->maxPar && g->m_num_of_particles >= ps->tasset->maxPar) {
        return NULL;
    }

    if (ps->tasset->maxPar && ps->group->m_num_of_particles >= ps->tasset->maxPar) {
        return NULL;
    }

    xPar* p;
    F32 life = prop->life.val[0], size_birth = prop->size_birth.val[0], size_death = prop->size_death.val[0];
    xVec3 emitPosition = a->pos;

    if (ps->tasset->maxPar && ps->group->m_num_of_particles + count >= ps->tasset->maxPar) {
        count = ps->tasset->maxPar - ps->group->m_num_of_particles;
    }

    xBase* attachObject = (xBase*)pe->attachTo;
    U32 attachGroupIndex = 0xFFFFFFFF, attachGroupTotal = 0xFFFFFFFF;
    S32 emitAgain, emitterIsAttachedToObject = FALSE;

    do {
        emitAgain = FALSE;
        xBase* emitObj = NULL;
        S32 marker = FALSE;

        if (attachObject) {
            if (attachObject->baseType == eBaseTypeGroup) {
                xGroup* g = (xGroup*)attachObject;

                if (attachGroupIndex == 0xFFFFFFFF) {
                    attachGroupTotal = xGroupGetCount(g);
                    attachGroupIndex = 0;
                }

                xASSERT_L(862, g->asset);
                if (g->asset->groupFlags & 0x1) {
                    U32 get_rnd_group_idx = xrand_RandomChoice(attachGroupTotal);

                    emitObj = xGroupGetItemPtr(g, get_rnd_group_idx);
                    emitAgain = FALSE;

                    if (!emitObj) {
                        emitObj = (xBase*)xSTFindAsset(xGroupGetItem(g, get_rnd_group_idx), NULL);
                        if (emitObj)
                            marker = TRUE;
                    }
                } else {
                    emitObj = xGroupGetItemPtr(g, attachGroupIndex);
                    if (!emitObj) {
                        emitObj = (xBase*)xSTFindAsset(xGroupGetItem(g, attachGroupIndex), NULL);
                        if (emitObj) {
                            marker = TRUE;
                        }
                    }

                    attachGroupIndex++;
                    emitAgain = attachGroupIndex < attachGroupTotal ? TRUE : FALSE;
                }
            } else {
                emitObj = attachObject;
            }
        }

        const xEnt* attach_ent = (const xEnt*)emitObj;
        if (emitObj) {
            emitterIsAttachedToObject = TRUE;
            if (marker) {
                emitPosition = *(xVec3*)emitObj;
                attach_ent = NULL;
            } else {
                switch (emitObj->baseType) {
                case eBaseTypeMovePoint:
                    emitPosition = *((xMovePoint*)emitObj)->pos;
                    attach_ent = NULL;
                    break;
                default:
                    if (!xEntValidType(attach_ent->baseType) || !attach_ent->model) {
                        attach_ent = NULL; emitPosition = 0.0f;
                    } else {
                        emitPosition = *xEntGetPos(attach_ent);
                    }
                }
            }
        } else {
            emitPosition = a->pos;
        }

        if ((pe->emit_flags & 0x10) && attach_ent && !xEntIsVisible(attach_ent)) {
            continue;
        }

        const xMat4x3* bone_mat = NULL;
        xVec3 bone_vel;
        if (a->emit_type == 15 && attach_ent) {
            bone_mat = xParEmitterTransformEntBone(emitPosition, bone_vel, *a, *attach_ent);
            bone_vel *= par_dt;
        }

        xVec3 add_vel;
        if (pe->emit_flags & 0x8) {
            add_vel.Sub(emitPosition, pe->last_attach_loc);
            pe->last_attach_loc = emitPosition;

            if (add_vel.length2() > 25.0f)
                add_vel = 0.0f;
        } else {
            add_vel = 0.0f;
        }

        if (doDebugDraw) {
            xVec3 diff; diff.Sub(*xEntGetPos(CURPLAYERS[0]), emitPosition);
            F32 ds2 = diff.length2();
            if (ds2 < xsqr(rad_cullDebugDraw)) {
                xDrawSetColor(g_WHITE);
                xDrawLine(xEntGetPos(CURPLAYERS[0]), &emitPosition);
            }
        }

        F32 last_emit_time = -emit_dt;

        const xVec3& camPos = zCamGetMatrix().pos;

        for (S32 i = 0; i < count; i++) {
            xParGroupSetSortDistance(camPos.Distance2(emitPosition), pe->sortID);

            p = xParGroupAddPar(ps->group);
            if (p) {
                last_p = p;

                life = xParInterpCompute(prop->life.interp, &prop->life, pe->rate_time, TRUE, 0.0f);

                size_birth = xParInterpCompute(prop->size_birth.interp, &prop->size_birth, pe->rate_time, TRUE, 0.0f);
                size_death = xParInterpCompute(prop->size_death.interp, &prop->size_death, pe->rate_time, TRUE, 0.0f);

                p->m_lifetime = life;
                p->totalLifespan = life;

                p->m_size = size_birth;
                if (size_death == size_birth)
                    p->m_sizeVel = 0.0f;
                else
                    p->m_sizeVel = (size_death - size_birth) / life;

                p->m_flag = 0;

                p->m_rotdeg[0] = pe->rot[0];
                p->m_rotdeg[1] = pe->rot[1];
                p->m_rotdeg[2] = pe->rot[2];

                for (S32 c = 0; c < (S32)ARRAY_LENGTH(prop->color_birth); c++) {
                    F32 fc1 = xParInterpCompute(prop->color_birth[c].interp, &prop->color_birth[c], pe->rate_time, TRUE, 0.0f);
                    F32 fc2 = xParInterpCompute(prop->color_death[c].interp, &prop->color_death[c], pe->rate_time, TRUE, 0.0f);

                    p->m_cfl[c] = fc1;
                    p->m_c[c] = fc1;
                    p->m_cvel[c] = (fc2 - fc1) / life;
                }

                p->m_pos = emitPosition;

                xParEmitterEmitSetTexIdxs(p, ps);

                switch (a->emit_type) {
                case 0:
                    xParEmitterEmitPoint(p, a, par_dt);
                    break;
                case 1:
                    xParEmitterEmitCircleEdge(p, a, par_dt);
                    break;
                case 2:
                    xParEmitterEmitCircle(p, a, par_dt);
                    break;
                case 3:
                    xParEmitterEmitRectEdge(p, a, par_dt);
                    break;
                case 4:
                    xParEmitterEmitRect(p, a, par_dt);
                    break;
                case 5:
                    xParEmitterEmitLine(p, a, par_dt);
                    break;
                case 7:
                    xParEmitterEmitSphereEdge(p, a, par_dt, 7);
                    break;
                case 10:
                    xParEmitterEmitSphereEdge(p, a, par_dt, 10);
                    break;
                case 11:
                    xParEmitterEmitSphereEdge(p, a, par_dt, 11);
                    break;
                case 8:
                    xParEmitterEmitSphere(p, a, par_dt);
                    break;
                case 6:
                {
                    xBase* obj = pe->emit_volume;
                    if (obj) {
                        if (obj->baseType == eBaseTypeVolume)
                            xParEmitterEmitVolume(p, a, par_dt, (xVolume*)obj);
                        else {
                            xASSERT_L(1096, xEntValidType(obj->baseType));
                            xParEmitterEmitEntity(p, a, par_dt, (xEnt*)obj);
                        }
                    }
                    break;
                }
                case 9:
                    xParEmitterEmitOffsetPoint(pe, p, a, par_dt, (xEnt*)emitObj);
                    break;
                case 12:
                    xParEmitterEmitVCylEdge(p, a, par_dt);
                    break;
                case 13:
                    xParEmitterEmitOCircleEdge(p, a, par_dt);
                    break;
                case 14:
                    xParEmitterEmitOCircle(p, a, par_dt);
                    break;
                case 15:
                    if (bone_mat) {
                        p->m_vel = bone_vel;
                        xParEmitterEmitEntBone(p, a, par_dt, *bone_mat);
                    }
                    break;
                case 16:
                    if (attach_ent) {
                        xParEmitterEmitEntBound(p, a, par_dt, attach_ent);
                    }
                    break;
                default:
                    break;
                }

                last_emit_time += emit_dt / count;
                xASSERT_L(1143, last_emit_time <= 0.0001f);

                bool tempGroupInitialized = false;
                static xParGroup temp_group;

                if (!tempGroupInitialized) {
                    tempGroupInitialized = true;
                    xParGroupInit(&temp_group);
                    pe->last_emit_position = p->m_pos;
                    pe->last_emit_velocity = p->m_vel;
                }

                temp_group.m_root = NULL;

                xPar* save_next = p->m_next, *save_prev = p->m_prev;
                xParGroupAddParP(&temp_group, p);

                F32 u = -last_emit_time / emit_dt;

                xVec3 pos_diff;
                if (emitterIsAttachedToObject) {
                    pos_diff.Sub(p->m_pos, emitPosition);
                    p->m_pos.Lerp(emitPosition, pe->last_emit_position, u);
                    p->m_pos.Add(pos_diff);
                } else {
                    pos_diff.Sub(p->m_pos, a->pos);
                    p->m_pos.Lerp(a->pos, pe->last_emit_position, u);
                    p->m_pos.Add(pos_diff);
                }

                F32 initial_length = p->m_vel.length();

                p->m_vel.Lerp(p->m_vel, pe->last_emit_velocity, u);

                if (initial_length > 0.00001f)
                    p->m_vel /= initial_length;
                p->m_vel *= initial_length * (1.0f - u) + u * pe->last_emit_velocity.length();

                p->m_lifetime += last_emit_time;

                if (p->m_lifetime < 0.0f) {
                    p->m_next = save_next;
                    p->m_prev = save_prev;
                    xParGroupKillPar(ps->group, p);
                    continue;
                }

                xParGroupUpdate(ps, &temp_group, -last_emit_time);

                p->m_next = save_next;
                p->m_prev = save_prev;

                p->m_vel += add_vel;

                if (xParEmitterCull(pe, p)) {
                    xParGroupKillPar(ps->group, p);
                    p = NULL;
                }

                if (doDebugDraw) {
                    xVec3 diff; diff.Sub(*xEntGetPos(CURPLAYERS[0]), p->m_pos);
                    F32 ds2 = diff.length2();
                    if (ds2 < xsqr(rad_cullDebugDraw)) {
                        xDrawSetColor(g_PINK);
                        xDrawLine(xEntGetPos(CURPLAYERS[0]), &p->m_pos);
                    }
                }
            } else {
                emitAgain = FALSE;
            }
        }
    } while (emitAgain);

    pe->last_emit_position = emitPosition;
    pe->last_emit_velocity = a->vel;

    return last_p;
}

void xParEmitterUpdate(xBase* to, xScene*, F32 dt)
{
    xParEmitter* pe = (xParEmitter*)to;

    xASSERT_L(1301, pe);
    xASSERT_L(1302, pe->tasset);

    if (pe->parSys) {
        if (pe->emit_flags & 0x1)
            xParEmitterEmit(pe, dt);
    }
}

// RESTORED: from BFBB GC, TSSM GC
void xParEmitterDestroy()
{
    xTWEAK_REMOVE("Particle Emitters");
}
// END RESTORED

#if DEBUG || RELEASE
namespace {

const char* region_type_names[] = {
    "Point",
    "XZ Circle Edge",
    "XZ Circle",
    "XZ Rectangle Edge",
    "XZ Rectangle",
    "Line",
    "Volume",
    "Sphere Edge",
    "Sphere",
    "Offset Point",
    "Sphere Edge Upper",
    "Sphere Edge Lower",
    "Cylinder-V Edge",
    "Circle Edge",
    "Circle",
    "Entity Bone",
    "Entity Bound"
};

const char* interp_type_names[] = {
    "Constant A",
    "Constant B",
    "Random",
    "Linear",
    "Sine",
    "Cosine",
    "Saw-Tooth",
    "Step"
};

const char* render_type_names[] = {
    "Sprite",
    "Streak",
    "Flat",
    "Static",
    "Ground",
    "QuadStreak",
    "InvStreak"
};

char* get_asset_name(xParEmitterAsset* asset)
{
    const char* name = xSTAssetName(asset);
    if (name && *name) return (char*)name;
    else {
        static char buffer[16];
        sprintf(buffer, "0x%08X", asset->id);
        return buffer;
    }
}

void on_change_circle_dir(const tweak_info& info)
{
    xASSERT_L(1391, info.context != 0);
    xParEmitterAsset& asset = *(xParEmitterAsset*)info.context;
    F32 mag2 = asset.e_circle.dir.length2();
    if (mag2 < 0.001f) asset.e_circle.dir.assign(0.0f, 1.0f, 0.0f);
    else asset.e_circle.dir *= 1.0f / xsqrt(mag2);
}

void on_change_volume_entity(const tweak_info& info)
{
    xASSERT_L(1400, info.context != 0);
    xParEmitter& emit = *(xParEmitter*)info.context;
    emit.emit_volume = NULL;
    U32 id = emit.tasset->e_volume.emit_volumeID;
    if (!id) return;
    xEnt* ent = (xEnt*)zSceneFindObject(id);
    if (!ent) return;
    xASSERT_L(1407, xEntValidType(ent->baseType));
    if (!ent->model) return;

    for (xModelInstance* model = ent->model; model; model = model->Next) {
        RpGeometry* geom = RpAtomicGetGeometry(model->Data);
        if (!RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0))) return;
    }
    emit.emit_volume = ent;
}

void on_change_bone_index(const tweak_info& info)
{
    xASSERT_L(1421, info.context != 0);
    xParEmitter& emit = *(xParEmitter*)info.context;
    xParEmitterAsset& asset = *emit.tasset;
    xPEEntBone& region = asset.e_entbone;
    xEnt* ent = (xEnt*)emit.attachTo;
    if (ent && xEntValidType(ent->baseType) && ent->model)
        region.bone = range_limit<U8>(region.bone, 0, ent->model->BoneCount);
}

void add_region_tweaks(xParEmitter& emit, bool reset)
{
    xParEmitterAsset& asset = *emit.tasset;
    char prefix[128];
    sprintf(prefix, "Particle Emitters|%s|\002Region Data", get_asset_name(&asset));
    xTWEAK_REMOVE(prefix);
    strcat(prefix, "|");

    switch (asset.emit_type) {
    case 0:
        break;
    case 1:
    case 2:
        zAUTOTWEAK(prefix, "Radius", &asset.e_circle.radius, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Deflection", &asset.e_circle.deflection, -1e9f, 1e9f, NULL, NULL, 2);
        if (reset) {
            asset.e_circle.radius = 1.0f;
            asset.e_circle.deflection = 0.0f;
        }
        break;
    case 3:
    case 4:
        zAUTOTWEAK(prefix, "X Length", &asset.e_rect.x_len, 0.0f, 1e6f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Z Length", &asset.e_rect.z_len, 0.0f, 1e6f, NULL, NULL, 2);
        if (reset) {
            asset.e_rect.x_len = 1.0f;
            asset.e_rect.z_len = 1.0f;
        }
        break;
    case 5:
        zAUTOTWEAK(prefix, "Location 1 ", &asset.e_line.pos1, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Location 2 ", &asset.e_line.pos2, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Radius", &asset.e_line.radius, 0.0f, 1e6f, NULL, NULL, 2);
        if (reset) {
            asset.e_line.pos1.assign(0.0f);
            asset.e_line.pos2.assign(1.0f);
            asset.e_line.radius = 1.0f;
        }
        break;
    case 6:
        static tweak_callback cb_volume_entity = tweak_callback::create(on_change_volume_entity);
        zAUTOTWEAK_SELECT(prefix, "Entity", &asset.e_volume.emit_volumeID, all_entity_labels, all_entity_ids, all_entity_count, &cb_volume_entity, &emit, 2);
        if (reset) {
            asset.e_volume.emit_volumeID = 0;
        }
        break;
    case 7:
    case 8:
    case 10:
    case 11:
        zAUTOTWEAK(prefix, "Radius", &asset.e_sphere.radius, 0.0f, 1e9f, NULL, NULL, 2);
        if (reset) {
            asset.e_sphere.radius = 1.0f;
        }
        break;
    case 9:
        zAUTOTWEAK(prefix, "Location", &asset.e_offsetp.offset, 0, 0, NULL, NULL, 2);
        if (reset) {
            asset.e_offsetp.offset.assign(0.0f);
        }
        break;
    case 12:
        zAUTOTWEAK(prefix, "Height", &asset.e_vcyl.height, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Radius", &asset.e_vcyl.radius, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Deflection", &asset.e_vcyl.deflection, -1e9f, 1e9f, NULL, NULL, 2);
        if (reset) {
            asset.e_vcyl.height = asset.e_vcyl.radius = 1.0f;
            asset.e_vcyl.deflection = 0.0f;
        }
        break;
    case 13:
    case 14:
        zAUTOTWEAK(prefix, "Radius", &asset.e_circle.radius, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Deflection", &asset.e_circle.deflection, -1e9f, 1e9f, NULL, NULL, 2);
        static tweak_callback cb_circle_dir = tweak_callback::create(on_change_circle_dir);
        zAUTOTWEAK(prefix, "Direction", &asset.e_circle.dir, 0, 0, &cb_circle_dir, &asset, 2);
        if (reset) {
            asset.e_circle.radius = 1.0f;
            asset.e_circle.deflection = 0.0f;
            asset.e_circle.dir.assign(0.0f, 1.0f, 0.0f);
        }
        break;
    case 15:
    {
        static const char* type_labels[] = {
            "Point",
            "Sphere",
            "Vertical Circle",
            "Right Circle",
            "Up Circle",
            "At Circle",
            "Sphere-Edge",
            "Vertical Circle-Edge",
            "Right Circle-Edge",
            "Up Circle-Edge",
            "At Circle-Edge"
        };
        static tweak_callback cb_bone_index = tweak_callback::create(on_change_bone_index);
        zAUTOTWEAK(prefix, "\001Bone Index", &asset.e_entbone.bone, 0, 64, &cb_bone_index, &emit, 2);
        zAUTOTWEAK_FLAG(prefix, "Transform Velocity", &asset.e_entbone.flags, 0x1, NULL, NULL, 2);
        zAUTOTWEAK_FLAG(prefix, "Transform Offset", &asset.e_entbone.flags, 0x2, NULL, NULL, 2);
        zAUTOTWEAK_FLAG(prefix, "Use Root Y", &asset.e_entbone.flags, 0x4, NULL, NULL, 2);
        zAUTOTWEAK_SELECT(prefix, "Type", &asset.e_entbone.type, type_labels, NULL, ARRAY_LENGTH(type_labels), NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Offset ", &asset.e_entbone.offset, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Radius", &asset.e_entbone.radius, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Deflection", &asset.e_entbone.deflection, -1e9f, 1e9f, NULL, NULL, 2);
        if (reset) {
            asset.e_entbone.bone = 0;
            asset.e_entbone.flags = 0;
            asset.e_entbone.type = 1;
            asset.e_entbone.offset = 0.0f;
            asset.e_entbone.radius = 1.0f;
            asset.e_entbone.deflection = 0.0f;
        }
        break;
    }
    case 16:
    {
        static const char* type_labels[] = {
            "Volume",
            "Edge"
        };
        zAUTOTWEAK_FLAG(prefix, "Transform Velocity", &asset.e_entbound.flags, 0x1, NULL, NULL, 2);
        zAUTOTWEAK_SELECT(prefix, "Type", &asset.e_entbound.type, type_labels, NULL, ARRAY_LENGTH(type_labels), NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Expand", &asset.e_entbound.expand, -1e9f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Deflection", &asset.e_entbound.deflection, -1e9f, 1e9f, NULL, NULL, 2);
        if (reset) {
            asset.e_entbound.flags = 0;
            asset.e_entbound.type = 0;
            asset.e_entbound.expand = 0.0f;
            asset.e_entbound.deflection = 0.0f;
        }
    } break;
    default:
        xFAIL_L(1586);
    }
}

void on_change_region_type(const tweak_info& info)
{
    xASSERT_L(1592, info.context != 0);
    xParEmitter& emit = *(xParEmitter*)info.context;
    add_region_tweaks(emit, true);
}

void on_change_cmd_count(const tweak_info& info)
{
    xASSERT_L(1599, info.context != 0);
    xParEmitter& emit = *(xParEmitter*)info.context;
    xParSys& sys = *emit.parSys;

    char prefix[128];
    sprintf(prefix, "Particle Emitters|%s|\001Commands", get_asset_name(emit.tasset));
    char* end = prefix + strlen(prefix);

    const tweak_info* parent = xDebugGetTweak(prefix);
    if (!parent) return;
    U32 size = xDebugCountChildTweaks(*parent);

    if (sys.cmdCount > size) {
        for (U32 i = size; i < sys.cmdCount; i++) {
            xParCmd& cmd = sys.cmd[i];
            sprintf(end, "|%02d|", i);
            xParCmdAddTweaks(prefix, cmd);
        }
    }
    else if (sys.cmdCount < size) {
        for (U32 i = sys.cmdCount; i < size; i++) {
            sprintf(end, "|%02d", i);
            xTWEAK_REMOVE(prefix);
        }
    }
}

void add_interp_tweak(const char* cat, const char* title, xParInterp& pi)
{
    char prefix[128];
    sprintf(prefix, "%s%s|", cat, title);
    zAUTOTWEAK(prefix, "A", &pi.val[0], -1e9f, 1e9f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "B", &pi.val[1], -1e9f, 1e9f, NULL, NULL, 2);
    zAUTOTWEAK_SELECT(prefix, "Type", &pi.interp, interp_type_names, NULL, ARRAY_LENGTH(interp_type_names), NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Frequency", &pi.freq, -1e9f, 1e9f, NULL, NULL, 2);
}

void verify_tables()
{
#if DEBUG
    // RESTORED: speculative
    if (0) {
        static bool verified = false;

        if (!verified) {
            verified = true;

            for (S32 i = 0; i < ARRAY_LENGTH(region_type_names); i++) {
                xASSERT(region_type_names[i]);
            }
            for (S32 i = 0; i < ARRAY_LENGTH(interp_type_names); i++) {
                xASSERT(interp_type_names[i]);
            }
            for (S32 i = 0; i < ARRAY_LENGTH(render_type_names); i++) {
                xASSERT(render_type_names[i]);
            }
        }
    }
    // END RESTORED
#endif
}

void on_change_render_type(const tweak_info& info)
{
    xASSERT_L(1669, info.context != 0);
    xParSys& sys = *(xParSys*)info.context;
    sys.group->draw = xParSysGetRenderFunction(sys.tasset->renderFunc);
}

void on_change_attach_to(const tweak_info& info)
{
    xASSERT_L(1676, info.context != 0);
    xParEmitter& emit = *(xParEmitter*)info.context;
    U32 id = emit.tasset->attachToID;
    if (!id) emit.attachTo = NULL;
    else emit.attachTo = zSceneFindObject(id);
}

void on_change_texture(const tweak_info& info)
{
    xParSys& sys = *(xParSys*)info.context;
    xParSysAsset& asys = *sys.tasset;
    if (!asys.textureID)
        sys.txtr_particle = NULL;
    else
        sys.txtr_particle = (RwTexture*)xSTFindAsset(asys.textureID, NULL);
}

}

static void explode_tweaks(xParEmitter& emit);

static void on_expand_emitter(const tweak_info& info)
{
    xParEmitter* emit = (xParEmitter*)info.context;
    xASSERT_L(1698, emit != 0);
    explode_tweaks(*emit);
}

static void add_tweaks(xParEmitter& emit)
{
    verify_tables();

    char name[128];
    sprintf(name, "Particle Emitters|%s", get_asset_name(emit.tasset));
    static tweak_callback cb_explode = tweak_callback::create_expand(on_expand_emitter, NULL);
    xTWEAK_BRANCH(name, &cb_explode, &emit, 2);
}

static void explode_tweaks(xParEmitter& emit)
{
    const char* asset_name = get_asset_name(emit.tasset);

    char prefix[128];
    sprintf(prefix, "Particle Emitters|%s", asset_name);
    xTWEAK_BRANCH(prefix, NULL, NULL, 2);
    strcat(prefix, "|");
    char* end = prefix + strlen(prefix);

    zAUTOTWEAK_FLAG(prefix, "Active", &emit.emit_flags, 0x1, NULL, NULL, 2);
    zAUTOTWEAK_FLAG(prefix, "Attach Velocity", &emit.emit_flags, 0x8, NULL, NULL, 2);
    zAUTOTWEAK_FLAG(prefix, "Attach Visibility", &emit.emit_flags, 0x10, NULL, NULL, 2);

    strcpy(end, "\001Current|");
    zAUTOTWEAK(prefix, "\001Count", &emit.group->m_num_of_particles, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK_SELECT(prefix, "Rate Mode", &emit.rate_mode, interp_type_names, NULL, ARRAY_LENGTH(interp_type_names), NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Rate", &emit.rate, 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Rate Time", &emit.rate_time, 0.0f, 1e5f, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Rotation Yaw", &emit.rot[0], 0, 255, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Rotation Pitch", &emit.rot[1], 0, 255, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "Rotation Roll", &emit.rot[2], 0, 255, NULL, NULL, 2);
    *end = '\0';

    xASSERT_L(1740, emit.tasset != 0);
    {
        xParEmitterAsset& asset = *emit.tasset;
        static tweak_callback cb_emit_type = tweak_callback::create_change(on_change_region_type);
        static tweak_callback cb_attach_to = tweak_callback::create(on_change_attach_to);

        zAUTOTWEAK_SELECT(prefix, "\002Region\001 Type", &asset.emit_type, region_type_names, NULL, ARRAY_LENGTH(region_type_names), &cb_emit_type, &emit, 2);
        zAUTOTWEAK(prefix, "Start Location ", &asset.pos, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Start Velocity ", &asset.vel, 0, 0, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Velocity Angle Variation", &asset.vel_angle_variation, 0.0f, 2.0f * PI, NULL, NULL, 2);
        zAUTOTWEAK_SELECT(prefix, "Attach To", &asset.attachToID, all_entity_labels, all_entity_ids, all_entity_count, &cb_attach_to, &emit, 2);
        add_region_tweaks(emit, false);
    }

    if (emit.prop) {
        xParEmitterPropsAsset& props = *emit.prop;

        strcpy(end, "\001Interpolate|");
        add_interp_tweak(prefix, "Rate", props.rate);
        add_interp_tweak(prefix, "Life", props.life);
        add_interp_tweak(prefix, "Size-Birth", props.size_birth);
        add_interp_tweak(prefix, "Size-Death", props.size_death);
        add_interp_tweak(prefix, "Color-Birth 0 Red", props.color_birth[0]);
        add_interp_tweak(prefix, "Color-Birth 1 Green", props.color_birth[1]);
        add_interp_tweak(prefix, "Color-Birth 2 Blue", props.color_birth[2]);
        add_interp_tweak(prefix, "Color-Birth 3 Alpha", props.color_birth[3]);
        add_interp_tweak(prefix, "Color-Death 0 Red", props.color_death[0]);
        add_interp_tweak(prefix, "Color-Death 1 Green", props.color_death[1]);
        add_interp_tweak(prefix, "Color-Death 2 Blue", props.color_death[2]);
        add_interp_tweak(prefix, "Color-Death 3 Alpha", props.color_death[3]);
        *end = '\0';
    }

    if (emit.parSys) {
        xParSys& sys = *emit.parSys;
        xParSysAsset& asys = *sys.tasset;

        zAUTOTWEAK(prefix, "Visible", &sys.visible, 0, 1, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Priority", &asys.priority, 0, 255, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "Max Particles", &asys.maxPar, 0, 0xFFFF, NULL, NULL, 2);

        zAUTOTWEAK_SELECT(prefix, "Blend: Source", &asys.renderSrcBlendMode, tweak_blend_mode_labels, NULL, ARRAY_LENGTH(tweak_blend_mode_labels), NULL, NULL, 2);

        zAUTOTWEAK_SELECT(prefix, "Blend: Destination", &asys.renderDstBlendMode, tweak_blend_mode_labels, NULL, ARRAY_LENGTH(tweak_blend_mode_labels), NULL, NULL, 2);
        static tweak_callback cb_render_type = tweak_callback::create_change(on_change_render_type);

        zAUTOTWEAK_SELECT(prefix, "Render Type", &asys.renderFunc, render_type_names, NULL, ARRAY_LENGTH(render_type_names), &cb_render_type, &sys, 2);
        zAUTOTWEAK_FLAG(prefix, "Pivot \001Left", &asys.parFlags, 0x8, NULL, NULL, 2);
        zAUTOTWEAK_FLAG(prefix, "Pivot \002Top", &asys.parFlags, 0x10, NULL, NULL, 2);
        zAUTOTWEAK_FLAG(prefix, "Pivot \003Right", &asys.parFlags, 0x20, NULL, NULL, 2);
        zAUTOTWEAK_FLAG(prefix, "Pivot Bottom", &asys.parFlags, 0x40, NULL, NULL, 2);
        zAUTOTWEAK_FLAG(prefix, "PTank Render", &asys.parFlags, 0x80, NULL, NULL, 2);
        zAUTOTWEAK_FLAG(prefix, "Bright", &asys.parFlags, 0x100, NULL, NULL, 2);

        static tweak_callback cb_change_texture = tweak_callback::create(on_change_texture);
        zAUTOTWEAK_SELECT(prefix, "Texture", &asys.textureID, all_texture_labels, all_texture_ids, all_texture_count, &cb_change_texture, &sys, 2);

        static tweak_callback cb_cmd_count = tweak_callback::create(on_change_cmd_count);
        U32 max_commands = sys.cmdCount <= 8 ? 8 : sys.cmdCount;
        zAUTOTWEAK(prefix, "\001Command Count", &sys.cmdCount, (U32)0, max_commands - 1, &cb_cmd_count, &emit, 2);
        for (U32 i = 0; i < sys.cmdCount; i++) {
            xParCmd& cmd = sys.cmd[i];
            sprintf(end, "\001Commands|%02d|", i);
            xParCmdAddTweaks(prefix, cmd);
        }
    }
}
#endif
