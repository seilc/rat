#include "xCommon.h"
#include "zHeistLookoutPoint.h"

#include "xCam.h"
#include "xEntMotion.h"
#include "xEvent.h"
#include "xFile.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xPtankPool.h"
#include "xScene.h"
#include "xSpacePartitionTree.h"
#include "zCam.h"
#include "zCamMarker.h"
#include "zCommonPlayer.h"
#include "zEntTrigger.h"
#include "zEvent.h"
#include "zFX.h"
#include "zGlobals.h"
#include "zHeistManager.h"
#include "zPad.h"
#include "zReticle.h"
#include "zScene.h"

zHeistManager* zHeistLookoutPoint::heistManager;

zHeistLookoutPoint::zHeistLookoutPoint(zHeistLookoutPointAsset* theAsset) : zTriggerInteraction(theAsset)
{
    xBaseInit(this, theAsset);

    xASSERT_FMT_L(37, theAsset->lookAtId, "The interaction \"%s\" doesn't have a look at entity specified", xSceneID2Name(xglobals->sceneCur, theAsset->id));
    lookAtEnt = (xEnt*)zSceneFindObject(theAsset->lookAtId);
    xASSERT_FMT_L(39, lookAtEnt, "The interaction \"%s\" can't find its look at entity (ID=0x%08x)", xSceneID2Name(xglobals->sceneCur, theAsset->id), theAsset->lookAtId);

    xASSERT_FMT_L(41, theAsset->eventRecieverId, "The interaction \"%s\" doesn't have an event reciever specified", xSceneID2Name(xglobals->sceneCur, theAsset->id));
    eventReciever = zSceneFindObject(theAsset->eventRecieverId);
    xASSERT_FMT_L(43, eventReciever, "The interaction \"%s\" can't find its event reciever (ID=0x%08x)", xSceneID2Name(xglobals->sceneCur, theAsset->id), theAsset->eventRecieverId);

    eventFunc = EventCB;
    type = eInteractionTypeLookoutPoint;

    if (linkCount)
        link = (xLinkAsset*)(theAsset + 1);
    else
        link = NULL;

    memset(&camAsset, 0, sizeof(camAsset));

    xVec3 pointAt = {
        trigger->frame->mat.pos.x - xEntGetPos(lookAtEnt)->x,
        trigger->frame->mat.pos.y - xEntGetPos(lookAtEnt)->y,
        trigger->frame->mat.pos.z - xEntGetPos(lookAtEnt)->z
    };

    xMat3x3 pointAtMat;
    xMat3x3LookVec(&pointAtMat, &pointAt);

    camAsset.right = pointAtMat.right;
    camAsset.up = pointAtMat.up;
    camAsset.at = pointAtMat.at;
    camAsset.pos = trigger->frame->mat.pos;
    camAsset.fov = theAsset->fov;
    camAsset.trans_time = theAsset->transitionTime;
    camAsset.trans_type = eTransType_Linear;

    if (theAsset->angleMaxX == 180.0f) {
        angleMaxX = PI;
    } else {
        angleMaxX = (PI / 180.0f) * theAsset->angleMaxX;
    }

    angleMaxY = (PI / 180.0f) * theAsset->angleMaxY;
    fov = (PI / 180.0f) * theAsset->fov;
    curYAngle = 0.0f;
    curXAngle = 0.0f;

    memset(&prevCameraTransitionParams, 0, sizeof(prevCameraTransitionParams));
    prevCameraTransitionParams.blend_time = 1.0f;

    if (!heistManager) {
        zHeistManager::Create();
    }

    xASSERT_L(98, heistManager);

    Reset();
}

void zHeistLookoutPoint::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(105, asset, asset_size, zHeistLookoutPointAsset, "interaction:heist_lookout_point");
    new (&data) zHeistLookoutPoint((zHeistLookoutPointAsset*)&asset);
}

void zHeistLookoutPoint::UpdateHeistManager(F32 dt)
{
    if (heistManager) {
        heistManager->Update(dt);
    }
}

void zHeistLookoutPoint::EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    xASSERT_L(120, to);

    zHeistLookoutPoint* lookoutPoint = (zHeistLookoutPoint*)to;

    xASSERT_L(124, lookoutPoint->baseType == eBaseTypeHeistLookoutPoint);
    lookoutPoint->EventCB(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zHeistLookoutPoint::EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    zHeistLookoutPointAsset* asset = GetAsset();

    switch (toEvent) {
    case eEventActivate:
        activated = true;
        break;

    case eEventDeactivate:
        activated = false;
        break;

    case eEventAvailable:
        available = true;
        break;

    case eEventUnavailable:
        available = false;
        break;
    }
}

void zHeistLookoutPoint::Update(F32 dt)
{
    logoAlpha = zIcon_GetNewAlpha(logoAlpha, dt, trigger->frame->mat.pos, activated && !interactingPlayer, 40.0f);
    if (!available) {
        logoAlpha *= 0.8f;
    }

    for (S32 i = 0; i < CURPLAYERS.size() && !interactingPlayer; i++) {
        StartInteracting((zCommonPlayer*)CURPLAYERS[i]);
    }

    if (interactingPlayer) {
        timeInInteraction += dt;
        F32 palpha = 1.0f - timeInInteraction / camAsset.trans_time;
        if (palpha < 0.0f) {
            palpha = 0.0f;
        }

        interactingPlayer->SetModelAlpha(palpha);
    }
}

void zHeistLookoutPoint::RenderOverlays()
{
    heistManager->RenderOverlays();
}

