#include "xCommon.h"
#include "zNPCBase.h"

#include "zEvent.h"
#include "zICompCombat.h"
#include "zICompLogic.h"
#include "zICompNPCEntity.h"
#include "zICompPerception.h"
#include "zICompSound.h"
#include "zICompSteering.h"

#include <float.h>

zNPCBase::zNPCBase()
{
    npcLogicComp = NULL;
    npcEntityComp = NULL;
    npcSteeringComp = NULL;
    npcPerceptionComp = NULL;
    npcSoundComp = NULL;
    npcCombatComp = NULL;

    children = NULL;
    parent = NULL;
    numChildren = 0;
}

void zNPCBase::EventWrapper(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id)
{
    zNPCBase* npc = (zNPCBase*)to;

    if (npc->SystemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return;
    }

    if (npc->ForwardEventToComponents(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return;
    }

    if (npc->children && to_event > 1000) {
        for (S32 i = 0; i < npc->numChildren; i++) {
            EventWrapper(from, &npc->children[i], to_event, to_param, to_param_widget, to_param_widget_id);
        }
    }
}

bool zNPCBase::SystemEvent(xBase*, xBase*, U32 to_event, const F32*, xBase*, U32)
{
    switch (to_event) {
    case eEventSpawn:
        if (!alive)
            taggedForSpawn = true;
        return true;
    case eEventKill:
        if (alive) {
            taggedForKill = true;
        }

        if (Kill())
            alive = false;
        return true;
    case eEventKilled:
        if (taggedForKill) {
            taggedForKill = false;
            alive = false;
        }
        return true;
    }

    return false;
}

void zNPCBase::Load(eNPCType type, const xModelAssetInfo* modelAsset, zNPCAssetRATS* npcAsset, U32 npcAssetSize)
{
    this->type = type;
    this->npcAsset = npcAsset;
    this->npcAssetSize = npcAssetSize;
    this->modelAsset = modelAsset;
    eventFunc = EventWrapper;

    xBaseInit(this, &npcAsset->ent_asset);

    Reset();

    baseFlags = k_XBASE_IS_ENABLED | k_XBASE_IS_VALID | k_XBASE_IS_VISIBLE_IN_CUTSCENES | k_XBASE_RECEIVES_SHADOWS;

    if (linkCount < 1) {
        link = NULL;
    } else {
        char* ptr = (char*)this->npcAsset;
        ptr += this->npcAssetSize;
        S32 amt_linkdata = linkCount * sizeof(xLinkAsset);
        xASSERT_FMT_L(138, ((amt_linkdata % 4)==0), "0x%08X", id);
        amt_linkdata = (amt_linkdata + 3) & ~3;
        link = (xLinkAsset*)(ptr - amt_linkdata);
    }
}

void zNPCBase::Reset()
{
    xVec3Copy(&lastPos, &npcAsset->ent_asset.pos);
    xVec3Copy(&lastOrientation, &npcAsset->ent_asset.ang);

    if (npcAsset->flags & k_ZNPCASSETRATS_NO_UPDATE_DISTANCE)
        updateCullDistance = FLT_MAX;
    else
        updateCullDistance = npcAsset->updateDistance;

    alive = false;
    inUpdateDistance = false;
    activated = false;
    taggedForSpawn = (npcAsset->flags & k_ZNPCASSETRATS_SPAWN_AT_START) != 0;
    taggedForKill = false;

#if RELEASE
    ResetAllComponents();
#else
    for (S32 i = 0; i < numChildren; i++) {
        children[i].Reset();
    }
#endif
}

#if RELEASE
void zNPCBase::BaseTimeStep(xVec3* cullOrigin, xScene* xscn, F32 dt)
#else
void zNPCBase::TimeStep(xVec3* cullOrigin, xScene* xscn, F32 dt)
#endif
{
    F32 distanceFromPlayer2 = cullOrigin->Distance2(lastPos);

    crossingUpdateDistance = false;
    if (inUpdateDistance) {
        if (distanceFromPlayer2 > xsqr(1.25f) * xsqr(updateCullDistance)) {
            inUpdateDistance = false;
            crossingUpdateDistance = true;
        }
    } else {
        if (distanceFromPlayer2 < xsqr(updateCullDistance)) {
            inUpdateDistance = true;
            crossingUpdateDistance = true;
        }
    }

    if (activated) {
        if (alive && inUpdateDistance) {
            BaseUpdate(xscn, dt);
            UpdateNPCBaseData();
        } else
            BaseDeactivate();
    } else {
        if (inUpdateDistance) {
            if (alive) {
                BaseActivate();
            }
            else if (taggedForSpawn) {
                BaseSpawn();
            }
        }
    }
}

void zNPCBase::BaseRender()
{
    if (!activated) {
        return;
    }

    Render();

    if (children)
        for (S32 i = 0; i < numChildren; i++) {
            children[i].Render();
        }
}

#if RELEASE
void zNPCBase::BaseReset()
{
    BaseDeactivate();

    Reset();

    if (children)
        for (S32 i = 0; i < numChildren; i++) {
            children[i].Reset();
        }
}
#endif

void zNPCBase::BaseUpdate(xScene* xscn, F32 dt)
{
    Update(xscn, dt);

    if (children)
        for (S32 i = 0; i < numChildren; i++) {
            children[i].Update(xscn, dt);
        }
}

bool zNPCBase::BaseSpawn()
{
#if RELEASE
    BaseReset();
#endif

    if (!Activate()) {
        if (crossingUpdateDistance)
            xFAIL_MSG_L(DEBUG ? 275 : 293, "Couldn't Activate the NPC!");
        BaseDeactivate();
        return false;
    }

    for (S32 i = 0; i < numChildren; i++) {
        if (!children[i].Activate()) {
            if (crossingUpdateDistance)
                xFAIL_MSG_L(DEBUG ? 286 : 304, "Couldn't Activate the NPC! One of the child NPCs failed!");
            BaseDeactivate();
            return false;
        }
    }

    AllAttached();
    Spawn();
    taggedForSpawn = false;
    activated = true;
    alive = true;

    for (S32 i = 0; i < numChildren; i++) {
        children[i].AllAttached();
        children[i].Spawn();
        children[i].taggedForSpawn = false;
        children[i].activated = true;
        children[i].alive = true;
    }

    return true;
}

bool zNPCBase::BaseActivate()
{
    if (!Activate()) {
        if (crossingUpdateDistance)
            xFAIL_MSG_L(DEBUG ? 318 : 336, "Couldn't Activate the NPC!");
        BaseDeactivate();
        return false;
    }

    for (S32 i = 0; i < numChildren; i++) {
        if (!children[i].Activate()) {
            if (crossingUpdateDistance)
                xFAIL_MSG_L(DEBUG ? 329 : 347, "Couldn't Activate the NPC! One of the child NPCs failed!");
            BaseDeactivate();
            return false;
        }
    }

    AllAttached();
    activated = true;

    for (S32 i = 0; i < numChildren; i++) {
        children[i].AllAttached();
        children[i].activated = true;
    }

    return true;
}

void zNPCBase::BaseDeactivate()
{
    for (S32 i = 0; i < numChildren; i++) {
        children[i].Deactivate();
        children[i].activated = false;
    }

    Deactivate();

    activated = false;
}

void zNPCBase::UpdateNPCBaseData()
{
    xASSERT_FMT_L(DEBUG ? 366 : 384, npcEntityComp, "This NPC has no entity. Its position and orientation cannot be determined. Please either add an entity or override the UpdateNPCBase function");

    xVec3Copy(&lastPos, &npcEntityComp->frame->mat.pos);
    xMat3x3GetEuler(&npcEntityComp->frame->mat, &lastOrientation);
}

void zNPCBase::SetChildren(zNPCBase* firstChildren, S32 numChildren)
{
    char indexStr[32];

    children = firstChildren;
    this->numChildren = numChildren;

    for (S32 i = 0; i < numChildren; i++) {
        sprintf(indexStr, "_C%d", i);
        children[i].id = xStrHashCat(id, indexStr);
    }
}

void zNPCBase::SetParent(zNPCBase* parent)
{
    this->parent = parent;
}

#if RELEASE
void zNPCBase::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCBase::AllAttached()
{
    if (npcLogicComp)
        npcLogicComp->AllAttached();
    if (npcEntityComp)
        npcEntityComp->AllAttached();
    if (npcSteeringComp)
        npcSteeringComp->AllAttached();
    if (npcPerceptionComp)
        npcPerceptionComp->AllAttached();
    if (npcSoundComp)
        npcSoundComp->AllAttached();
    if (npcCombatComp)
        npcCombatComp->AllAttached();
}

void zNPCBase::PreUpdateAllComponents(xScene* xscn, F32 dt)
{
    if (npcLogicComp)
        npcLogicComp->PreUpdate(xscn, dt);
    if (npcEntityComp)
        npcEntityComp->PreUpdate(xscn, dt);
    if (npcSteeringComp)
        npcSteeringComp->PreUpdate(xscn, dt);
    if (npcPerceptionComp)
        npcPerceptionComp->PreUpdate(xscn, dt);
    if (npcSoundComp)
        npcSoundComp->PreUpdate(xscn, dt);
    if (npcCombatComp)
        npcCombatComp->PreUpdate(xscn, dt);
}

void zNPCBase::PostUpdateAllComponents(xScene* xscn, F32 dt)
{
    if (npcLogicComp)
        npcLogicComp->PostUpdate(xscn, dt);
    if (npcEntityComp)
        npcEntityComp->PostUpdate(xscn, dt);
    if (npcSteeringComp)
        npcSteeringComp->PostUpdate(xscn, dt);
    if (npcPerceptionComp)
        npcPerceptionComp->PostUpdate(xscn, dt);
    if (npcSoundComp)
        npcSoundComp->PostUpdate(xscn, dt);
    if (npcCombatComp)
        npcCombatComp->PostUpdate(xscn, dt);
}

void zNPCBase::DebugRenderAllComponents()
{
    if (npcLogicComp)
        npcLogicComp->DebugRender();
    if (npcEntityComp)
        npcEntityComp->DebugRender();
    if (npcSteeringComp)
        npcSteeringComp->DebugRender();
    if (npcPerceptionComp)
        npcPerceptionComp->DebugRender();
    if (npcSoundComp)
        npcSoundComp->DebugRender();
    if (npcCombatComp)
        npcCombatComp->DebugRender();
}

bool zNPCBase::ForwardEventToComponents(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id)
{
    if (npcLogicComp && npcLogicComp->SystemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return true;
    }

    if (npcEntityComp && npcEntityComp->SystemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return true;
    }

    if (npcSteeringComp && npcSteeringComp->SystemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return true;
    }

    if (npcPerceptionComp && npcPerceptionComp->SystemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return true;
    }

    if (npcSoundComp && npcSoundComp->SystemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return true;
    }

    if (npcCombatComp && npcCombatComp->SystemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id)) {
        return true;
    }

    return false;
}

void zNPCBase::DetachAllComponents()
{
    if (npcLogicComp)
        npcLogicComp->Detach();
    if (npcEntityComp)
        npcEntityComp->Detach();
    if (npcSteeringComp)
        npcSteeringComp->Detach();
    if (npcPerceptionComp)
        npcPerceptionComp->Detach();
    if (npcSoundComp)
        npcSoundComp->Detach();
    if (npcCombatComp) {
        npcCombatComp->Detach();
    }

    npcLogicComp = NULL;
    npcEntityComp = NULL;
    npcSteeringComp = NULL;
    npcPerceptionComp = NULL;
    npcSoundComp = NULL;
    npcCombatComp = NULL;
}

#if RELEASE
void zNPCBase::ResetAllComponents()
{
    if (npcLogicComp)
        npcLogicComp->Reset();
    if (npcEntityComp)
        npcEntityComp->Reset();
    if (npcSteeringComp)
        npcSteeringComp->Reset();
    if (npcPerceptionComp)
        npcPerceptionComp->Reset();
    if (npcSoundComp)
        npcSoundComp->Reset();
    if (npcCombatComp)
        npcCombatComp->Reset();
}
#endif
