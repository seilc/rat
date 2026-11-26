#include "xCommon.h"
#include "zCineFXShared.h"

#include "xColor.h"
#include "xDraw.h"
#include "xFX.h"
#include "xFont.h"
#include "xMath.h"
#include "xPad.h"
#include "xParEmitterType.h"
#include "xParGroup.h"
#include "xParSys.h"
#include "zAutoTweak.h"
#include "zCutsceneMgr.h"
#include "zGlobals.h"
#include "zParEmitter.h"

#include <rpskin.h>
#include <stdio.h>

#if DEBUG || RELEASE
void zCinFX_clamp_bone_index(S32* index, RpAtomic* model)
{
    S32 bones = 0;
    RpGeometry* geom = RpAtomicGetGeometry(model);
    if (geom) {
        RpSkin* skin = RpSkinGeometryGetSkin(geom);
        if (skin) bones = RpSkinGetNumBones(skin);
    }

    *index = range_limit(*index, 0, bones);
}

// UNUSED: needed to match (speculative code)
void zCinFX_clamp_bone_index(F32* index, RpAtomic* model)
{
    S32 bones = 0;
    RpGeometry* geom = RpAtomicGetGeometry(model);
    if (geom) {
        RpSkin* skin = RpSkinGeometryGetSkin(geom);
        if (skin) bones = RpSkinGetNumBones(skin);
    }

    S32 bone_index = range_limit((S32)(*index + 0.5f), 0, bones);
    *index = bone_index;
}
#endif

// UNUSED: needed to match (code from TSSM GC)
void zCinFX_Generic_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    xASSERT(fxrec->flg_stat & NCIN_FXF_ACTIVE);
    xASSERT(!(fxrec->flg_stat & NCIN_FXF_DONE));

    if (killit)
        fxrec->flg_stat |= NCIN_FXF_DONE;
}

// UNUSED: needed to match (code from TSSM GC)
void zCinFX_GenNCINInput_Upd(const zCutsceneMgr* csnmgr, NCINEntry* fxrec, S32 killit)
{
    xASSERT_FMT(fxrec->input.fxparm, "'%s' missing data pointer", fxrec->info.twk_name ? fxrec->info.twk_name : "?");

    if (!fxrec->input.fxparm) {
        fxrec->flg_stat |= NCIN_FXF_DONE;
        return;
    }

    zCinFX_Generic_Upd(csnmgr, fxrec, killit);
}

#if DEBUG || RELEASE
void zCinFX_IndexInput_Twk(const char* prefix_root, NCINEntry* fxrec)
{
    char prefix_base[128], prefix[128];

    sprintf(prefix_base, "%s|%s", prefix_root, "\004Index");

    IndexInput* pi = &fxrec->index;

    sprintf(prefix, "%s|", prefix_base);
    zAUTOTWEAK(prefix, "\001Cin Anim Index", &pi->idx_anim, 0, 1000, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "\002Anim Bone Index", &pi->idx_bone, 0, 1000, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "\003Anim Bone A Index", &pi->idx_boneA, 0, 1000, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "\004Anim Bone B Index", &pi->idx_boneB, 0, 1000, NULL, NULL, 2);
}

void zCinFX_BasicInput_Twk(const char* prefix_root, NCINEntry* fxrec)
{
    char prefix_base[128], prefix[128];

    sprintf(prefix_base, "%s|%s", prefix_root, "\004Basic");

    PosInput* pi = &fxrec->basic;

    sprintf(prefix, "%s|", prefix_base);
    zAUTOTWEAK(prefix, "pos_A[0] - PtA", &pi->pos_A[0], 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "pos_A[1] - PtA", &pi->pos_A[1], 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "pos_B[0] - PtB", &pi->pos_B[0], 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "pos_B[1] - PtB", &pi->pos_B[1], 0, 0, NULL, NULL, 2);
}
#endif

void zCinFX_BoneProbe_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit)
        fxrec->flg_stat |= NCIN_FXF_DONE;
    if (fxrec->flg_stat & 0x4)
        fxrec->fxdata.probedata.render = false;
}

