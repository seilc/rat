#include "xCommon.h"
#include "zCameraFly.h"

#include "xGlobals.h"
#include "xMath.h"
#include "xScrFx.h"
#include "xstransvc.h"
#include "zBase.h"
#include "zCam.h"
#include "zEvent.h"
#include "zPlayer.h"
#include "zScene.h"

void zCameraFly_Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(34, asset, asset_size, CameraFly_asset, "game_object:Flythrough");
    zCameraFly_Init((zCameraFly*)&data, (CameraFly_asset*)&asset);
}

void zCameraFly_Init(zCameraFly* fly, CameraFly_asset* asset)
{
    xBaseInit(fly, asset);

    xASSERT_L(44, fly);
    xASSERT_L(45, asset);
    xASSERT_L(46, fly->baseType == eBaseTypeCameraFly);

    CameraFly_asset* casset = asset;
    fly->casset = casset;
    fly->eventFunc = zCameraFlyEventCB;

    if (fly->linkCount)
        fly->link = (xLinkAsset*)(casset + 1);
    else
        fly->link = NULL;
}

void zCameraFly_Setup(zCameraFly* fly)
{
    fly->baseFlags |= k_XBASE_IS_PERSISTENT;
}

void zCameraFly_Update(xBase* to, xScene* sc, F32)
{
    xASSERT_L(67, to);
    xASSERT_L(68, sc);
}

// UNUSED: needed to match (speculative code)
void zCameraFly_Reset(zCameraFly* fly)
{
    xASSERT(fly);
    xASSERT(fly->casset);

    xBaseReset(fly, fly->casset);
}

void zCameraFly_Save(zCameraFly* fly, xSerial* s)
{
    xASSERT_L(94, fly);
    xASSERT_L(95, s);

    xBaseSave(fly, s);
}

void zCameraFly_Load(zCameraFly* fly, xSerial* s)
{
    xASSERT_L(112, fly);
    xASSERT_L(113, s);

    xBaseLoad(fly, s);
}

zCamFlyThrough sFlyCam;
F32 zCamFlyThrough::skipTimeFlythrough = 2.0f;
const F32 zCamFlyThrough::FLY_FRAMERATE = 30.0f;

inline F32 zCamFlyThrough::TranSpeed(zFlyKey* keys)
{
    return FLY_FRAMERATE * xVec3Dist((xVec3*)&keys[0].matrix[9], (xVec3*)&keys[1].matrix[9]);
}

F32 zCamFlyThrough::MatrixSpeed(zFlyKey* keys)
{
    F32 dot = xabs(xVec3Dot((xVec3*)&keys[0].matrix[0], (xVec3*)&keys[1].matrix[0]));
    dot = MAX(xabs(xVec3Dot((xVec3*)&keys[0].matrix[3], (xVec3*)&keys[1].matrix[3])), dot);
    dot = MAX(xabs(xVec3Dot((xVec3*)&keys[0].matrix[6], (xVec3*)&keys[1].matrix[6])), dot);

    return FLY_FRAMERATE * (2.0f * (180.0f / PI) * xacos(MIN(1.0f, dot)));
}

