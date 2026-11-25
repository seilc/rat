#ifndef ZCOMMONPLAYER_H
#define ZCOMMONPLAYER_H

#include "zPlayer.h"
#include "zContextObject.h"
#include "zPlayerConstrainer.h"
#include "zPlayerOneLinerData.h"

struct pointer_asset;
struct xEnv;
struct xHierarchyBoundInitData;
struct xLight;
struct zSurfAssetIN;
class zCarryableObject;
class zInteraction;
class zNPCBase;
class zSpringboard;

enum zCommonPlayerUpgradeType
{
    ezCommonPlayerUpgradeType_Melee,
    ezCommonPlayerUpgradeType_AirMelee,
    ezCommonPlayerUpgradeType_Distance,
    ezCommonPlayerUpgradeType_Health,
    ezCommonPlayerUpgradeType_NukeCnt,
    ezCommonPlayerUpgradeType_Count
};

enum zCommonPlayerUpgradeLevel
{
    ezCommonPlayerUpgradeLevel_Normal,
    ezCommonPlayerUpgradeLevel_Level1,
    ezCommonPlayerUpgradeLevel_Level2,
    ezCommonPlayerUpgradeLevel_Count
};

struct zCommonPlayer : zPlayer
{
    enum LedgeState
    {
        LedgeState_None,
        LedgeState_JumpInterp,
        LedgeState_Grab,
        LedgeState_Count,
        LedgeState_ForceSize = FORCEENUMSIZEINT
    };

    struct LedgeGrabParams
    {
        LedgeState state;
        struct
        {
            U8 hasGrabbedLedge : 1;
            U8 inLedgeAnim : 1;
            U8 foundValidLedge : 1;
            U32 pad : 27;
        } flags;
        F32 delay;
        xVec3 playerToLedge;
        xVec3 player_bound_pos;
        xVec3 grab_position;
        xVec3 flipUpTrans;
    };

    struct FootLockInfo
    {
        xVec3 offset;
        S32 bone;
        xVec3 lockPos;
    };

    class StepFX
    {
        struct zFootstepDetect
        {
            xVec3 last_emit_position;
            U8 was_lifted;
        };

        class zDecalEmitterList;

        zSurfAssetIN* currentSurface;
        zSurfAssetIN* lastSurface;
        F32 lastSurfaceTime;
        zFootstepDetect footsteps[2];
        zDecalEmitterList* decals;
        F32 dust_emit_time_L;
        F32 dust_time_L;
        F32 dust_emit_remaining_L;
        F32 dust_emit_time_R;
        F32 dust_time_R;
        F32 dust_emit_remaining_R;
    };

    enum HitType
    {
        eHitType_None = -1,
        eHitType_BackGround,
        eHitType_FrontGround,
        eHitType_BackAir,
        eHitType_FrontAir,
        eHitType_BackPlayerHit,
        eHitType_FrontPlayerHit,
        eHitType_InfiniteFall,
        eHitType_FallInGoo
    };

    struct Point
    {
        F32 value;
        F32 time;
    };

    struct MoveSupressParams
    {
        Point points[3];
        U8 pointsInitialized[3];
        U8 verified_init;
        U32 num_segments;
        F32 cur_time;
        F32 cur_supression;
    };

    enum AutoMoveMode
    {
        eAutoMoveOff,
        eAutoMoveMoving,
        eAutoMoveInterp
    };

    struct zPlayerCarryInfo
    {
        zCarryableObject* grabbed;
        U32 grabbedModelID;
        U32 animCode;
        xVec3 targetVelocity;
        U32 lerpedIn;
        xMat4x3 spin;
        xQuat startQuat;
        xQuat endQuat;
        xVec3 objScale;
        xVec3 physicsTranslation;
        F32 radius;
        bool orientToPlayer;
        U8 destroyed;
        F32 targetRot;
        F32 startRot;
        U32 grabTarget;
        xVec3 grabOffset;
        F32 grabLerpLast;
        U32 grabYclear;
        F32 grab_emit_remaining;
        F32 grab_dust_time;
    };

    static U32 NUKE_CAPACITY_START;
#if DEBUG || RELEASE
    static S32 MAX_STATES;
#endif

