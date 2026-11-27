#include "xCommon.h"
#include "zPlayerAction.h"

#include "zPlayer.h"

#include <string.h>

xAnimState* zPlayerAction::NewState(xAnimTable* table, const char* name, U32 flags, U32 userFlags, F32 speed, F32* boneBlend, F32* timeSnap, F32 fadeRecip, U16* fadeOffset, xAnimBeforeEnterCallback beforeEnter, xAnimStateCallback stateCallback, xAnimBeforeAnimMatricesCallback beforeAnimMatrices)
{
    return xAnimTableNewState(table, name, flags, userFlags, speed, boneBlend, timeSnap, fadeRecip, fadeOffset, this, beforeEnter, stateCallback, beforeAnimMatrices);
}

void zPlayerAction::NewStateMany(xAnimTable* table, const char* name, S32 howMany, U32 flags, U32 userFlags, F32 speed, F32* boneBlend, F32* timeSnap, F32 fadeRecip, U16* fadeOffset, xAnimBeforeEnterCallback beforeEnter, xAnimStateCallback stateCallback, xAnimBeforeAnimMatricesCallback beforeAnimMatrices)
{
    xAnimTableNewStateMany(
        table, name, howMany, flags, userFlags, speed, boneBlend, timeSnap, fadeRecip, fadeOffset, this, beforeEnter, stateCallback, beforeAnimMatrices);
}

void zPlayerAction::AddActionTransition(xAnimTable* table, const char* source, const char* dest, xAnimTransitionCallback condition, U16 priority, F32 blend, U32 flags, U32 userFlags)
{
    xAnimTableNewTransition(
        table, source, dest, condition, ActionChange, flags, userFlags, 0.0f, 0.0f, priority, 0, blend, NULL);
}

U32 zPlayerAction::ActionChange(xAnimTransition* tran, xAnimSingle*, void*)
{
    zPlayerAction* nextAction = (zPlayerAction*)tran->Dest->CallbackData;
    nextAction->manager->SetCurrentAction(nextAction);
    return 0;
}

void zPlayerAction::Move(xScene* s, F32 dt, xEntFrame* frame)
{
    player->DefaultMove(s, dt, frame);
}

void zPlayerAction::BeginUpdate(F32 dt)
{
    player->DefaultBeginUpdate(dt);
}

void zPlayerAction::Update(F32 dt)
{
    player->DefaultUpdate(dt);
}

void zPlayerAction::EndUpdate(F32 dt)
{
    player->DefaultEndUpdate(dt);
}

bool zPlayerAction::CollisionResponse(const xVec3& currentDir, const SphereCollisionResults& sceneCollide, xVec3& responseDpos)
{
    return player->DefaultCollisionResponse(currentDir, sceneCollide, responseDpos);
}

void zPlayerActionManager::SetCurrentAction(zPlayerAction* newCurrentAction)
{
    xASSERT_FMT_L(81, newCurrentAction, "You can't pass in NULL to SetCurrentAction");

    zPlayerAction* prevAction = currentAction;
    if (currentAction) {
        currentAction->End();
    }

    currentAction = newCurrentAction;
    currentAction->Begin();
}

U32 zPlayerActionManager::GetCurrentActionID() const
{
    if (currentAction) {
        return currentAction->GetID();
    } else {
        return -1;
    }
}

void zPlayerActionManager::Init(U32 actionCount)
{
    listSize = actionCount;
    U32 listMemSize = listSize * sizeof(zPlayerAction*);
    actionList = (zPlayerAction**)xMEMALLOC_L(listMemSize, 0, eMemMgrTag_Player, 0, 110);
    memset(actionList, 0, listMemSize);
    currentAction = NULL;
}

void zPlayerActionManager::Add(zPlayer* player, zPlayerAction* action)
{
    action->player = player;
    action->manager = this;
    xASSERT_FMT_L(119, action->GetID() < listSize, "This action ID is greater than the number of actions in the list. Crashing");
    xASSERT_FMT_L(120, !actionList[ action->GetID() ], "The action list already has a pointer for ID %i", action->GetID());
    actionList[action->GetID()] = action;
}

