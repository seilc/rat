#include "xCommon.h"
#include "zNPCStates.h"

#include "xString.h"
#include "zICompNPCEntity.h"

// UNUSED: needed to match (speculative code)
void zTimerState::Setup(S32 nextStateEnum, F32 time)
{
    stateEnumToGo = nextStateEnum;
    endTime = time;
    stateTime = 0.0f;
}

void zAnimPlayState::Setup(S32 nextStateEnum, const char* animationName)
{
    animStateID = xStrHash(animationName);
    stateEnumToGo = nextStateEnum;
}

#if RELEASE
void zAnimPlayState::CheckTranstitions(zNPCBase* owner)
#else
void zAnimPlayState::Update(zNPCBase* owner, F32)
#endif
{
    zICompNPCEntity* compEnt = owner->GetEntityComponent();
    if (!compEnt || compEnt->IsAnimationStopped(0)) {
        SwitchState(stateEnumToGo);
    }
}

#if RELEASE
void zAnimPlayState::Activate(zNPCBase* owner)
#else
void zAnimPlayState::Resume(zNPCBase* owner)
#endif
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState(animStateID, 0.3f, NULL);
    }
}

void zAnimWTimerState::Setup(S32 nextStateEnum, const char* animationName, F32 time)
{
    animStateID = xStrHash(animationName);
    stateEnumToGo = nextStateEnum;
    endTime = time;
    stateTime = 0.0f;
}

#if !RELEASE
void zAnimWTimerState::Enter(zNPCBase* owner)
{
    stateTime = 0.0f;
    Resume(owner);
}
#endif

#if RELEASE
void zAnimWTimerState::Activate(zNPCBase* owner)
#else
void zAnimWTimerState::Resume(zNPCBase* owner)
#endif
{
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->SetAnimState(animStateID, 0.3f, NULL);
    }
}

#if RELEASE
void zAnimWTimerState::Update(zNPCBase*, F32 dt)
{
    stateTime += dt;
}

void zAnimWTimerState::Resume(zNPCBase*)
{
    stateTime = 0.0f;
}

void zAnimWTimerState::CheckTranstitions(zNPCBase*)
{
    if (endTime < stateTime) {
        SwitchState(stateEnumToGo);
    }
}
#else
void zAnimWTimerState::Update(zNPCBase*, F32 dt)
{
    stateTime += dt;
    if (endTime < stateTime) {
        SwitchState(stateEnumToGo);
    }
}
#endif
