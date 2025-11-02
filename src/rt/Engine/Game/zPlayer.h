#ifndef ZPLAYER_H
#define ZPLAYER_H

#include "zEnt.h"
#include "zPlayerAction.h"
#include "zBrainTypes.h"
#include "xModelBlur.h"
#include "zPlayerInput.h"
#include "xJSP.h"
#include "zNPCAttacker.h"
#include "xEntDrive.h"
#include "zScene.h"
#include "zCombat.h"

struct zSlideCam;
struct xModelAssetParam;
class zInteraction;
struct xSweptSphere;
struct SphereCollisionResults;

struct PlayerStats
{
    S32 enemiesDefeated;
    S32 meleeDamage;
    S32 airMeleeDamage;
    S32 rangeDamage;
    S32 dodgeDamage;
    S32 finishingMoves;
    S32 damageTaken;
    S32 biggestCombo;
    S32 timesDefeated;
};

struct zPlayerCheckPoint
{
    xVec3 position;
    F32 rotation;
};

struct zPlayer : zEnt
{
public:
    zPlayerActionManager actionManager;
    S32 index;
    
public:
    virtual void Init(xEntAsset* asset);
    virtual void SceneSetup();
    virtual void SceneEnter(zScene*);
    virtual void SceneExit(zScene*);
    virtual void BoundUpdate(xVec3* _pos) = 0;
    virtual bool AllowInteraction(zInteraction*);
    virtual void AttackHit(bool);
    virtual void BeginUpdate(F32 dt);
    virtual void DefaultBeginUpdate(F32 dt);
    virtual void Update(F32 dt);
    virtual void DefaultUpdate(F32 dt);
    virtual void EndUpdate(F32 dt);
    virtual void DefaultEndUpdate(F32 dt);
    virtual void Move(xScene* s, F32 dt, xEntFrame* frame);
    virtual void DefaultMove(xScene* s, F32 dt, xEntFrame* frame);
    virtual void Render();
    virtual bool WasRendered() const;
    virtual void HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void Reset();
    virtual bool IsDead() const;
    virtual void Exit();
    virtual void Save(xSerial* serial);
    virtual void Load(xSerial* serial);
    virtual bool Damage(const zCombatDamageInfo& damageInfo);
    virtual xEnt* GetCarriedEntity();
    virtual S32* GetHeadBones();
    virtual void PostRenderCleanup();
    virtual void RenderCustomUI();
    virtual bool CanTakeDamage() const;
    virtual void LoadCheckPoint();
    virtual void LoadCheckPointB(xVec3 pos, F32 ang);
    virtual void StoreCheckPoint(const zPlayerCheckPoint& theCheckPoint);
    virtual void SetCamera();
    virtual void RenderEffects();
    virtual void SetupTweaks();
    virtual void GiveHealth(S32 hitPoints, bool from_pickup);
    virtual void ResetHealth();
    virtual bool NeedsHealth() const;
    virtual const xVec3& GetFloorNormal() const;

    virtual bool IsFloorColliding() const
    {
        return Get_floor_collision() && CollisionOn();
    }

    virtual const xVec3* GetFloorPosition() const;
    virtual const char** GetBoneNames() const;
    virtual bool IsVisible() const;
    virtual void RenderTransparent(F32 alpha);
    virtual bool NeedsIncrediPower() const;
    virtual void GiveIncrediPower(S16, bool);
    virtual void TakeIncrediPower(F32);
    virtual bool IsSwimming() const;

    bool CollisionOn() const
    {
        return Get_collisionOn();
    }

    virtual bool FindShortestCollision(xSweptSphere& sws, U32& collisionFlags, S32 penby, bool);
    virtual void FindStaticCollisions(const xSphere** spheres, S32 numSpheres, const xVec3& initialDisplacement, const xVec3& validDepenetration);
    virtual xVec3 get_target();
    virtual F32 GetScreenFadeTime();
    virtual F32 StartScreenFadeAt();
    virtual F32 PredictY(F32 time) const;
    virtual bool IsInteractioning();

