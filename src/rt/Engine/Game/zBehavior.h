#ifndef ZBEHAVIOR_H
#define ZBEHAVIOR_H

#include "xEnt.h"
#include "zCombat.h"

class behavior;
struct zEntTrigger;
struct SphereCollisionResults;
struct zPlayer;

enum GrabType
{
    eGrabTypeRemyStanding,
    eGrabTypeElastigirl,
    eGrabTypeCount
};

struct zTransientNodeData
{
    void* contextDataPtr[4];
    F32 contextDataF32[4];
    S32 contextDataS32[4];
};

struct zTransientNode
{
    behavior* transient;
    zTransientNodeData contextData;
};

class behavior
{
protected:
    xEnt* owner;

public:
    typedef bool(*testFunc)(xEnt*, behavior*, behavior*);

    virtual bool inAnimState(const char* state, F32 look_ahead, xModelInstance* model) const;
    virtual bool inAnimState(U32 stateID, F32 look_ahead, xModelInstance* model) const;
    virtual bool inAnimStateRange(U32 stateIDstart, U32 stateIDend, F32 look_ahead, xModelInstance* model) const;
    virtual F32 getAnimDuration();
    virtual F32 getAnimTime();
    virtual bool isAnimStateStopped(const char* state, xModelInstance* model) const;
    virtual bool isAnimStateStopped(U32 stateID, xModelInstance* model) const;
    virtual U32 getThrowAnimCode();
    virtual const char* getName();
    virtual void setOwner(xEnt* new_owner);
    virtual xEnt* getOwner();
    virtual bool systemEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual bool damage(zCombatDamageInfo& damageInfo);
    virtual void add_states(xAnimTable* table);
    virtual void add_transitions(xAnimTable* table);
    virtual void enterBehavior(const behavior* prev_behavior) = 0;
    virtual void updateBehavior(F32 dt);
    virtual void exitBehavior();
    virtual void SetAsNotCurrent();
    virtual void SetAsCurrent();
    virtual void setup();
    virtual void reset();
    virtual void exit();
    virtual void setupTransientDataPtr(zTransientNodeData* inData);
    virtual void addTest(behavior* destBehavior, testFunc inTestFunc);
    virtual void addTest(behavior* destBehavior, testFunc inTestFunc, char* description, F32 waitTime);
    virtual void replaceTest(behavior* oldDestBehavior, testFunc oldTestFunc, behavior* newDestBehavior, testFunc newTestFunc);
    virtual void replaceTest(behavior* oldDestBehavior, testFunc oldTestFunc, behavior* newDestBehavior, testFunc newTestFunc, char* description);
    virtual void insertTest(behavior* oldDestBehavior1, testFunc oldTestFunc1, behavior* oldDestBehavior2, testFunc oldTestFunc2, behavior* newDestBehavior, testFunc newTestFunc);
    virtual void insertTest(behavior* oldDestBehavior1, testFunc oldTestFunc1, behavior* oldDestBehavior2, testFunc oldTestFunc2, behavior* newDestBehavior, testFunc newTestFunc, char* description);
    virtual void setFlags(U32 inFlags);
    virtual U32 getFlags();
    virtual bool isAttackState();
    virtual void preRender();
    virtual void render();
    virtual bool runnable();
    virtual bool disableTargetTest();
    virtual void beforeAnimMatrices(xAnimPlay* play, xQuat* quat, xVec3* tran, S32 boneCount);
    virtual bool grabbable(GrabType grabType);
    virtual bool punchable();
    virtual bool DamageTransitionAllowable();
    virtual bool DamageCanBeDealt();
    virtual bool EscapeNoEnterTransitionAllowable();
    virtual bool JumpTransitionAllowable();
    virtual bool CoverTransitionAllowable();
    virtual bool DodgeTransitionAllowable();
    virtual bool TreatNoEnterAsCollision();
    virtual void NoEnterCollision(zEntTrigger*);
    virtual bool CanBeFrozen();
    virtual bool collisionResponse(const xVec3& current_dir, const SphereCollisionResults& scene_collide, xVec3& response_dpos, bool);
    virtual void registerCollision(const SphereCollisionResults& scene_collide);
    virtual U32 ThrowableDestroy(void*, bool);
    virtual U32 ThrowableCollide(F32&, xEnt*, const xVec3*, const xVec3*, F32, F32, void*);
    virtual F32 delay();
    virtual bool CanBeTargeted();
    virtual bool CanBeInterruptedForIncidental();
    virtual F32 GetAIHelperHintSafeDistance();
    virtual bool isTransientComplete();
    virtual void setAnimState(const char* new_state, F32 blend, xModelInstance* model);
    virtual void setAnimState(const char* new_state, F32 blend, F32 dest_time, xModelInstance* model);
    virtual void setAnimState(U32 new_stateID, F32 blend, xModelInstance* model);
    virtual void setAnimState(U32 new_stateID, F32 blend, F32 dest_time, xModelInstance* model);
    virtual bool hasAnimEnded(xModelInstance* model);

#ifdef DEBUGRELEASE
    virtual void debugDisplay();
    virtual void debugRender();
    virtual void addStaticTweaks();
    virtual void addTweaks(const char*);
    virtual const char* getTweakName();
#endif

protected:
    struct testNode
    {
        testFunc testFunction;
        behavior* changeToBehavior;
        testNode* next;
#ifdef DEBUGRELEASE
        char* description;
#endif
        F32 waitTime;
        F32 remainingTime;
    };

