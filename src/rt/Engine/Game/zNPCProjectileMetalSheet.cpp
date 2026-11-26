#include "xCommon.h"
#include "zNPCProjectileMetalSheet.h"

#include "xClumpColl.h"
#include "xCollide.h"
#include "xCollideSweptSphere.h"
#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xModel.h"
#include "xSndMgr.h"
#include "xSpacePartitionTree.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zEnv.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zNPCCommon.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zShrapnel.h"

#include <string.h>

namespace
{
    xTWEAKABLE F32 OFFSET_X = 1.9f;
    xTWEAKABLE F32 OFFSET_Y = 0.03f;
    xTWEAKABLE F32 OFFSET_Z = 1.3f;
    xTWEAKABLE F32 METAL_SHEET_SHADOW_R = 1.0f;
    xTWEAKABLE U8 METAL_SHEET_SHADOW_STRENGTH = 0x50;
}

RpAtomic* zNPCProjectileMetalSheet::modelFile;
xBound zNPCProjectileMetalSheet::modelOOBBound;
F32 zNPCProjectileMetalSheet::radius;
zShrapnelAsset* zNPCProjectileMetalSheet::explosion_shrapnel;

void zNPCProjectileMetalSheet::Reset()
{
    Destroy();
}

void zNPCProjectileMetalSheet::PlayDestroySound()
{
    if (model) {
        xSndGroupHandle soundGroupHandle = xSndMgrGetSoundGroup(HASH_TANK_SHEET_DESTROY_SOUND_GROUP);
        if (soundGroupHandle) {
            xMat4x3* modelFrame = (xMat4x3*)model->Mat;
            xSndMgrPlay(soundGroupHandle, 0, &modelFrame->pos, NULL, NULL, NULL, NULL);
        }
    }
}

void zNPCProjectileMetalSheet::Update(F32 dt)
{
    switch (currentState) {
    case 0:
    {
        xMat4x3* modelFrame = (xMat4x3*)model->Mat;
        xMat4x3Copy(modelFrame, &((xMat4x3*)owner->model->Mat)[attachBone]);
        xMat4x3Mul(modelFrame, modelFrame, (xMat4x3*)owner->model->Mat);

        xVec3 offset = { OFFSET_X, OFFSET_Y, OFFSET_Z };
        xMat3x3RMulVec(&offset, modelFrame, &offset);
        modelFrame->pos.Add(offset);

        xVec3Copy(&prevPrevPosition, &prevPosition);
        xVec3Copy(&prevPosition, &modelFrame->pos);
        break;
    }
    case 1:
    {
        lifetime -= dt;
        if (lifetime <= 0.0f) {
            PlayDestroySound();
            Destroy();
            return;
        }

        xMat4x3* modelFrame = (xMat4x3*)model->Mat;
        xMat3x3LMulRotY(modelFrame, modelFrame, 40.0f / PI * dt);

        xVec3 dpos;
        dpos.Scale(direction, 10.0f * dt);
        Collide(dpos, dt);
        if (!IsActive()) {
            return;
        }

        modelFrame->pos.Add(dpos);
        xBoundUpdate(&bound);
        break;
    }
    }

    xMat4x3 shadowMat;
    xMat4x3Copy(&shadowMat, (xMat4x3*)model->Mat);
    xShadowSimple_Add(&simpShadow, &shadowMat, METAL_SHEET_SHADOW_R, 1.0f);
    position = ((xMat4x3*)model->Mat)->pos;
}

void zNPCProjectileMetalSheet::Render()
{
    xModelRender(model);
}

void zNPCProjectileMetalSheet::Destroy()
{
    active = false;
    if (model) {
        xModelInstanceFree_L(model, true, 118);
        model = NULL;
        modelID = 0;
    }

    if (sheetSoundHandle != XSNDMGR_INVALID_SND_HANDLE) {
        xSndMgrStop(sheetSoundHandle);
        sheetSoundHandle = XSNDMGR_INVALID_SND_HANDLE;
    }
}

bool zNPCProjectileMetalSheet::Setup(void* configData)
{
    xASSERT_L(133, configData);
    MetalSheetConfig* config = (MetalSheetConfig*)configData;

    owner = config->owner;
    if (!model) {
        model = xModelInstanceAlloc(modelFile, NULL, 0, 0, NULL);
        xASSERT_FMT_L(140, model || xModelPoolStressTesting(), "Can't allocate model for metal sheet. Will fail gracefully.");
        if (!model)
            return false;
        xModelInstanceSetID_L(143, model, HASH_fx_tank_sheet_a);

        zEnv* zen = globals.sceneCur->zen;
        xEnvAsset* easset = zen->easset;

        xASSERT_L(148, easset->objectLightKit);
        xModelSetLightKit(model, (xLightKit*)xSTFindAsset(easset->objectLightKit, NULL), 0.0f);
    }

    xASSERT_L(152, model);

    lifetime = 15.0f;

    if (model) {
        xMat4x3* modelFrame = (xMat4x3*)model->Mat;
        xMat4x3Copy(modelFrame, &config->startMat);
        xVec3Copy(&prevPrevPosition, &modelFrame->pos);
        xVec3Copy(&prevPosition, &modelFrame->pos);
        bound = modelOOBBound;
        bound.mat = modelFrame;
        attachBone = config->attachBone;
    }

    xShadowSimple_CacheInit(&simpShadow, METAL_SHEET_SHADOW_STRENGTH);

    currentState = 0;
    active = true;
    return true;
}