    bool Get_humanPlayer() const
    {
        return (zPlayerFlags >> 18) & 0x1;
    }

    bool Get_floor_collision() const
    {
        return (zPlayerFlags >> 1) & 0x1;
    }

    bool Get_collisionOn() const
    {
        return (zPlayerFlags >> 3) & 0x1;
    }

    virtual void SetEnemyIsNear(bool isit);
    virtual void EnemyKilled(zNPC::zNPCCommon* npc);
    virtual bool IsJumping() const = 0;
    virtual void ParseIni();
    virtual void AddStates(xAnimTable* table);
    virtual void AddTransitions(xAnimTable*);
    virtual void InitActions() = 0;
    virtual bool CollisionResponse(const xVec3& currentDir, const SphereCollisionResults& sceneCollide, xVec3& responseDpos);
    virtual bool DefaultCollisionResponse(const xVec3& current_dir, const SphereCollisionResults& scene_collide, xVec3& response_dpos);
    virtual void RegisterCollision(const SphereCollisionResults& sceneCollide);
    virtual bool StayOnGround();
    virtual bool CollideWithNPC();
    virtual F32 InitialDepenetrationVelocity();
    virtual void ReactiveAnimationCollided();

public:
    struct mblur_data
    {
        xModelBlur::config_data cfg;
        xModelBlur blur;
    };

    xVec3 trigLastFramePos;
    xScene* sc;
    xMat4x3 lastmat;
    PlayerStats playStats;
    eBrainType brain_id;
    xVec3 floor_pos;
    xVec3 floor_normal;
    xSphere extraSpheres[5];
    S32 numExtraSpheres;
    S32 zPlayerFlags;
    F32 collision_underneath_height;
    F32 collision_underneath_time;
    mblur_data* mblur;

protected:
    zPad* pad;
    F32 slipFadeTimer;
    S32 slide;
    F32 slideTimer;
    F32 controlOffTimer;
    F32 controlOnTime;
    U32 controlOff;
    U32 controlOnEvent;
    zPlayerCheckPoint checkPoint;
    U8 cheatMode;
    zPlayerInput playerInput;
    xVec3 lastDeltaPos;
    F32 update_dt;
    F32 last_update_dt;
    xVec3 update_motion;
    xVec3 previous_position;
    xVec3 predictRotate;
    xVec3 predictTranslate;
    F32 predictAngV;
    xVec3 predictCurrDir;
    F32 predictCurrVel;
    zSlideCam* slideCam;
    xJSPHeader* floor_jsp;
    xClumpCollBSPTriangle* floor_bspTriangle;
    xVec3 floorNorm;
    S32 slope;
    F32 floor_collision_timer;
    F32 floor_distance;
    xEnt* floor_entity;
    U32 floor_oid;
    xSurface* floor_surface;
    U32 collisionFlags;
    U8 currentIsNearLedge;
    U8 currentIsNearLedgeValid;
    F32 fallingTime;
    F32 idleAnimationTimer;
    U32 lorezModelID;
    xModelInstance* lorezModel;
    xModelInstance* hirezModel;
    zNPCAttackerData npcAttacker[3];
    zNPC::zNPCCommon* roundRobinMelee;
    F32 roundRobinTimeOut;
    F32 enemyKilledComboTimer;
    S32 enemyKilledCombo;

private:
    F32 depenetration_velocity;
    xEntShadow entShadow_embedded;
    xShadowSimpleCache simpShadow_embedded;
    U32 loaded_assetid;
    xEntDrive drv;
    U16 parametersSize;
    xModelAssetParam* parameters;
    xEnt* reticleTarget;
    RpAtomic* reticleModel;
    F32 reticleRot;
    F32 reticleAlpha;
    xMat4x3 reticleMat;
    F32 idleSoundTimer;
    U32 oldSlidePen;

#ifdef DEBUGRELEASE
    struct
    {
        U8 debugging;
        xVec3 positions[180];
        xVec3 startPos;
        xVec3 endPos;
        S32 posCount;
    } physicsDebug;
#endif

public:
    U32* hitSoundsMap;
};

#endif