    testNode testList;
    zTransientNodeData* transientData;

public:
    U32 type;
    U32 behaviorFlags;
};

struct behavior_node
{
    S32 priority;
    behavior* data;
    behavior_node* next;
    behavior_node* previous;
    F32 delay;
};

class zBehaviorManager
{
protected:
    behavior_node behaviorList;
    behavior_node* behaviorEndOfList;
    behavior* behaviorCurrentNonTransient;
    behavior* behaviorDefault;
    behavior* behaviorDamage;
    behavior* behaviorProjectile;
    behavior* behaviorCarry;
    behavior* behaviorThrow;
    behavior* behaviorFrozen;
    behavior* behaviorThaw;
    behavior* behaviorNuke;
    behavior* behaviorFrozenCarry;
    behavior* behaviorFrozenThrow;
    behavior* behaviorDeath;
    behavior* behaviorBlock;
    behavior* behaviorBlockHit;
    behavior* behaviorEscapeNoEnter;
    behavior* behaviorWaitNoEnter;
    behavior* behaviorJump;
    behavior* behaviorCover;
    behavior* behaviorShield;
    behavior* behaviorRangeAttack;
    behavior* behaviorFrozenDamage;
    zTransientNode* transientBehavior;
    S32 transientTotal;
    S32 transientMax;
    F32 inStateTimer;
    U32 enteredBehavior;

public:
    virtual bool TestForJumpBehaviorChange(F32);
    virtual bool TestForShieldBehaviorChange(F32);
    virtual bool TestForDeathVolumeBehaviorChange(F32);
    virtual void testForBehaviorChange(F32 dt);
    virtual bool testForCompleteChange(F32);
    virtual void setDefaultBehavior(behavior* inDefault);
    virtual void preupdate(F32);
    virtual void update(F32 dt);
    virtual void staticSetup();
    virtual void reset();
    virtual void exit();
    virtual void add_states(xAnimTable* table);
    virtual void add_transitions(xAnimTable* table);
    virtual void preRender();
    virtual void render();
    virtual void beforeAnimMatrices(xAnimPlay* play, xQuat* quat, xVec3* tran, S32 boneCount);
    virtual bool systemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
    virtual void nuke(xEnt*, xVec3* sourcePos, F32 inTime, F32 inRadius, F32 outRadius);
    virtual void damage(zCombatDamageInfo& damageInfo);
    virtual void carry(zPlayer*);
    virtual bool collisionResponse(const xVec3& current_dir, const SphereCollisionResults& scene_collide, xVec3& response_dpos);
    virtual void registerCollision(const SphereCollisionResults& scene_collide);
    virtual void RunDamageBehavior(behavior* dmgBehavior, zCombatDamageInfo& damageInfo);

#ifdef DEBUGRELEASE
    virtual void debugDisplay();
    virtual void debugRender();
    virtual void addTweaks(const char*);
#endif
};

#endif