void zPlayerActionManager::Setup()
{
    currentAction = NULL;
    for (U32 i = 0; i < listSize; i++) {
        xASSERT_FMT_L(129, actionList[i], "Missing action with id %i", i);
        actionList[i]->Setup();
    }
}

void zPlayerActionManager::Reset()
{
    if (currentAction) {
        currentAction->End();
        currentAction = NULL;
    }

    for (U32 i = 0; i < listSize; i++) {
        xASSERT_FMT_L(143, actionList[i], "Missing action with id %i", i);
        actionList[i]->Reset();
    }

    xASSERT_FMT_L(147, currentAction, "CurrentAction is null after a reset. One of the actions should set itself as the current behavior.");
}

void zPlayerActionManager::BeginUpdate(F32 dt)
{
    for (U32 i = 0; i < listSize; i++) {
        xASSERT_FMT_L(155, actionList[i], "Missing action with id %i", i);
        actionList[i]->Timestep(dt);
    }

    currentAction->BeginUpdate(dt);
}

void zPlayerActionManager::Update(F32 dt)
{
    currentAction->Update(dt);
}

void zPlayerActionManager::EndUpdate(F32 dt)
{
    currentAction->EndUpdate(dt);
}

void zPlayerActionManager::Move(xScene* s, F32 dt, xEntFrame* frame)
{
    currentAction->Move(s, dt, frame);
}

void zPlayerActionManager::Render()
{
    currentAction->Render();
}

bool zPlayerActionManager::CollisionResponse(const xVec3& currentDir, const SphereCollisionResults& sceneCollide, xVec3& responseDpos)
{
    for (U32 i = 0; i < listSize; i++) {
        xASSERT_FMT_L(185, actionList[i], "Missing action with id %i", i);
        actionList[i]->NotifyCollision(currentDir, sceneCollide);
    }
    return currentAction->CollisionResponse(currentDir, sceneCollide, responseDpos);
}

void zPlayerActionManager::AddStates(xAnimTable* table)
{
    for (U32 i = 0; i < listSize; i++) {
        xASSERT_FMT_L(194, actionList[i], "Missing action with id %i", i);
        actionList[i]->AddStates(table);
    }
}

void zPlayerActionManager::AddTransitions(xAnimTable* table)
{
    for (U32 i = 0; i < listSize; i++) {
        xASSERT_FMT_L(202, actionList[i], "Missing action with id %i", i);
        actionList[i]->AddInternalTransitions(table);
        actionList[i]->AddActionTransitions(table);
    }
}

#if DEBUG || RELEASE
void zPlayerActionManager::AddTweaks()
{
    for (U32 i = 0; i < listSize; i++) {
        xASSERT_FMT_L(213, actionList[i], "Missing action with id %i", i);
        actionList[i]->AddTweaks();
    }
}
#endif

void zPlayerActionManager::AddStandardTransitionsTo(U32 actionID, xAnimTable* table, const char* fromStates)
{
    xASSERT_FMT_L(221, actionID < listSize, "This action ID is greater than the number of actions in the list. Crashing");
    xASSERT_FMT_L(222, actionList[actionID], "Missing action with id %i", actionID);
    actionList[actionID]->AddStandardTransitionsFrom(table, fromStates);
}

void zPlayerActionManager::AddTransitionsTo(U32 actionID, xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags)
{
    xASSERT_FMT_L(228, actionID < listSize, "This action ID is greater than the number of actions in the list. Crashing");
    xASSERT_FMT_L(229, actionList[actionID], "Missing action with id %i", actionID);
    actionList[actionID]->AddTransitionsFrom(table, fromStates, priority, blend, flags, userFlags);
}

zPlayerAction* zPlayerActionManager::GetAction(U32 actionID)
{
    xASSERT_FMT_L(235, actionID < listSize, "This action ID is greater than the number of actions in the list. Crashing");
    xASSERT_FMT_L(236, actionList[actionID], "Missing action with id %i", actionID);
    return actionList[actionID];
}

zPlayerAction* zPlayerActionManager::GetAction(U32 actionID) const
{
    xASSERT_FMT_L(242, actionID < listSize, "This action ID is greater than the number of actions in the list. Crashing");
    xASSERT_FMT_L(243, actionList[actionID], "Missing action with id %i", actionID);
    return actionList[actionID];
}
