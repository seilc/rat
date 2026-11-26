#include "xCommon.h"
#include "zHeistInteractionObject.h"

#include "xCam.h"
#include "xEntMotion.h"
#include "xEvent.h"
#include "xFile.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xMovePoint.h"
#include "xPtankPool.h"
#include "xScene.h"
#include "xSpacePartitionTree.h"
#include "zEntTrigger.h"
#include "zEvent.h"
#include "zFX.h"
#include "zHeistManager.h"
#include "zReticle.h"
#include "zScene.h"

zHeistDistractionObjectManager* zHeistDistractionObject::heistDistractionObjectManager;

void zHeistInteractionObjectManager::Init()
{
    heistDistractionObjectManager = (zHeistDistractionObjectManager*)xMEMALLOC_L(sizeof(zHeistDistractionObjectManager), 0, eMemMgrTag_Heists, 0, 33);
    heistObjectiveManager = (zHeistObjectiveManager*)xMEMALLOC_L(sizeof(zHeistObjectiveManager), 0, eMemMgrTag_Heists, 0, 34);

    zHeistDistractionObject::SetDistractionObjectManager(heistDistractionObjectManager);

    heistDistractionObjectManager->Init();
    heistObjectiveManager->Init();
}

void zHeistInteractionObjectManager::Exit()
{
    heistDistractionObjectManager->Exit();
    heistObjectiveManager->Exit();

    heistDistractionObjectManager = NULL;
    heistObjectiveManager = NULL;
}

void zHeistInteractionObjectManager::RenderOverlays()
{
    heistDistractionObjectManager->RenderOverlays();
    heistObjectiveManager->RenderOverlays();
}

void zHeistInteractionObjectManager::Update(F32 dt)
{
    heistDistractionObjectManager->Update(dt);
    heistObjectiveManager->Update(dt);
}

zHeistDistractionObject::zHeistDistractionObject(zHeistDistractionObjectAsset* theAsset) : zTriggerInteraction(theAsset)
{
    xBaseInit(this, theAsset);

    xASSERT_FMT_L(74, theAsset->enemyToDistractId, "The interaction \"%s\" doesn't have an enemy specified", xSceneID2Name(xglobals->sceneCur, theAsset->id));
    distractEnt = (xEnt*)zSceneFindObject(theAsset->enemyToDistractId);
    xASSERT_FMT_L(76, distractEnt, "The interaction \"%s\" can't find its enemy entity (ID=0x%08x)", xSceneID2Name(xglobals->sceneCur, theAsset->id), theAsset->enemyToDistractId);

    xASSERT_FMT_L(78, theAsset->distractionPointId, "The interaction \"%s\" doesn't have a point specified", xSceneID2Name(xglobals->sceneCur, theAsset->id));
    distractPoint = (xMovePoint*)zSceneFindObject(theAsset->distractionPointId);
    xASSERT_FMT_L(80, distractPoint, "The interaction \"%s\" can't find its move point (ID=0x%08x)", xSceneID2Name(xglobals->sceneCur, theAsset->id), theAsset->distractionPointId);

    distractTime = theAsset->distractionTime;
    distractDelay = theAsset->distractionDelay;
    timer = 0.0f;

    eventFunc = EventCB;
    type = eInteractionTypeDistractionPoint;

    if (linkCount)
        link = (xLinkAsset*)(theAsset + 1);
    else
        link = NULL;

    if (!heistDistractionObjectManager) {
        zHeistManager::Create();
    }

    xASSERT_L(101, heistDistractionObjectManager);
    objectArrayIdx = heistDistractionObjectManager->AddToObjectCount();

    Reset();
}

void zHeistDistractionObject::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(109, asset, asset_size, zHeistDistractionObjectAsset, "interaction:distraction_point");
    new (&data) zHeistDistractionObject((zHeistDistractionObjectAsset*)&asset);
}

void zHeistDistractionObject::EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    xASSERT_L(116, to);

    zHeistDistractionObject* distractionPoint = (zHeistDistractionObject*)to;

    xASSERT_L(120, distractionPoint->baseType == eBaseTypeDistractionPoint);
    distractionPoint->EventCB(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zHeistDistractionObject::EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventSceneEnter:
        SceneEnter();
        break;

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

void zHeistDistractionObject::SceneEnter()
{
    heistDistractionObjectManager->RegisterObject(this);
}

void zHeistDistractionObject::Update(F32 dt)
{
    if (timer > 0.0f) {
        timer -= dt;
        if (timer <= 0.0f) {
            F32 params[4];
            params[0] = distractTime;
            zEntEvent(this, distractEnt, eEventDistracted, params, distractPoint, distractPoint->id);
        }
    }

    logoAlpha = zIcon_GetNewAlpha(logoAlpha, dt, trigger->frame->mat.pos, activated && !interactingPlayer, 40.0f);
    if (!available) {
        logoAlpha *= 0.8f;
    }

    for (S32 i = 0; i < CURPLAYERS.size() && !interactingPlayer; i++) {
        StartInteracting((zCommonPlayer*)CURPLAYERS[i]);
    }
}

void zHeistDistractionObject::Start(zCommonPlayer* player)
{
    zInteraction::Start(player);

    timer = distractDelay;
    zEntEvent(this, eEventDistractionStarted);
}

void zHeistDistractionObject::Stop()
{
    zInteraction::Stop();
}

void zHeistDistractionObjectManager::Init()
{
}

void zHeistDistractionObjectManager::Exit()
{
}

void zHeistDistractionObjectManager::RenderOverlays()
{
}

void zHeistDistractionObjectManager::Update(F32)
{
}

void zHeistDistractionObjectManager::RegisterObject(zHeistDistractionObject* regObj)
{
    if (regObj) {
        if (!objectArray) {
            objectArray = (zHeistDistractionObject**)xMEMALLOC_L(objectCount * sizeof(zHeistDistractionObject*), 0, eMemMgrTag_Heists, 0, 229);
        }

        InsertObject(regObj, regObj->GetObjectArrayIdx());
    }
}

S32 zHeistDistractionObjectManager::AddToObjectCount()
{
    S32 idx = objectCount;
    objectCount++;
    return idx;
}

void zHeistDistractionObjectManager::InsertObject(zHeistDistractionObject* insObj, S32 idx)
{
    xASSERT_L(244, objectArray);
    xASSERT_L(245, idx < objectCount);
    objectArray[idx] = insObj;
}

void zHeistObjectiveManager::Init()
{
}

void zHeistObjectiveManager::Exit()
{
}

void zHeistObjectiveManager::RenderOverlays()
{
}

void zHeistObjectiveManager::Update(F32)
{
}
