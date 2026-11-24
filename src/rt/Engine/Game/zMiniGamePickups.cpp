#include "xCommon.h"
#include "zMiniGamePickups.h"

#include "xGroup.h"
#include "xModel.h"
#include "xModelBucket.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "zEnv.h"
#include "zOtherAsset.h"

void zMiniGamePickupController::Setup(xGroup* inGroup, F32 spawnTime, F32 showTime, bool resetTypes)
{
    xASSERT_L(18, inGroup);

    spawnGroup = inGroup;
    initialSpawnTime = currentSpawnTime = spawnTime;
    initialShowTime = currentShowTime = showTime;

    if (resetTypes)
        typeCount = 0;
    state = 0;
}

void zMiniGamePickupController::SetAvailablePickup(S32 idx)
{
    if (available.modelInstance) {
        xModelInstanceFree_L(available.modelInstance, true, 33);
        available.modelInstance = NULL;
    }

    U32 modelID = type[idx].modelID;
    RpAtomic* imodel = (RpAtomic*)xSTFindAsset(modelID, NULL);
    if (imodel) {
        available.modelInstance = xModelInstanceAlloc(imodel, NULL, 0, 0, NULL);
        xModelInstanceCheckAlloc_L(42, available.modelInstance, xSTAssetName(modelID)); if (available.modelInstance) available.modelInstance->modelID = modelID;
        xModelSetLightKit(available.modelInstance, zEnvGetGlobalObjectLightKit(), 0.0f);
    }

    available.idx = idx;

    pointer_asset* pointer = (pointer_asset*)xGroupGetAnyRandomPtr(spawnGroup);
    xASSERT_L(51, pointer);
    xMat4x3Identity(&available.position);
    available.position.pos = pointer->loc;
}

void zMiniGamePickupController::Update(F32 dt)
{
    switch (state) {
    case 0:
        currentSpawnTime -= dt;
        if (currentSpawnTime <= 0.0f) {
            currentShowTime = initialShowTime;
            state = 1;
            SetAvailablePickup(GetRandomType());
        }
        break;
    case 1:
        currentShowTime -= dt;
        if (currentShowTime <= 0.0f) {
            state = 0;
            currentSpawnTime = initialSpawnTime;
        }
        break;
    }
}

void zMiniGamePickupController::AddType(S32 id, F32 likelyhood, F32 activeTime, U32 modelID, U32 soundID)
{
    xASSERT_L(94, typeCount < MAX_PICKUP_TYPES);
    type[typeCount].id = id;
    type[typeCount].likelyhood = likelyhood;
    type[typeCount].activeTime = activeTime;
    type[typeCount].soundID = soundID;
    type[typeCount].modelID = modelID;
    typeCount++;
}

void zMiniGamePickupController::Reset()
{
    currentSpawnTime = initialSpawnTime;
    state = 0;
}

void zMiniGamePickupController::Render()
{
    if (state == 1) {
        if (available.modelInstance) {
            xMat4x3Copy((xMat4x3*)available.modelInstance->Mat, &available.position);
            xModelBucket_Add(available.modelInstance, true);
        }
    }
}

S32 zMiniGamePickupController::GetRandomType()
{
    S32 typeRangeIndices[MAX_PICKUP_TYPES], inRangeCount;

    do {
        F32 randomValue = xurand();
        inRangeCount = 0;

        for (S32 i = 0; i < typeCount; i++) {
            if (randomValue <= type[i].likelyhood) {
                typeRangeIndices[inRangeCount++] = i;
            }
        }
    } while (inRangeCount <= 0);

    S32 pickedIdx = xrand_RandomChoice(inRangeCount);
    xASSERT_L(154, pickedIdx < inRangeCount);

    return typeRangeIndices[pickedIdx];
}

bool zMiniGamePickupController::Test(xVec3* pos, F32 radius, S32* availableIdx)
{
    if (state == 1) {
        F32 dist = pos->Distance(available.position.pos);
        if (dist <= radius) {
            zMiniGamePickupType* testType = &type[available.idx];
            *availableIdx = testType->id;
            state = 0;
            currentSpawnTime = initialSpawnTime;

            if (testType->soundID) {
                xSndGroupHandle soundGroupHandle = xSndMgrGetSoundGroup(testType->soundID);
                xASSERT_L(177, soundGroupHandle);
                if (soundGroupHandle) {
                    xSndMgrPlay(soundGroupHandle, 0, pos, NULL, NULL, NULL, NULL);
                }
            }

            return true;
        }
    }

    return false;
}