void zCinFX_BoneProbe_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model, RwMatrix* animMat, U32 animIndex,
                         U32, U32&)
{
    if (!fxrec->fxdata.probedata.render || animIndex != fxrec->index.idx_anim)
        return;

    xVec3 pnt1, pnt2, offset;

    S32 numBones = 0;
    RpGeometry* geom = RpAtomicGetGeometry(model);
    if (geom) {
        RpSkin* skin = RpSkinGeometryGetSkin(geom);
        if (skin) numBones = RpSkinGetNumBones(skin);
    }

    S32 boneIndex = fxrec->index.idx_bone;
    if (boneIndex < 0) {
        boneIndex = 0;
        fxrec->index.idx_bone = boneIndex;
    }
    if (boneIndex > numBones) {
        boneIndex = numBones;
        fxrec->index.idx_bone = boneIndex;
    }

    xMat3x3RMulVec(&pnt1, (xMat3x3*)&animMat[boneIndex], &fxrec->basic.pos_A[0]);
    xVec3AddTo(&pnt1, (xVec3*)&animMat[boneIndex].pos);
    if (boneIndex != 0)
        xVec3AddTo(&pnt1, (xVec3*)&animMat[0].pos);

    fxrec->basic.pos_A[1] = pnt1;

    xVec3Copy(&offset, &fxrec->basic.pos_A[0]);
    offset.x += 3.0f;
    xMat3x3RMulVec(&pnt2, (xMat3x3*)&animMat[boneIndex], &offset);
    xVec3AddTo(&pnt2, (xVec3*)&animMat[boneIndex].pos);
    if (boneIndex != 0)
        xVec3AddTo(&pnt2, (xVec3*)&animMat[0].pos);
    xDrawSetColor(g_RED);
    xDrawLine(&pnt1, &pnt2);

    offset.x -= 3.0f;
    offset.y += 3.0f;
    xMat3x3RMulVec(&pnt2, (xMat3x3*)&animMat[boneIndex], &offset);
    xVec3AddTo(&pnt2, (xVec3*)&animMat[boneIndex].pos);
    if (boneIndex != 0)
        xVec3AddTo(&pnt2, (xVec3*)&animMat[0].pos);
    xDrawSetColor(g_GREEN);
    xDrawLine(&pnt1, &pnt2);

    offset.y -= 3.0f;
    offset.z += 3.0f;
    xMat3x3RMulVec(&pnt2, (xMat3x3*)&animMat[boneIndex], &offset);
    xVec3AddTo(&pnt2, (xVec3*)&animMat[boneIndex].pos);
    if (boneIndex != 0)
        xVec3AddTo(&pnt2, (xVec3*)&animMat[0].pos);
    xDrawSetColor(g_BLUE);
    xDrawLine(&pnt1, &pnt2);

    xVec3Copy(&pnt1, &fxrec->basic.pos_B[0]);
    xVec3Copy(&pnt2, &pnt1);
    pnt2.x += 3.0f;
    xDrawSetColor(g_RED);
    xDrawLine(&pnt1, &pnt2);

    pnt2.x -= 3.0f;
    pnt2.y += 3.0f;
    xDrawSetColor(g_GREEN);
    xDrawLine(&pnt1, &pnt2);

    pnt2.y -= 3.0f;
    pnt2.z += 3.0f;
    xDrawSetColor(g_BLUE);
    xDrawLine(&pnt1, &pnt2);
}

#if DEBUG || RELEASE
void zCinFX_BoneProbe_Twk(const char* prefix_root, NCINEntry* fxrec)
{
    char prefix_base[128], prefix[128];

    sprintf(prefix_base, "%s|%s", prefix_root, "Probe");

    PosInput* pi = &fxrec->basic;

    sprintf(prefix, "%s|", prefix_base);
    zAUTOTWEAK(prefix, "pos_A[0] - Bone Local Offset", &pi->pos_A[0], 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "pos_A[1] - unused", &pi->pos_A[1], 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "pos_B[0] - World Position", &pi->pos_B[0], 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "pos_B[1] - unused", &pi->pos_B[1], 0, 0, NULL, NULL, 2);
    zAUTOTWEAK(prefix, "render", &fxrec->fxdata.probedata.render, 0, 0, NULL, NULL, 2);
}
#endif