    zContextObjectManager contextManager;
    bool has_picked_up_incredimeter;
    bool has_picked_up_health;
    bool has_grabbed_zipline;
#if DEBUG || RELEASE
    xLight* light;
    bool lightOn;
#endif
    F32 stickFlickMinTimer;
    F32 stickFlickMaxTimer;
    F32 stickFlickAngle;
    F32 stickFlickUnit[2];
    LedgeGrabParams ledge;

protected:
    zPlayerOneLinerData oneLinerData;
    F32 collisionMasterRadius;

public:
    S32 surfaceDamage;
    zHitSource surfaceSource;
    xVec3 surfaceKnockback;

protected:
    F32 surfAccelWalk;
    F32 surfAccelRun;
    F32 surfDecelIdle;
    F32 surfDecelSkid;
    F32 surfSlickRatio;
    F32 surfSlickTimer;
    F32 surfPeakRatio;
    F32 surfMaxSpeed;
    F32 surfSlipTimer;
    F32 dust_emit_time;
    F32 dust_time;
    F32 dust_emit_remaining;
    F32 smoke_emit_increment;
    F32 max_smoke_emit_time;
    F32 smoke_emit_timer;
    F32 smoke_emit_remaining;
    F32 smoke_offset;
    FootLockInfo leftFoot;
    FootLockInfo rightFoot;

public:
    StepFX stepfx;

private:
    F32 hangTimer;
    F32 last_dt;

public:
    HitType currentHitType;
    xVec3 damageVec;
    F32 hitTimer;
    S32 hitJuggleCount;

private:
    MoveSupressParams move_supress;

public:
    zInteraction* currAction;
    F32 walkToActionTimer;
    F32 autoMoveSpeed;
    AutoMoveMode autoMoveMode;
    F32 autoMoveDist;
    pointer_asset* autoMovePointer;
    xBase* autoMoveSender;
    F32 extraIdleTimer;
    zSpringboard* springboard;
    zNPCBase* interactingNPC;
    F32 walkLerp;
    S32 finishingMove;
    zNPC::zNPCCommon* lastHitNPC;

protected:
    U32 powerUpTbl[5];

private:
    U32 expPoints;
    U32 expPointsToUpgrade;
    U32 nukes;
    U8 speed;

public:
    U32 upgradeLevel;
    U32 uCommonPlayerFlags;

private:
    xMat4x3 playerAbsMat;
    xVec3 lastFloorNorm;
    F32 maxVelmag;
    F32 leanLerp;
    F32 moveSpeedNormal[6];
    F32 moveSpeedJump[6];
    F32 normalTurnFactor;
    F32 attackTurnFactor;
    F32 airTurnFactor;
    F32 stoppedTurnSpeedUp;
    F32 animWalk[3];
    F32 animRun[3];
#if DEBUG || RELEASE
    xAnimState* lastState;
    xAnimState* stateStack[16];
#endif
    struct
    {
        S16 incrediPower;
    } checkpoint;
    bool shouldTurnToFace;
    bool allowTurnToFaceAbort;
    F32 turnToFaceAngle;
    F32 turnToFaceSpeed;
    F32 lastMag;
    F32 lastAngle;
    F32 moveAcceleration;
    F32 moveDeceleration;
    F32 runTimer;
    F32 minimumRunTime;
    F32 turnInPlaceTimer;
    F32 stickMagnitude;
    F32 stickAngle;
    F32 facingAngle;
    bool facingAngleValid;
    bool useThrowTarget;
    xVec3 throwTarget;
    F32 skidStopDeceleration;
    F32 skidStopDecelTime;
    F32 skidStopLerpTime;
    zPlayerConstrainer constrainer;
    bool pushedPlayerLastFrame;
    xVec3 pushedPlayerEnemyPos;

public:
    zPlayerCarryInfo carry;

    virtual S32 EnvCollision(xSweptSphere* sphere, xEnv*);
    virtual void BreakGrab();
    virtual void BeforeAnimMatrices(xAnimPlay*, xQuat*, xVec3*, S32);
    virtual void LoadUpgrades();
    virtual U8 DoCriticalCollisionCheck() const;
    virtual xHierarchyBoundInitData* GetHierarchyBoundInitData() const;
    virtual F32 GetHierarchyMasterRadius() const;
    virtual void AbsControl(F32 x, F32 z, F32 dt);
    virtual void Rumble(U32 rumbleType);
    virtual void SetupRumbles();
    virtual void HandleFootstepCustom(zSurfAssetIN*);
    virtual void SavedFriend();
    virtual void SetUpgrade(zCommonPlayerUpgradeType forThisType, zCommonPlayerUpgradeLevel level);

    zCommonPlayerUpgradeLevel GetUpgradeLevel(zCommonPlayerUpgradeType forThisType);
    void GiveExperiencePoints(U32 points);
    U32 GetExperiencePoints() { return expPoints; }
};

#endif
