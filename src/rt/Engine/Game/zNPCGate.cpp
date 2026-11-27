#include "xCommon.h"
#include "zNPCGate.h"

#include "xFont.h"
#include "xGroup.h"
#include "xMemory.h"
#include "xSpacePartitionTree.h"
#include "zBase.h"
#include "zEntTrigger.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zICompNPCEntity.h"
#include "zNPCBase.h"
#include "zPlayer.h"
#include "zScene.h"

#include <float.h>

namespace {

zNPCGateMgr gNPCGateMgr;

}

void zNPCGateMgr::SceneSetup()
{
    if (gNPCGateMgr.numberOfGates)
        gNPCGateMgr.npcGates = new (eMemStaticType_0, eMemMgrTag_NPCGate) zNPCGate*[gNPCGateMgr.numberOfGates];
}

void zNPCGateMgr::Assign(zNPCGate* npcGate)
{
    xASSERT_L(54, npcGates);
    xASSERT_L(55, curGate<numberOfGates);
    npcGates[curGate++] = npcGate;
}

void zNPCGateMgr::SceneUpdate()
{
    for (S32 i = 0; i < gNPCGateMgr.numberOfGates; i++) {
        gNPCGateMgr.npcGates[i]->Update();
    }
}

void zNPCGateMgr::SceneExit()
{
    for (S32 i = 0; i < gNPCGateMgr.numberOfGates; i++) {
        gNPCGateMgr.npcGates[i]->Destroy();
    }

    gNPCGateMgr.numberOfGates = 0;
    gNPCGateMgr.npcGates = NULL;
    gNPCGateMgr.curGate = 0;
}

void zNPCGateMgr::HandlePlayerCollisions()
{
    for (S32 i = 0; i < gNPCGateMgr.numberOfGates; i++) {
        gNPCGateMgr.npcGates[i]->HandlePlayerCollision();
    }
}

void zNPCGate::InitDynamic(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidateMin_L(106, asset, asset_size, zNPCGateAsset, "Enemy:NPC Gate");
    new (&data) zNPCGate((zNPCGateAsset*)&asset, asset_size);
    gNPCGateMgr.IncrementNumber();
}

zNPCGate::zNPCGate(zNPCGateAsset* asset, U32)
{
    xBaseInit(this, asset);

    eventFunc = EventHandler;
    npcGateAsset = asset;

    numberOfNPCs = 0;
    npcPointers = NULL;
    gateTrigger = NULL;
    gateMat = NULL;
    gateFacing = eNPCGateDir_At;
}

void zNPCGate::EventHandler(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xASSERT_L(131, to->baseType == eBaseTypeNPCGate);

    zNPCGate* npcGate = (zNPCGate*)to;
    switch (toEvent) {
    case eEventEnable:
        npcGate->gateIsEnabled = true;
        break;

    case eEventDisable:
        npcGate->gateIsEnabled = false;
        break;
    }
}

void zNPCGate::Setup()
{
    SetupAttachedNPCs();

    SetupPhysicalData();

    gNPCGateMgr.Assign(this);
}

void zNPCGate::SetupAttachedNPCs()
{
    numberOfNPCs = 0;
    npcPointers = NULL;

    if (npcGateAsset->npcsGroup == 0) {
        xFAIL_MSG_L(169, "There are no npcs group attached to NPC Gate");
        return;
    }

    xBase* b = zSceneFindObject(npcGateAsset->npcsGroup);
    if (!b || b->baseType != eBaseTypeGroup) {
        xFAIL_MSG_L(177, "The widget attached to the NPC Gate is not a group!");
        return;
    }

    xGroup* npcsGroup = (xGroup*)b;
    S32 numberInGroup = xGroupGetCount(npcsGroup);

    for (S32 i = 0; i < numberInGroup; i++) {
        xBase* b = xGroupGetItemPtr(npcsGroup, i);
        if (!b) {
            continue;
        }

        if (b->baseType != eBaseTypeNPC) {
            xFAIL_MSG_L(196, "This NPC Gate group contains a non NPC.");
            continue;
        }

        zNPCBase* npc = (zNPCBase*)b;
        numberOfNPCs += npc->GetNumberOfChildren() + 1;
    }

    npcPointers = new (eMemStaticType_0, eMemMgrTag_NPCGate) zNPCBase*[numberOfNPCs];

    S32 curNPC = 0;
    for (S32 i = 0; i < numberInGroup; i++) {
        xBase* b = xGroupGetItemPtr(npcsGroup, i);
        if (!b) {
            continue;
        }

        if (b->baseType != eBaseTypeNPC) {
            continue;
        }

        zNPCBase* npc = (zNPCBase*)b;
        zNPCBase* children;
        S32 numChildren;
        npc->GetChildren(&children, &numChildren);

        npcPointers[curNPC++] = npc;

        for (S32 j = 0; j < numChildren; j++) {
            npcPointers[curNPC++] = &children[j];
        }
    }
}

