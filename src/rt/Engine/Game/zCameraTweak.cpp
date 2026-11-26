#include "xCommon.h"
#include "zCameraTweak.h"

#include "xMath.h"
#include "zBase.h"
#include "zCam.h"
#include "zEvent.h"

#define MAX_NESTED_CAM_TWEAKS 8

struct zCamTweak
{
    U32 owner;
    F32 priority;
    F32 time;
    F32 pitch;
    F32 distMult;
    F32 pivotHeight;
    F32 FOV;
};

static S32 sCamTweakCount;
static zCamTweak sCamTweakList[MAX_NESTED_CAM_TWEAKS];
static F32 sCamTweakLerp;
static F32 sCamTweakTime;
static F32 sCamTweakPitch[2];
static F32 sCamTweakDistMult[2];
static F32 sCamTweakPivotHeight[2];
static F32 sCamTweakFOV[2];
static F32 sCamTweakPitchCur;
static F32 sCamTweakDistMultCur;
static F32 sCamTweakPivotHeightCur;
static F32 sCamTweakFOVCur;

void zCameraTweakGlobal_Init()
{
    zCameraTweakGlobal_Reset();
}

void zCameraTweakGlobal_Add(U32 owner, F32 priority, F32 time, F32 pitch, F32 distMult, F32 pivotHeight, F32 fov)
{
    xASSERT_L(126, sCamTweakCount < MAX_NESTED_CAM_TWEAKS);

    S32 i;
    for (i = 0; i < sCamTweakCount; i++) {
        if (owner == sCamTweakList[i].owner) {
            return;
        }
    }

    for (i = 0; i < sCamTweakCount; i++) {
        if (priority >= sCamTweakList[i].priority) {
            for (S32 j = sCamTweakCount; j >= i + 1; j--) {
                sCamTweakList[j] = sCamTweakList[j - 1];
            }
            break;
        }
    }

    sCamTweakList[i].owner = owner;
    sCamTweakList[i].priority = priority;
    sCamTweakList[i].time = MAX(time, 0.001f);
    sCamTweakList[i].pitch = (PI / 180.0f) * pitch;
    if (distMult == 0.0f)
        sCamTweakList[i].distMult = zCamGetDefault()->get_rest_dist_default();
    else
        sCamTweakList[i].distMult = distMult;
    sCamTweakList[i].pivotHeight = pivotHeight;
    sCamTweakList[i].FOV = (PI / 180.0f) * fov;
    sCamTweakCount++;

    if (i == 0) {
        sCamTweakPitch[1] = sCamTweakPitch[1] * sCamTweakLerp + sCamTweakPitch[0] * (1.0f - sCamTweakLerp);
        sCamTweakDistMult[1] = sCamTweakDistMult[1] * sCamTweakLerp + sCamTweakDistMult[0] * (1.0f - sCamTweakLerp);
        // BUG: reads sCamTweakPitch[1], not sCamTweakPivotHeight[1]
        sCamTweakPivotHeight[1] = sCamTweakPitch[1] * sCamTweakLerp + sCamTweakPivotHeight[0] * (1.0f - sCamTweakLerp);
        sCamTweakFOV[1] = sCamTweakFOV[1] * sCamTweakLerp + sCamTweakFOV[0] * (1.0f - sCamTweakLerp);

        sCamTweakLerp = 1.0f;
        sCamTweakTime = sCamTweakList[0].time;
        sCamTweakPitch[0] = sCamTweakList[0].pitch;
        sCamTweakDistMult[0] = sCamTweakList[0].distMult;
        sCamTweakPivotHeight[0] = sCamTweakList[0].pivotHeight;
        sCamTweakFOV[0] = sCamTweakList[0].FOV;
    }
}

void zCameraTweakGlobal_Remove(U32 owner)
{
    for (S32 i = 0; i < sCamTweakCount; i++) {
        if (owner == sCamTweakList[i].owner) {
            if (i == 0) {
                sCamTweakPitch[1] = sCamTweakPitch[1] * sCamTweakLerp + sCamTweakPitch[0] * (1.0f - sCamTweakLerp);
                sCamTweakDistMult[1] = sCamTweakDistMult[1] * sCamTweakLerp + sCamTweakDistMult[0] * (1.0f - sCamTweakLerp);
                sCamTweakPivotHeight[1] = sCamTweakPivotHeight[1] * sCamTweakLerp + sCamTweakPivotHeight[0] * (1.0f - sCamTweakLerp);
                sCamTweakFOV[1] = sCamTweakFOV[1] * sCamTweakLerp + sCamTweakFOV[0] * (1.0f - sCamTweakLerp);

                sCamTweakLerp = 1.0f;
                sCamTweakTime = sCamTweakList[0].time;
                if (sCamTweakCount > 1) {
                    sCamTweakPitch[0] = sCamTweakList[1].pitch;
                    sCamTweakDistMult[0] = sCamTweakList[1].distMult;
                    sCamTweakPivotHeight[0] = sCamTweakList[1].pivotHeight;
                    sCamTweakFOV[0] = sCamTweakList[1].FOV;
                } else {
                    sCamTweakPitch[0] = 0.0f;
                    sCamTweakDistMult[0] = zCamGetDefault()->get_rest_dist_default();
                    sCamTweakPivotHeight[0] = 0.0f;
                    sCamTweakFOV[0] = 0.0f;
                }
            }

            for (S32 j = i; j < sCamTweakCount - 1; j++) {
                sCamTweakList[j] = sCamTweakList[j + 1];
            }
            sCamTweakCount--;
            break;
        }
    }
}

