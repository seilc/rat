#include "xCommon.h"
#include "zButtonMasherWidget.h"

#include "xEvent.h"
#include "xGlobals.h"
#include "zEvent.h"
#include "zPad.h"
#include "zPlayer.h"
#include "zPlayerInput.h"
#include "zScene.h"

zButtonMasherMashChecker** zButtonMasherController::objectArray;
S32 zButtonMasherController::objectCount;

void zButtonMasherController::Init()
{
    objectCount = 0;
    objectArray = NULL;
}

void zButtonMasherController::UpdateAll(F32 dt)
{
    for (S32 i = 0; i < objectCount; i++) {
        objectArray[i]->Update(dt);
    }
}

S32 zButtonMasherController::AddToObjectCount()
{
    S32 t = objectCount;
    objectCount++;
    return t;
}

void zButtonMasherController::InsertInObjectArray(zButtonMasherMashChecker* inObj, S32 idx)
{
    if (objectArray == NULL) {
        objectArray = (zButtonMasherMashChecker**)xMEMALLOC_L(objectCount * sizeof(zButtonMasherMashChecker*), 0, eMemMgrTag_UI, 0, 58);
    }

    xASSERT_L(60, idx < objectCount);
    objectArray[idx] = inObj;
}

zButtonMasherMashChecker::zButtonMasherMashChecker(zButtonMasherWidgetAsset* theAsset)
{
    xBaseInit(this, theAsset);

    xASSERT_FMT_L(71, theAsset->playerId, "The button masher \"%s\" doesn't have a player specified", xSceneID2Name(xglobals->sceneCur, theAsset->id));
    zPlayer* player = (zPlayer*)zSceneFindObject(theAsset->playerId);
    xASSERT_FMT_L(73, player, "The button masher \"%s\" can't find its player (ID=0x%08x)", xSceneID2Name(xglobals->sceneCur, theAsset->id), theAsset->playerId);

    eventFunc = EventCB;

    if (linkCount)
        link = (xLinkAsset*)((U8*)theAsset + sizeof(zButtonMasherWidgetAsset));
    else
        link = NULL;

    asset = theAsset;

    privateFlags.on = false;
    privateFlags.doCalcRate = false;

    curRate = 0.0f;
    curVelocity = 0.0f;
    timer = 0.0f;
    button = asset->buttonId;

    playerInput = &player->GetPlayerInput();
    ResetButton();

    objectArrayIdx = zButtonMasherController::AddToObjectCount();
}

void zButtonMasherMashChecker::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(101, asset, asset_size, zButtonMasherWidgetAsset, "Button_Masher_Widget");
    new (&data) zButtonMasherMashChecker((zButtonMasherWidgetAsset*)&asset);
}

void zButtonMasherMashChecker::EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventSceneEnter:
        zButtonMasherController::InsertInObjectArray(this, objectArrayIdx);

    case eEventStart:
        if (!privateFlags.on) {
            zEntEvent(this, eEventNoRateThreshold);
        }
        privateFlags.on = true;
        break;

    case eEventStop:
        privateFlags.on = false;
        break;
    }
}

void zButtonMasherMashChecker::EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    xASSERT_L(132, to);

    zButtonMasherMashChecker* obj = (zButtonMasherMashChecker*)to;

    xASSERT_L(136, obj->baseType == eBaseTypeButtonMasherWidget);
    obj->EventCB(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

bool zButtonMasherMashChecker::CycleDone()
{
    return cycleDone;
}

void zButtonMasherMashChecker::Update(F32 dt)
{
    if (!privateFlags.on) {
        return;
    }

    timer += dt;

    F32 retRate = 0.0f;

    if (cycleDone) {
        ResetButton();

        if (!privateFlags.doCalcRate) {
            retRate = 0.0f;
            privateFlags.doCalcRate = true;
        } else {
            retRate = 1.0f / timer - curRate;
        }

        timer = 0.0f;
    }
    else if (timer > asset->decayTime) {
        privateFlags.doCalcRate = false;
        if (asset->decayRate != 0.0f && curRate > asset->decayRate * dt) {
            retRate = -asset->decayRate * dt;
        } else {
            retRate = -curRate;
        }
    }

    if (curRate < asset->lowRateThreshold) {
        if (curRate + retRate > asset->highRateThreshold) {
            zEntEvent(this, eEventHighRateThreshold);
        }
        else if (curRate + retRate > asset->medRateThreshold) {
            zEntEvent(this, eEventMedRateThreshold);
        }
        else if (curRate + retRate > asset->lowRateThreshold) {
            zEntEvent(this, eEventLowRateThreshold);
        }
    }
    else if (curRate < asset->medRateThreshold) {
        if (curRate + retRate > asset->highRateThreshold) {
            zEntEvent(this, eEventHighRateThreshold);
        }
        else if (curRate + retRate > asset->medRateThreshold) {
            zEntEvent(this, eEventMedRateThreshold);
        }
        else if (curRate + retRate > asset->lowRateThreshold) {
        } else {
            zEntEvent(this, eEventNoRateThreshold);
        }
    }
    else if (curRate < asset->highRateThreshold) {
        if (curRate + retRate > asset->highRateThreshold) {
            zEntEvent(this, eEventHighRateThreshold);
        }
        else if (curRate + retRate > asset->medRateThreshold) {
        } else if (curRate + retRate > asset->lowRateThreshold) {
            zEntEvent(this, eEventLowRateThreshold);
        } else {
            zEntEvent(this, eEventNoRateThreshold);
        }
    } else {
        if (curRate + retRate > asset->highRateThreshold) {
        } else if (curRate + retRate > asset->medRateThreshold) {
            zEntEvent(this, eEventMedRateThreshold);
        }
        else if (curRate + retRate > asset->lowRateThreshold) {
            zEntEvent(this, eEventLowRateThreshold);
        } else {
            zEntEvent(this, eEventNoRateThreshold);
        }
    }

    curRate += retRate;

    if (playerInput->GetPad()->Pressed(1 << button))
        pressed = true;
    if (pressed && playerInput->GetPad()->Released(1 << button))
        cycleDone = true;
}

void zButtonMasherMashChecker::ResetButton()
{
    cycleDone = false;
    pressed = false;
}