void zHeistLookoutPoint::HandleInput(zPad* currPad, F32 dt)
{
    xMat4x3 rotMat, valMat;
    F32 rotAngle = (PI / 3.0f) * currPad->GetAnalog2()->GetX() * dt;

    if (!(curXAngle + rotAngle < angleMaxX) && angleMaxX < PI) {
        rotAngle = angleMaxX - curXAngle;
    }

    if (!(curXAngle + rotAngle > -angleMaxX) && angleMaxX < PI) {
        rotAngle = -angleMaxX - curXAngle;
    }

    switch (xInputState) {
    case 0:
        if (rotAngle < 0.0f) {
            xInputState = 1;
            zEntEvent(eventReciever, eEventHeistLookoutRight);
        }
        else if (rotAngle > 0.0f) {
            xInputState = 2;
            zEntEvent(eventReciever, eEventHeistLookoutLeft);
        }
        break;

    case 1:
        if (rotAngle == 0.0f) {
            xInputState = 0;
            zEntEvent(eventReciever, eEventHeistLookoutXCentered);
        }
        else if (rotAngle > 0.0f) {
            xInputState = 2;
            zEntEvent(eventReciever, eEventHeistLookoutLeft);
        }
        break;

    case 2:
        if (rotAngle == 0.0f) {
            xInputState = 0;
            zEntEvent(eventReciever, eEventHeistLookoutXCentered);
        }
        else if (rotAngle < 0.0f) {
            xInputState = 1;
            zEntEvent(eventReciever, eEventHeistLookoutRight);
        }
        break;
    }

    xMat4x3Rot(&rotMat, &xVec3::m_UnitAxisY, rotAngle, &camAsset.pos);
    xMat4x3Init(&valMat, &camAsset.right, &camAsset.up, &camAsset.at, &camAsset.pos);

    xMat4x3Mul(&valMat, &valMat, &rotMat);
    camAsset.right = valMat.right;
    camAsset.up = valMat.up;
    camAsset.at = valMat.at;
    camAsset.pos = valMat.pos;

    curXAngle += rotAngle;

    rotAngle = (PI / 3.0f) * currPad->GetAnalog2()->GetY() * dt;

    if (!(curYAngle + rotAngle < angleMaxY)) {
        rotAngle = angleMaxY - curYAngle;
    }

    if (!(curYAngle + rotAngle > -angleMaxY)) {
        rotAngle = -angleMaxY - curYAngle;
    }

    switch (yInputState) {
    case 0:
        if (rotAngle > 0.0f) {
            yInputState = 4;
            zEntEvent(eventReciever, eEventHeistLookoutDown);
        }
        else if (rotAngle < 0.0f) {
            yInputState = 3;
            zEntEvent(eventReciever, eEventHeistLookoutUp);
        }
        break;

    case 4:
        if (rotAngle == 0.0f) {
            yInputState = 0;
            zEntEvent(eventReciever, eEventHeistLookoutYCentered);
        }
        else if (rotAngle < 0.0f) {
            yInputState = 3;
            zEntEvent(eventReciever, eEventHeistLookoutUp);
        }
        break;

    case 3:
        if (rotAngle == 0.0f) {
            yInputState = 0;
            zEntEvent(eventReciever, eEventHeistLookoutYCentered);
        }
        else if (rotAngle > 0.0f) {
            yInputState = 4;
            zEntEvent(eventReciever, eEventHeistLookoutDown);
        }
        break;
    }

    xMat4x3Rot(&rotMat, &camAsset.right, rotAngle, &camAsset.pos);
    xMat4x3Init(&valMat, &camAsset.right, &camAsset.up, &camAsset.at, &camAsset.pos);

    xMat4x3Mul(&valMat, &valMat, &rotMat);
    camAsset.right = valMat.right;
    camAsset.up = valMat.up;
    camAsset.at = valMat.at;
    camAsset.pos = valMat.pos;

    curYAngle += rotAngle;

    if (heistManager) {
        heistManager->HandleInput(currPad, dt, this);
    }
}

void zHeistLookoutPoint::Start(zCommonPlayer* player)
{
    if (zCamGetPrimary()->get_final_dest() == zCamGetPrimary()) {
        player->SetExternalColorControl(true);
        zInteraction::Start(player);
        player->currAction = this;
        prevCamera = zCamGetPrimary();
        zCamMarkerActivateCamera(camAsset, NULL);

        heistManager->BeginInteraction(this);

        timeInInteraction = 0.0f;
        xInputState = 0;
        yInputState = 0;

        zEntEvent(eventReciever, eEventHeistLookoutStart);
        zEntEvent(this, eEventHeistLookoutStart);
    }
}

void zHeistLookoutPoint::Stop()
{
    xASSERT_L(359, interactingPlayer);
    interactingPlayer->StartFadeIn();
    interactingPlayer->SetExternalColorControl(false);
    zInteraction::Stop();
    prevCamera->cfg_common.blend_time = camAsset.trans_time;
    globals.cam->transition_to(*prevCamera, &prevCameraTransitionParams, false, false);

    heistManager->EndInteraction();

    zEntEvent(eventReciever, eEventHeistLookoutStop);
    zEntEvent(this, eEventHeistLookoutStop);
}

void zHeistLookoutPoint::GetLookAtVec(xVec3& inVec)
{
    inVec.Sub(*xEntGetPos(lookAtEnt), trigger->frame->mat.pos);
}

void zHeistLookoutPoint::GetLookFromPos(xVec3& inVec)
{
    inVec = trigger->frame->mat.pos;
}
