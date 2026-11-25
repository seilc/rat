#include "xCommon.h"
#include "zCheckpoint.h"

#include "xSpacePartitionTree.h"
#include "xDebugTweak.h"
#include "xEnt.h"
#include "xEvent.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zCam.h"
#include "zEntTrigger.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zOtherAsset.h"
#include "zPad.h"
#include "zPlayer.h"
#include "zScene.h"

U32 zCheckpoint::currentActive;
U32 zCheckpoint::lastCheckpointNumber;

void zCheckpoint::SceneInit(zScene* scene)
{
    currentActive = 0;

    S32 count = scene->baseCount[eBaseTypeCheckpoint];
    zCheckpoint* checkpointList = (zCheckpoint*)scene->baseList[eBaseTypeCheckpoint];
    U32 number;

    if (scene->sceneID != IDTAG('M','N','U','S')) {
    }

    lastCheckpointNumber = 0;

    for (S32 i = 0; i < count; i++) {
        number = checkpointList[i].GetNumber();

        if (number == 1) {
            zEntEvent(&checkpointList[i], eEventCheckpointActivate);
        }

        if (number > lastCheckpointNumber) {
            lastCheckpointNumber = number;
        }
    }

    static tweak_callback tweakOnOffCB = tweak_callback::create_edit(NULL, TweakFinishEditCB);
    xTWEAK("Checkpoint Active", &currentActive, 1, lastCheckpointNumber, &tweakOnOffCB, NULL, 0x4);
}

void zCheckpoint::SceneReset(zScene* scene)
{
    S32 count = scene->baseCount[eBaseTypeCheckpoint];
    zCheckpoint* checkpointList = (zCheckpoint*)scene->baseList[eBaseTypeCheckpoint];

    for (S32 i = 0; i < count; i++) {
        if (checkpointList[i].GetNumber() == currentActive) {
            checkpointList[i].Reset();
            break;
        }
    }
}

void zCheckpoint::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(83, asset, asset_size, zCheckpointAsset, "Checkpoint");
    ((zCheckpoint&)data).Init((zCheckpointAsset*)&asset);
}

void zCheckpoint::Init(zCheckpointAsset* asset)
{
    xBaseInit(this, asset);

    this->asset = asset;
    eventFunc = EventCB;

    if (linkCount) {
        link = (xLinkAsset*)(asset + 1);
    } else {
        link = NULL;
    }

    if (asset->number != 1) {
        xASSERT_FMT_L(110, asset->triggerID, "Checkpoint %d has no trigger", asset->number);
        trigger = (zEntTrigger*)zSceneFindObject(asset->triggerID);
    }

    xASSERT_FMT_L(114, asset->pointerMrIID, "Checkpoint %d has no MrI pointer specified", asset->number);

    U32 size;
    pointerMrI = (pointer_asset*)xSTFindAsset(asset->pointerMrIID, &size);

    if (asset->number != 1) {
        triggerLink.srcEvent = eEventEnterAny;
        triggerLink.dstEvent = eEventCheckpointActivate;
        triggerLink.dstAssetID = id;

        xASSERT_FMT_L(129, trigger->linkCount == 0, "Checkpoint trigger must not have any links");
        trigger->link = &triggerLink;
        trigger->linkCount = 1;
    }
}

void zCheckpoint::Reset()
{
    xVec3 loc;
    F32 yaw;

    for (S32 i = 0; i < CURPLAYERS.size(); i++) {
        if (i == 0) {
            loc = pointerMrI->loc;
            yaw = pointerMrI->yaw;
        } else {
            loc = pointerFrozone->loc;
            yaw = pointerFrozone->yaw;
        }

        CURPLAYERS[i]->LoadCheckPointB(loc, (PI / 180.0f) * yaw);
        CURPLAYERS[i]->BoundUpdate(&xEntGetFrame(CURPLAYERS[i])->pos);
    }

    if (asset->scriptID) {
        zEntEvent(asset->scriptID, eEventRun);
    }

    zCamSetCamera(asset->cameraID, true);
}

void zCheckpoint::EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xASSERT_L(171, to);
    zCheckpoint& checkpoint = *(zCheckpoint*)to;
    xASSERT_L(173, checkpoint.baseType == eBaseTypeCheckpoint);

    if (toEvent == eEventCheckpointActivate) {
        if (checkpoint.GetNumber() > currentActive) {
            checkpoint.Activate();
        }
    }
}

void zCheckpoint::Activate()
{
    currentActive = asset->number;
}

void zCheckpoint::TweakFinishEditCB(const tweak_info&)
{
    U32 takeDamage = globals.settings.TakeDamage;
    globals.settings.TakeDamage = TRUE;

    zEntEvent(CURPLAYERS[0], eEventKill);
    zEntEvent(CURPLAYERS[1], eEventKill);

    globals.settings.TakeDamage = takeDamage;

#if DEBUG || RELEASE
    xDebugModeSet(1);
#endif

    zPad::GetDebugPad()->DisablePad();
}