void zNPCGate::SetupPhysicalData()
{
    gateIsEnabled = npcGateAsset->flags & 0x1;

    if (npcGateAsset->gateTrigger == 0) {
        xFAIL_MSG_L(252, "There is no trigger attached to NPC Gate");
        return;
    }

    xBase* b = zSceneFindObject(npcGateAsset->gateTrigger);
    if (!b || b->baseType != eBaseTypeTrigger) {
        xFAIL_MSG_L(260, "There is a problem with the trigger of the NPC Gate!");
        return;
    }

    gateTrigger = (zEntTrigger*)b;

    xASSERT_L(265, gateTrigger->subType == eEntTriggerTypeBox);

    gateBox = gateTrigger->triggerBox;

    const xSphere* player_spheres[zPlayer::MAX_EXTRA_SPHERES];
    S32 num_spheres = CURPLAYERS[0]->GetCollisionSpheres(player_spheres);
    F32 playerBound = player_spheres[0]->r, trigSizeInc = 2.0f + playerBound;

    gateTrigger->triggerBox.upper.x += trigSizeInc;
    gateTrigger->triggerBox.upper.y += trigSizeInc;
    gateTrigger->triggerBox.upper.z += trigSizeInc;

    gateTrigger->triggerBox.lower.x -= trigSizeInc;
    gateTrigger->triggerBox.lower.y -= trigSizeInc;
    gateTrigger->triggerBox.lower.z -= trigSizeInc;

    gateMat = &gateTrigger->frame->mat;

    gateFacing = (eNPCGateDir)npcGateAsset->gateFacing;

    switch (gateFacing) {
    case eNPCGateDir_At:
        gateDir = gateMat->at;
        gateMax = gateBox.upper.z;
        gateMin = gateBox.lower.z;
        break;
    case eNPCGateDir_Right:
        gateDir = gateMat->right;
        gateMax = gateBox.upper.x;
        gateMin = gateBox.lower.x;
        break;
    case eNPCGateDir_AtM:
        gateDir = gateMat->at;
        gateDir *= -1.0f;
        gateMax = -gateBox.lower.z;
        gateMin = -gateBox.upper.z;
        break;
    case eNPCGateDir_RightM:
        gateDir = gateMat->right;
        gateDir *= -1.0f;
        gateMax = -gateBox.lower.x;
        gateMin = -gateBox.upper.x;
        break;
    }

    gateDir.normalize();

    gateIsOn = false;
}

void zNPCGate::Update()
{
    if (gateIsEnabled) {
        gateIsOn = false;

        xVec3 playerPos = CURPLAYERS[0]->frame->mat.pos;
        if (gateTrigger->PointIntersects(playerPos)) {
            F32 gatePos = -FLT_MAX;

            for (S32 i = 0; i < numberOfNPCs; i++) {
                const xVec3* npcPos;
                zICompNPCEntity* npcEnt = npcPointers[i]->GetEntityComponent();
                if (!npcEnt)
                    continue;

                npcPos = npcEnt->GetPosition();

                F32 npcGatePos = ConvertWorldToGateLocalZ(*npcPos);
                if (npcGatePos < gateMax && npcGatePos > gateMin) {
                    gateIsOn = true;

                    npcGatePos += npcEnt->bound.sph.r;
                    if (npcGatePos > gatePos)
                        gatePos = npcGatePos;
                }
            }

            if (gatePos > gateMax) {
                gatePos = gateMax;
            }

            if (gateIsOn) {
                switch (gateFacing) {
                case eNPCGateDir_At:
                    gateBox.upper.z = gatePos;
                    break;
                case eNPCGateDir_Right:
                    gateBox.upper.x = gatePos;
                    break;
                case eNPCGateDir_AtM:
                    gateBox.lower.z = -gatePos;
                    break;
                case eNPCGateDir_RightM:
                    gateBox.lower.x = -gatePos;
                    break;
                }
            }
        }
    }
}