void zCamFlyThrough::update(xScene&, F32 dt)
{
    if (mStopFly) {
        return;
    }

    if (CURPLAYERS[0]->GetPad()->Pressed(0x50000) && mFlyTime > skipTimeFlythrough) {
        mStopFly = true;
        return;
    }

    S32 i, flyIdx;
    F32 flyLerp, flyFrame;

    flyFrame = FLY_FRAMERATE * mFlyTime;
    flyIdx = std::floorf(flyFrame);
    flyLerp = flyFrame - std::floorf(flyFrame);
    if (flyIdx >= mNumKeys - 1) {
        mStopFly = true;
        return;
    }

    zFlyKey keys[4];
    keys[0] = mKeyFrames[(flyIdx - 1 >= 0) ? flyIdx - 1 : flyIdx];
    keys[1] = mKeyFrames[flyIdx];
    keys[2] = mKeyFrames[flyIdx + 1];
    keys[3] = mKeyFrames[(flyIdx + 2 < mNumKeys) ? flyIdx + 2 : flyIdx + 1];

    U8* data = (U8*)keys;
    for (i = 0; i < sizeof(keys); i += 4, data += 4) {
        U8 tmpA = data[0];
        U8 tmpB = data[1];
        data[0] = data[3];
        data[1] = data[2];
        data[2] = tmpB;
        data[3] = tmpA;
    }

    if (flyIdx > 0) {
        F32 trandiff1 = TranSpeed(&keys[0]);
        F32 trandiff2 = TranSpeed(&keys[1]);
        F32 trandiff3 = TranSpeed(&keys[2]);
        if (trandiff2 > 10.0f && trandiff2 > 5.0f * trandiff1 && trandiff2 > 5.0f * trandiff3) {
            flyLerp = 0.0f;
        } else {
            F32 matdiff1 = MatrixSpeed(&keys[0]);
            F32 matdiff2 = MatrixSpeed(&keys[1]);
            F32 matdiff3 = MatrixSpeed(&keys[2]);
            if (matdiff2 > 45.0f && matdiff2 > 5.0f * matdiff1 && matdiff2 > 5.0f * matdiff3) {
                flyLerp = 0.0f;
            }
        }
    }

    xMat3x3 tmpMat;
    xQuat quats[2], qresult;
    for (i = 0; i < 2; i++) {
        tmpMat.right.x = -keys[i + 1].matrix[0];
        tmpMat.right.y = -keys[i + 1].matrix[1];
        tmpMat.right.z = -keys[i + 1].matrix[2];
        tmpMat.up.x = keys[i + 1].matrix[3];
        tmpMat.up.y = keys[i + 1].matrix[4];
        tmpMat.up.z = keys[i + 1].matrix[5];
        tmpMat.at.x = -keys[i + 1].matrix[6];
        tmpMat.at.y = -keys[i + 1].matrix[7];
        tmpMat.at.z = -keys[i + 1].matrix[8];
        xQuatFromMat(&quats[i], &tmpMat);
    }
    xQuatSlerp(&qresult, &quats[0], &quats[1], flyLerp);
    xQuatToMat(&qresult, &mat);
    xVec3Lerp(&mat.pos, (xVec3*)&keys[1].matrix[9], (xVec3*)&keys[2].matrix[9], flyLerp);

    mFlyTime += dt * mFlyRate;
}

void zCamFlyThrough::create()
{
    xCam::create();
    flags |= 0x4;
    orient_type = XCAM_ORIENT_QUAT;
    coord_type = XCAM_COORD_CART;
}

void zCamFlyThrough::start()
{
    xCam::start();
    mFlyTime = 1.0f / FLY_FRAMERATE;
    mFlyRate = 1.0f;
    mStopFly = false;
    for (S32 i = 0; i < CURPLAYERS.size(); i++)
        CURPLAYERS[i]->TurnControlOff(CONTROL_OWNER_FLY_CAM);
    xScrFxLetterbox(TRUE);
}

void zCamFlyThrough::stop()
{
    xCam::stop();
    mStopFly = false;
    zEntEvent(mCamFly, eEventStop);
    mCamFly = NULL;
    for (S32 i = 0; i < CURPLAYERS.size(); i++)
        CURPLAYERS[i]->TurnControlOn(CONTROL_OWNER_FLY_CAM);
    xScrFxLetterbox(FALSE);
}

xCam* zCamFlyThrough::get_next()
{
    if (mStopFly) {
        return mPreviousCam;
    } else {
        return this;
    }
}

void zCamFlyThrough::SetFlyData(void* data, U32 size, zCameraFly* fly, xCam* previous)
{
    xASSERT_L(314, previous);
    mStopFly = false;
    mKeyFrames = (zFlyKey*)data;
    mNumKeys = size / sizeof(zFlyKey);
    mPreviousCam = previous;
    mCamFly = fly;
}

void zCamFlyThrough::StartFly(zCameraFly* fly)
{
    if (sFlyCam.started() || sFlyCam.mCamFly) {
        xASSERT_FMT_L(
            327, sFlyCam.mCamFly == 0, "Trying to start fly-through %s when %s is already running", xSTAssetName(fly->id), xSTAssetName(sFlyCam.mCamFly->id));
        xASSERT_FMT_L(328, !sFlyCam.started(), "Trying to start fly-through %s before previous fly-through finished shutting down", xSTAssetName(fly->id));
        return;
    }

    PKRAssetTOCInfo ainfo;
    if (xSTGetAssetInfo(fly->casset->flyID, &ainfo)) {
        sFlyCam.create();
        sFlyCam.SetFlyData(ainfo.mempos, ainfo.size, fly, zCamGetPrimary());
        zCamAdd(sFlyCam, true);
    }
}

void zCameraFlyEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xASSERT_L(368, to);

    zCameraFly* fly = (zCameraFly*)to;

    switch (toEvent) {
    case eEventEnable:
        xBaseEnable(fly);
        break;
    case eEventDisable:
        xBaseDisable(fly);
        break;
    case eEventRun:
        zCamFlyThrough::StartFly(fly);
        break;
    case eEventStop:
        break;
    case eEventSceneBegin:
        break;
    }
}
