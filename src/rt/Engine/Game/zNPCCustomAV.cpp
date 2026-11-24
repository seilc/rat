#include "xCommon.h"
#include "zNPCCustomAV.h"

#include "containers.h"
#include "xAnim.h"
#include "xCam.h"
#include "xEntMotion.h"
#include "xEvent.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xModel.h"
#include "xMovePointAsset.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "zAnimCustom.h"
#include "zEvent.h"
#include "zNPCCommon.h"

void zNPCCustomAV::Init(xBase& data, xDynAsset& dynasset, size_t asset_size)
{
    xDynAssetValidate_L(26, dynasset, asset_size, zNPCCustomAVAsset, "npc:NPC_Custom_AV");

    xBaseInit(&data, &dynasset);

    zNPCCustomAV* newAV = (zNPCCustomAV*)&data;

    newAV->asset = (zNPCCustomAVAsset*)&dynasset;
    newAV->eventFunc = NULL;

    if (newAV->linkCount)
        newAV->link = (xLinkAsset*)(newAV->asset + 1);
    else
        newAV->link = NULL;
}

void zNPCCustomAV::Reset()
{
    animDelay = asset->animDelay;
    soundDelay = asset->soundDelay;
    animLoopTime = asset->loopTime;

    if (asset->animID)
        animState = eCUSTOMAVSTATE_Waiting;
    else
        animState = eCUSTOMAVSTATE_Finished;

    if (asset->soundGroupID)
        soundState = eCUSTOMAVSTATE_Waiting;
    else
        soundState = eCUSTOMAVSTATE_Finished;

    if (asset->faceTargetID > 1)
        movepointAsset = (xMovePointAsset*)xSTFindAsset(asset->faceTargetID, NULL);
    else
        movepointAsset = NULL;
}

void zNPCCustomAV::Update(zNPC::zNPCCommon* npc, F32 dt)
{
    switch (animState) {
    case eCUSTOMAVSTATE_Waiting:
        animDelay -= dt;
        if (animDelay <= 0.0f) {
            bool loopThatAnim = false;
            if (animLoopTime > 0.0f)
                loopThatAnim = true;
            zAnimCustomPlay(asset->animID, npc->model, 0, loopThatAnim, asset->animBlendTime, 0.0f, 1.0f);

            xASSERT_L(85, asset->speedMin <= asset->speedMax);
            F32 speed = xrand_RandomFloatRange(asset->speedMin, asset->speedMax);
            npc->model->Anim->Single->CurrentSpeed = speed;

            if (asset->randomStartTime) {
                F32 newTime = xrand_RandomFloatRange(0.0f, npc->model->Anim->Single->State->Data->Duration);
                npc->model->Anim->Single->Time = newTime;
            }

            animState = eCUSTOMAVSTATE_Playing;
            zEntEvent(this, eEventCustomAVAnimStarted);
        }
        break;
    case eCUSTOMAVSTATE_Playing:
    {
        xAnimState* astate = npc->model->Anim->Single->State;
        F32 duration = astate->Data->Duration;
        if (npc->model->Anim->Single->Time + dt >= duration) {
            if (animLoopTime > 0.0f)
                animState = eCUSTOMAVSTATE_Looping;
            else
                animState = eCUSTOMAVSTATE_Finished;

            zEntEvent(this, eEventCustomAVAnimFinished);
        }
        break;
    }
    case eCUSTOMAVSTATE_Looping:
        animLoopTime -= dt;
        if (animLoopTime <= 0.0f) {
            animState = eCUSTOMAVSTATE_Finished;
        }
        break;
    case eCUSTOMAVSTATE_Finished:
        break;
    }

    switch (soundState) {
    case eCUSTOMAVSTATE_Waiting:
        soundDelay -= dt;
        if (soundDelay <= 0.0f) {
            soundGroupHandle = xSndMgrGetSoundGroup(asset->soundGroupID);
            if (soundGroupHandle) {
                soundHandle = xSndMgrPlay(soundGroupHandle, 0, NULL, NULL, NULL, NULL, NULL);
            }

            soundState = eCUSTOMAVSTATE_Playing;
            zEntEvent(this, eEventCustomAVSoundStarted);
        }
        break;
    case eCUSTOMAVSTATE_Playing:
        if (!asset->waitOnSound) {
            soundState = eCUSTOMAVSTATE_Finished;
        }
        else if (!xSndMgrIsPlaying(soundGroupHandle)) {
            soundState = eCUSTOMAVSTATE_Finished;
            zEntEvent(this, eEventCustomAVSoundFinished);
        }
        break;
    case eCUSTOMAVSTATE_Looping:
        xASSERT_L(160, 0);
        break;
    case eCUSTOMAVSTATE_Finished:
        break;
    }
}

bool zNPCCustomAV::HasCompleted()
{
    if (animState == eCUSTOMAVSTATE_Finished && soundState == eCUSTOMAVSTATE_Finished) {
        zEntEvent(this, eEventCustomAVFinished);
        return true;
    }

    return false;
}

bool zNPCCustomAV::TurnToTarget()
{
    if (asset->faceTargetID == 0)
        return false;
    return true;
}

bool zNPCCustomAV::UsePlayerTarget()
{
    if (asset->faceTargetID == 1)
        return true;
    return false;
}

void zNPCCustomAV::GetMovePointTargetPos(xVec3* outPos)
{
    xASSERT_L(199, movepointAsset);
    *outPos = movepointAsset->pos;
}

// UNUSED: needed to match (code from RotU GC)
void zNPCCustomAV::SetSpawnCustomAV(U32 id)
{
    if (asset->animBlendTime != 0.0f) {
        DBprintf(DBML_WARN, "SetSpawnCustomAV, blend time for %s was %f, forcing to 0.0f\n", xSTAssetName(id),
                 asset->animBlendTime);
    }
    asset->animBlendTime = 0.0f;
}
