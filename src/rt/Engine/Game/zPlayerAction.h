#ifndef ZPLAYERACTION_H
#define ZPLAYERACTION_H

#include "zEnt.h"

class zPlayerActionManager;
struct zPlayer;
struct SphereCollisionResults;

class zPlayerAction
{
protected:
    zPlayerActionManager* manager;
    zPlayer* player;

public:
    virtual void AddStandardTransitionsFrom(xAnimTable* table, const char* fromStates)
    {
        AddTransitionsFrom(table, fromStates, 1000, 0.15f, 0, 0);
    }

    virtual void AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags) = 0;
    virtual U32 GetID() = 0;
    virtual void AddStates(xAnimTable* table) = 0;
    virtual void AddInternalTransitions(xAnimTable*) {}
    virtual void AddActionTransitions(xAnimTable* table) = 0;
    virtual void Reset() {}
    virtual void Move(xScene* s, F32 dt, xEntFrame* frame);
    virtual void BeginUpdate(F32 dt);
    virtual void Update(F32 dt);
    virtual void EndUpdate(F32 dt);
    virtual void NotifyCollision(const xVec3&, const SphereCollisionResults&) {}
    virtual bool CollisionResponse(const xVec3& currentDir, const SphereCollisionResults& sceneCollide, xVec3& responseDpos);
    virtual void Setup() {}
    virtual void Begin() {}
    virtual void End() {}
    virtual void Render() {}
    virtual void Timestep(F32) {}
#if DEBUG || RELEASE
    virtual void AddTweaks();
#endif

    static void AddActionTransition(xAnimTable* table, const char* source, const char* dest, xAnimTransitionCallback condition, U16 priority, F32 blend, U32 flags, U32 userFlags);
    xAnimState* NewState(xAnimTable* table, const char* name, U32 flags, U32 userFlags, F32 speed = 1.0f, F32* boneBlend = NULL, F32* timeSnap = NULL, F32 fadeRecip = 0.0f, U16* fadeOffset = NULL, xAnimBeforeEnterCallback beforeEnter = NULL, xAnimStateCallback stateCallback = NULL, xAnimBeforeAnimMatricesCallback beforeAnimMatrices = NULL);
};

#define PLAYER_ACTION_CHECK_L(cls, name, line)                                                                                                      \
    U32 name(xAnimTransition*, xAnimSingle*);                                                                                                       \
    static U32 an##name(xAnimTransition* tran, xAnimSingle* anim, void*)                                                                            \
    {                                                                                                                                               \
        xASSERT_FMT_L(line, anim->State->CallbackData == tran->Dest->CallbackData,                                                                  \
            "Action transition that didn't use the action transition function. This will potentially cause an error.");                             \
        return ((cls*)anim->State->CallbackData)->name(tran, anim);                                                                                 \
    }
#define PLAYER_ACTION_CHECK(cls, name) PLAYER_ACTION_CHECK_L(cls, name, __LINE__)

#define PLAYER_ACTION_DEST_CHECK(cls, name)                                                       \
    U32 name(xAnimTransition*, xAnimSingle*);                                                     \
    static U32 an##name(xAnimTransition* tran, xAnimSingle* anim, void*)                          \
    {                                                                                             \
        return ((cls*)tran->Dest->CallbackData)->name(tran, anim);                                \
    }

class zPlayerActionManager
{
public:
    void SetCurrentAction(zPlayerAction* newCurrentAction);
    void Init(U32 actionCount);
    void Add(zPlayer* player, zPlayerAction* action);
    void Setup();
    void Reset();
    U32 GetCurrentActionID() const;
    void AddStandardTransitionsTo(U32 actionID, xAnimTable* table, const char* fromStates);

protected:
    zPlayerAction** actionList;
    zPlayerAction* currentAction;
    U32 listSize;
};

#endif