void zCinFX_EntityBonePar_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model, RwMatrix* animMat,
                             U32 animIndex, U32, U32&)
{
    if (animIndex != fxrec->index.idx_anim)
        return;

    zCinFX_clamp_bone_index(&fxrec->index.idx_bone, model);

    F32 dt = globals.update_dt;
    xParEmitter* pe = fxrec->fxdata.pardata.emitter;
    if (!pe)
        return;

    xParEmitterAsset* a = pe->tasset;
    xParEmitterPropsAsset* prop = pe->prop;
    xParSys* ps = pe->parSys;
    if (!ps)
        return;
    xParGroup* g = ps->group;
    if (!g)
        return;
    if (a->emit_type != eParRegionEntBone)
        return;

    pe->rate_time += dt;
    S32 rate_has_elapsed = pe->rate_time > prop->rate.freq;

    if (prop->rate.freq == 0.0f)
        pe->rate_time = 0.0f;
    while (pe->rate_time > prop->rate.freq)
        pe->rate_time -= prop->rate.freq;

    F32 rate = xParInterpCompute(pe->rate_mode, &prop->rate, pe->rate_time, rate_has_elapsed, pe->rate);

    pe->rate = rate;

    pe->rate_fraction += rate * dt;
    pe->rate_fraction_cull += rate * dt;

    S32 count = std::floorf(pe->rate_fraction);
    if (count <= 0)
        return;
    pe->rate_fraction -= count;

    if (ps->tasset->maxPar) {
        if (g->m_num_of_particles >= ps->tasset->maxPar)
            return;
        if (ps->group->m_num_of_particles + count >= ps->tasset->maxPar)
            count = ps->tasset->maxPar - ps->group->m_num_of_particles;
    }

    F32 life = prop->life.val[0], size_birth = prop->size_birth.val[0], size_death = prop->size_death.val[0];

    xMat4x3 bone_mat;
    zCinFX_get_bone_matrix(bone_mat, animMat, fxrec->index.idx_bone);
    xVec3 oldloc = fxrec->basic.pos_B[0], loc, vel;
    xParEmitterTransformEntBone(loc, vel, *a, bone_mat);
    fxrec->basic.pos_B[0] = loc;
    vel *= dt;

    xVec3 add_vel;
    if (pe->emit_flags & 0x8) {
        add_vel.Sub(loc, oldloc);

        if (add_vel.length2() > 25.0f) add_vel = 0.0f;
    } else
        add_vel = 0.0f;

    for (S32 i = 0; i < count; i++) {
        xPar* p = xParGroupAddPar(ps->group);
        if (!p)
            return;

        life = xParInterpCompute(prop->life.interp, &prop->life, pe->rate_time, TRUE, 0.0f);

        size_birth = xParInterpCompute(prop->size_birth.interp, &prop->size_birth, pe->rate_time, TRUE, 0.0f);

        size_death = xParInterpCompute(prop->size_death.interp, &prop->size_death, pe->rate_time, TRUE, 0.0f);

        p->m_lifetime = life;
        p->totalLifespan = life;

        p->m_size = size_birth;
        p->m_sizeVel = (size_death - size_birth) / life;

        p->m_flag = 0;

        p->m_rotdeg[0] = pe->rot[0];
        p->m_rotdeg[1] = pe->rot[1];
        p->m_rotdeg[2] = pe->rot[2];

        for (S32 c = 0; c < 4; c++) {
            F32 fc1 = xParInterpCompute(prop->color_birth[c].interp, &prop->color_birth[c], pe->rate_time, TRUE, 0.0f);

            F32 fc2 = xParInterpCompute(prop->color_death[c].interp, &prop->color_death[c], pe->rate_time, TRUE, 0.0f);

            p->m_cfl[c] = fc1;
            p->m_c[c] = fc1;
            p->m_cvel[c] = (fc2 - fc1) / life;
        }

        p->m_pos = loc;

        xParEmitterEmitSetTexIdxs(p, ps);

        p->m_vel = vel;
        xParEmitterEmitEntBone(p, a, dt, bone_mat);
        p->m_vel += add_vel;
    }
}