// UNUSED: needed to match (speculative code)
void zNPCProjectileMetalSheet::GetCurrentDirection(xVec3& dir)
{
    dir.Sub(prevPosition, prevPrevPosition);
    dir.y = direction.y;
    dir.safe_normalize(direction);
}

void zNPCProjectileMetalSheet::Collide(const xVec3& dpos, F32)
{
    xVec3 centers[8] = {
        { bound.box.box.upper.x - 0.1f, 0.0f, bound.box.box.upper.z - 0.1f },
        { bound.box.box.upper.x - 0.1f, 0.0f, bound.box.box.lower.z + 0.1f },
        { bound.box.box.lower.x + 0.1f, 0.0f, bound.box.box.upper.z - 0.1f },
        { bound.box.box.lower.x + 0.1f, 0.0f, bound.box.box.lower.z + 0.1f },
        { bound.box.box.upper.x - 0.1f, 0.0f, 0.0f },
        { bound.box.box.lower.x + 0.1f, 0.0f, 0.0f },
        { 0.0f, 0.0f, bound.box.box.upper.z - 0.1f },
        { 0.0f, 0.0f, bound.box.box.lower.z + 0.1f },
    };

    xMat4x3* modelFrame = (xMat4x3*)model->Mat;
    for (U32 k = 0; k < ARRAY_LENGTH(centers); k++) {
        xMat4x3ToWorld(&centers[k], modelFrame, &centers[k]);

        xSweptSphere sws;
        memset(&sws, 0, sizeof(sws));

        xVec3 start, end;
        xVec3Copy(&start, &centers[k]);
        end.Add(start, dpos);

        xSweptSpherePrepare(&sws, &start, &end, 0.1f, 1);

        for (S32 i = 0; i < CURPLAYERS.size(); i++) {
            if (CURPLAYERS[i]->Get_defeated()) {
                continue;
            }

            const xSphere* extra_spheres[zPlayer::MAX_EXTRA_SPHERES];
            S32 num_spheres = CURPLAYERS[i]->GetCollisionSpheres(extra_spheres);
            for (S32 j = 0; j < num_spheres; j++) {
                if (xSweptSphereToSphere(&sws, extra_spheres[j])) {
                    Hit();
                    return;
                }
            }
        }

        if (xSweptSphereToStatDyn(&sws, xglobals->sceneCur, owner, k_XENT_COLLTYPE_PC, 0)) {
            Hit();
            return;
        }

        U32 previousFlags = xClumpColl_SetFilterFlags(0, 8, 8);
        S32 hitEnv = xSweptSphereToEnv(&sws, xglobals->sceneCur->env);
        xClumpColl_RestoreFilterFlags(previousFlags);

        if (hitEnv) {
            Hit();
            return;
        }
    }
}

void zNPCProjectileMetalSheet::Hit()
{
    xVec3 vel = { 0.0f, 0.0f, 0.0f };
    if (explosion_shrapnel)
        explosion_shrapnel->initCB(explosion_shrapnel, model, NULL, &vel, NULL);
    PlayDestroySound();
    Destroy();
}

void zNPCProjectileMetalSheet::StaticSetup()
{
    modelFile = xModelFindAtomic(HASH_fx_tank_sheet_a, NULL);
    if (modelFile) {
        xModelPoolStressTestDisable(eModelPoolStressor_NPCProjectileInit);
        modelOOBBound.type = k_XBOUNDTYPE_OBB;
        xModelInstance* tempInst = xModelInstanceAlloc(modelFile, NULL, 0, 0, NULL);
        xModelInstanceSetID_L(295, tempInst, HASH_fx_tank_sheet_a);
        xBoxForModel(modelOOBBound.box.box, tempInst, true);
        xModelInstanceFree_L(tempInst, true, 297);
        xModelPoolStressTestEnable(eModelPoolStressor_NPCProjectileInit);
        radius = 0.5f * modelOOBBound.box.box.upper.Distance(modelOOBBound.box.box.lower);
        explosion_shrapnel = (zShrapnelAsset*)xSTFindAsset(HASH_tank_sheet_explode_shrapnel, NULL);
    }
}

#if DEBUG || RELEASE
void zNPCProjectileMetalSheet::AddStaticTweaks()
{
}
#endif