F32 zNPCGate::ConvertWorldToGateLocalZ(const xVec3& iWorld)
{
    xVec3 diff;
    diff.Sub(iWorld, gateMat->pos);
    return gateDir.x * diff.x + gateDir.z * diff.z;
}

void zNPCGate::ConvertWorldToBoxLocal(const xVec3& iWorld, xVec3& oLocal)
{
    xVec3 diff;
    diff.Sub(iWorld, gateMat->pos);

    xVec3* matAt = &gateMat->at;

    oLocal.x = matAt->z * diff.x - matAt->x * diff.z;
    oLocal.y = diff.y;
    oLocal.z = matAt->x * diff.x + matAt->z * diff.z;
}

void zNPCGate::ConvertBoxLocalToWorld(const xVec3& iLocal, xVec3& oWorld)
{
    xVec3* matAt = &gateMat->at;

    oWorld.x = matAt->z * iLocal.x + matAt->x * iLocal.z;
    oWorld.y = iLocal.y;
    oWorld.z = matAt->z * iLocal.z - matAt->x * iLocal.x;

    oWorld.Add(gateMat->pos);
}

void zNPCGate::HandlePlayerCollision()
{
    if (!gateIsEnabled || !gateIsOn) {
        return;
    }

    xVec3* playerOldPos = &CURPLAYERS[0]->frame->mat.pos;
    xVec3* playerDPos = &CURPLAYERS[0]->frame->dpos;
    xVec3 playerNewPos;
    playerNewPos.Add(*playerOldPos, *playerDPos);

    xVec3 localPlayerNewPos;
    ConvertWorldToBoxLocal(playerNewPos, localPlayerNewPos);

    F32 wallDistances[4];
    wallDistances[0] = gateBox.upper.z - localPlayerNewPos.z;
    wallDistances[1] = gateBox.upper.x - localPlayerNewPos.x;
    wallDistances[2] = localPlayerNewPos.z - gateBox.lower.z;
    wallDistances[3] = localPlayerNewPos.x - gateBox.lower.x;

    if (wallDistances[0] < 0.0f || wallDistances[1] < 0.0f || wallDistances[2] < 0.0f || wallDistances[3] < 0.0f) {
        return;
    }

    S32 closestWall = -1;
    F32 closestWallDist = FLT_MAX;
    for (S32 i = 0; i < 4; i++) {
        if (wallDistances[i] >= 0.0f && wallDistances[i] < closestWallDist) {
            closestWallDist = wallDistances[i];
            closestWall = i;
        }
    }

    xASSERT_L(477, closestWall>=0 && closestWall<4);

    switch (closestWall) {
    case 0:
        localPlayerNewPos.z = gateBox.upper.z;
        break;
    case 1:
        localPlayerNewPos.x = gateBox.upper.x;
        break;
    case 2:
        localPlayerNewPos.z = gateBox.lower.z;
        break;
    case 3:
        localPlayerNewPos.x = gateBox.lower.x;
        break;
    }

    ConvertBoxLocalToWorld(localPlayerNewPos, playerNewPos);

    playerDPos->Sub(playerNewPos, *playerOldPos);
}

void zNPCGate::Destroy()
{
    numberOfNPCs = 0;
    npcPointers = NULL;
    gateTrigger = NULL;
    gateMat = NULL;
    gateFacing = eNPCGateDir_At;
}