void zCameraTweakGlobal_Reset()
{
    sCamTweakCount = 0;
    sCamTweakPitch[0] = 0.0f;
    sCamTweakPitch[1] = 0.0f;
    sCamTweakDistMult[0] = zCamGetDefault()->get_rest_dist_default();
    sCamTweakDistMult[1] = zCamGetDefault()->get_rest_dist_default();
    sCamTweakTime = 0.1f;
    sCamTweakLerp = 0.0f;
    sCamTweakPitchCur = 0.0f;
    sCamTweakDistMultCur = zCamGetDefault()->get_rest_dist_default();
    sCamTweakPivotHeightCur = 0.0f;
    sCamTweakFOVCur = 0.0f;
}

void zCameraTweakGlobal_Update(F32 dt)
{
    sCamTweakLerp -= dt / sCamTweakTime;
    if (sCamTweakLerp < 0.0f) {
        sCamTweakLerp = 0.0f;
    }

    sCamTweakPitchCur = sCamTweakPitch[1] * sCamTweakLerp + sCamTweakPitch[0] * (1.0f - sCamTweakLerp);
    sCamTweakDistMultCur = sCamTweakDistMult[1] * sCamTweakLerp + sCamTweakDistMult[0] * (1.0f - sCamTweakLerp);
    sCamTweakPivotHeightCur = sCamTweakPivotHeight[1] * sCamTweakLerp + sCamTweakPivotHeight[0] * (1.0f - sCamTweakLerp);
    sCamTweakFOVCur = sCamTweakFOV[1] * sCamTweakLerp + sCamTweakFOV[0] * (1.0f - sCamTweakLerp);

    zCamGetDefault()->set_rest_dist(sCamTweakDistMultCur, false, 0.0f);
}

F32 zCameraTweakGlobal_GetPitch()
{
    return sCamTweakPitchCur;
}

F32 zCameraTweakGlobal_GetPivotHeight()
{
    return sCamTweakPivotHeightCur;
}

F32 zCameraTweakGlobal_GetFOV()
{
    return sCamTweakFOVCur;
}

void zCameraTweak_Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(326, asset, asset_size, CameraTweak_asset, "game_object:Camera_Tweak");
    zCameraTweak_Init((zCameraTweak*)&data, (CameraTweak_asset*)&asset);
}

void zCameraTweak_Init(zCameraTweak* tweak, CameraTweak_asset* asset)
{
    xBaseInit(tweak, asset);

    xASSERT_L(336, tweak);
    xASSERT_L(337, asset);
    xASSERT_L(338, tweak->baseType == eBaseTypeCameraTweak);

    CameraTweak_asset* casset = asset;
    tweak->casset = casset;
    tweak->eventFunc = zCameraTweak_EventCB;

    if (tweak->linkCount)
        tweak->link = (xLinkAsset*)(casset + 1);
    else
        tweak->link = NULL;
}

void zCameraTweak_Save(zCameraTweak* tweak, xSerial* s)
{
    xBaseSave(tweak, s);
}

void zCameraTweak_Load(zCameraTweak* tweak, xSerial* s)
{
    xBaseLoad(tweak, s);
}

void zCameraTweak_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xASSERT_L(370, to);

    zCameraTweak* tweak = (zCameraTweak*)to;

    switch (toEvent) {
    case eEventEnable:
        xBaseEnable(tweak);
        break;
    case eEventDisable:
        xBaseDisable(tweak);
        break;
    case eEventRun:
        if (xBaseIsEnabled(tweak)) {
            zCameraTweakGlobal_Add(
                tweak->id, tweak->casset->priority, tweak->casset->time, tweak->casset->pitch_adjust,
                tweak->casset->dist_adjust, tweak->casset->pivot_height_adjust, tweak->casset->fov_adjust);
        }
        break;
    case eEventStop:
        if (xBaseIsEnabled(tweak)) {
            zCameraTweakGlobal_Remove(tweak->id);

            if (tweak->casset->dist_adjust > 0.0f) {
                zCamPlayer::rest_config tempCfg;
                zCamGetDefault()->get_rest_default(tempCfg);
                zCamGetDefault()->set_rest(tempCfg, false, tweak->casset->time);
            }
        }
        break;
    }
}
