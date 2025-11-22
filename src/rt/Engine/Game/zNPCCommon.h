#ifndef ZNPCCOMMON_H
#define ZNPCCOMMON_H

#include "zNPCBaseIN2.h"
#include "xEntDrive.h"
#include "iTime.h"
#include "zBehavior.h"
#include "zNPCPerception.h"
#include "zNPCGroup.h"
#include "zNPCOneLiner.h"
#include "zThrowable.h"
#include "zHitParameters.h"

class zNPCNavigate;
struct zNPCBlurInstance;
class zSwitchLever;
struct xModelAssetParam;
struct zNPCCombatInitData;

#ifdef DEBUGRELEASE
struct zDebugStoredEvents
{
    iTime storedTime;
    xBase* from;
    xBase* to;
    U32 to_event;
    F32* to_param;
    xBase* to_param_widget;
};

struct zDebugStoredAnimChanges
{
    iTime storedTime;
    char animName[256];
};
#endif

namespace zNPC {

class zNPCCommonMoveToTransient;
class zNPCCommonNukeTransient;
class zNPCCommonAnimTest;
class zNPCCommonPlayTaunt;
class zNPCCommonPlayCustomTransient;
class zNPCCommonInterestingIdle;

enum ezBlockDir
{
    ezBlockDir_None,
    ezBlockDir_Front,
    ezBlockDir_Around
};

class zNPCCommon : public zNPCBaseIN2
{
public:
    virtual const char* get_type_name() const;
    virtual bool HasBlockAbility();
    virtual bool HasDodgeAbility();
    virtual bool HasJumpAbility();
    virtual bool CanTakeDamage();
    virtual bool CanBeInterruptedForIncidental();
    virtual F32 GetAIHelperHintSafeDistance();
    virtual S32 GetNPCType() { return 0; }
    virtual U32 GetShadowBoneIndex();
    virtual S32 GetNukeBoneIndex();
    virtual void GetNukeBoneOffset(xVec3* offset);
    virtual void GetStunIconBoneOffset(xVec3* offset);
    virtual S32 GetShieldGeneratorBoneIndex();
    virtual void GetShieldGeneratorBoneOffset(xVec3* offset);
    virtual F32 GetShieldGeneratorBoneRotation();
    virtual bool CanHelperAITarget();
    virtual void GetPosFromBase(xBase* inBase, xVec3* outPos);
    virtual F32 GetFreezeTime();
    virtual F32 GetThawTime();
    virtual void UpdateBlurs(F32);
    virtual iSndGroupHandle GetOneLinerSoundHandle(ezNPCOneLinerType);
    virtual U32 GetInterestingIdleAnimID(U32);
    virtual U32 GetInterestingMovementAnimID(U32);
    virtual F32 GetInterestingIdleAnimTime(U32);
    virtual S32 GetMaxHitPoints();
    virtual U32 GetNPCIceType();
    virtual void InterestingIdleFX(U32);
    virtual void ResetInterestingIdleFX();
    virtual F32 GetShuffleAndTauntWaitTime();
    virtual void UpdateTauntFX(F32);
    virtual xEnt* GetAttackingPlayerEnt();
    virtual void SetAttackingPlayerEnt(xEnt* inAttackingPlayer, F32 inTime);
    virtual zNPCCombatInitData* GetCombatData();
    virtual void SetVariantFlags();
    virtual void change_bounds(F32 scale, xVec3 offset, bool use_bounding_box);
    virtual U32 GetVariant();
    virtual xGroup* GetInterestPointerGroup();
    virtual bool IsFrozen();
    virtual bool IsGrabbable();
    virtual F32 WeaponGetMaxRange();
    virtual bool HasRangeWeapon();
    virtual void BehaviorUpdate(F32);
    virtual void UpdateFrozen(F32 dt);
    virtual void before_anim_matrices(xAnimPlay* play, xQuat* quat, xVec3* tran, S32 boneCount);
    virtual void add_states(xAnimTable* table);
    virtual void add_transitions(xAnimTable* table);
    virtual void BuildStateMachine();
    virtual void SetDefaultAndEventStates();
    virtual void update_npc(F32 dt);
    virtual void GetNukeIconMatrix(xMat4x3* posMat);
    virtual F32 GetShieldMidHeight();
    virtual F32 GetShieldScale();
    virtual void GetShieldOffset(xVec3& offset);
    virtual F32 GetMaxShieldValue();
    virtual bool IsShielding();
    virtual bool FunkyCheatEffective();
    virtual F32 GetFunkyCheatScale();
    virtual void NPCFXUpdate();
    virtual void NPCFXRender();
    virtual bool destroy_throwable(zThrowableSize);
    virtual bool static_object_carry();
    virtual bool player_orient_carry();
    virtual void damage(const zCombatDamageInfo& damageInfo);
    virtual void deliver_damage(const zCombatDamageInfo& damageInfo);
    virtual bool valid_damage(const zCombatDamageInfo& damageInfo);
    virtual ezHitReaction GetDamageHitReaction(const zCombatDamageInfo& damageInfo, ezHitReactionState inState);
    virtual bool ShouldEscapeGrab();
    virtual bool grabbable(GrabType grabType);
    virtual bool giveBonus();
    virtual bool punchable();
    virtual bool Freeze(F32 intensity);
    virtual void culling_changed();
    virtual F32 GetMeshCircleRadius();
    virtual U8 get_target_priority() const;
    virtual zCombatDamageInfo* GetCurrentCombatInfo() const;
    virtual void SetCurrentCombatInfo(zCombatDamageInfo* info);
    virtual ezBlockDir GetBlockDirection() const;
    virtual void SetProjectileExtControl();
    virtual F32 GetScreenBlockTolerance();
    virtual void debug_render();
    virtual void move();
    virtual F32 DistanceToPriorityTarget();
#ifdef DEBUGRELEASE
    virtual void debug_display();
    virtual void add_tweaks(const char* prefix);
#endif
    virtual F32 CalcTranslucency();
    virtual void activate_visible();
    virtual void deactivate_visible();

public:
    xVec3 bound_offset;
    F32 move_speed_multiply;
    xEntDrive npcDrive;
    zNPCCommonMoveToTransient* moveToTransient;
    zNPCCommonNukeTransient* nukeTransient;
    zNPCCommonAnimTest* animTestState;
    zNPCCommonPlayTaunt* playTauntState;
    zNPCCommonPlayCustomTransient* playCustomTransient;
    zNPCCommonInterestingIdle* interestingIdleState;
    struct
    {
        bool human : 1;
        bool cast_shadow : 1;
        bool complex_shadow : 1;
        bool dying : 1;
        bool not_throw_target : 1;
        bool shouldGivePower : 1;
        bool hurtByPlayer : 1;
        bool carried : 1;
        bool invincible : 1;
        bool firstUpdate : 1;
        bool noDamageSmoke : 1;
        U8 pad2 : 5;
        U8 pad3 : 8;
        U8 old_chkby : 8;
    } commonFlags;
#ifdef DEBUGRELEASE
    zDebugStoredEvents storedEvents[8];
    zDebugStoredAnimChanges storedAnimChanges[8];
#endif
    zNPCNavigate* navigation_behavior;
    zBehaviorManager manager;
    F32 freezeValue;
    F32 freezeMultiplier;
    F32 freezeColorValue;
    bool skipThaw;
    bool inJumpVolume;
    bool inSlowEnemyCollisionVolume;
    bool canBlock;
    bool canDodge;
    bool canJump;
    bool canTakeDamage;
    bool delayedFinishedSpawningEvent;
    F32 nukeShockScaleRatio;
    F32 nukeRotation;
    F32 stunRotation;
    F32 stunAxisRotation;
    F32 nukeShockRotation;
    xEnt* killedBy;
    F32 shieldCurrent;
    F32 lastAnimTime;
    xSndHandle hackE3LoopingSoundHandle;
    zNPCBlurInstance* blurInstance;
    zSwitchLever* lever;
    S32 leverAction;
    zNPCPerception perception;
    F32 shadow_radius;
    S16 incredi_power_points;
    S32 grab_bone;
    xVec3 grab_offset;
    xVec3 surroundedMoveTo;
    bool surroundedValid;
    xVec3 shakeModelDisp;
#ifdef DEBUGRELEASE
    F32 tweak_scale;
    xVec3 tweak_offset;
    const char* debug_name;
    bool tweak_use_bounding_box;
    bool debug_output;
#endif

protected:
    bool off_during_cinematic;
    F32 NPCFXdt;
    F32 near_target_radius;
    F32 mass;
    F32 cull_distance;
    group* group_data;
    zCombatDamageInfo currentCombatInfo;
    ezBlockDir blockDirection;
    S16 bound_bone;
    U16 sound_id_offset;
    U16 globalParametersSize;
    xModelAssetParam* globalParameters;
    U16 overrideParametersSize;
    xModelAssetParam* overrideParameters;
    F32 shadow_strength;
    F32 attackingPlayerTimer;
    xEnt* attackingPlayerEnt;
    xGroup* interestPointerGroup;
    xModelInstance* nukeIcon;
    xModelInstance* nukeRings[6];
    xModelInstance* stunIcon;
    xModelInstance* shieldGenerator;
    F32 destroyShieldGeneratorTimer;
    F32 destroyShieldSparkTimer;
    xBound oldBound;
    xEnt* aboutToBeHitBy;
    xEnt* lastHitBy;
    bool hitByThrowable;

public:
    F32 originalScale;
    bool ourDeathHasBeenRegistered;

// ???
#ifdef DEBUGRELEASE
protected:
#endif
    F32 prevAlpha;
};

}

#endif
