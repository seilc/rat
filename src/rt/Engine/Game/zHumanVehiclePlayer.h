#ifndef ZHUMANVEHICLEPLAYER_H
#define ZHUMANVEHICLEPLAYER_H

#include "zPlayer.h"
#include "zContextObject.h"

struct xHierarchyBoundInitData;

struct zHumanVehiclePlayer : zPlayer
{
    virtual void Init(xEntAsset* asset);
    virtual void SceneSetup();
    virtual void BoundUpdate(xVec3*);
    virtual bool AllowInteraction(zInteraction*);
    virtual void DefaultBeginUpdate(F32 dt);
    virtual void DefaultUpdate(F32 dt);
    virtual void HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void Reset();
    virtual void Exit();
    virtual bool Damage(const zCombatDamageInfo& damageInfo);
    virtual S32* GetHeadBones();
    virtual void RenderEffects();
#if DEBUG || RELEASE
    virtual void SetupTweaks();
    virtual const char** GetBoneNames() const;
#endif
    virtual bool IsInteractioning() { return currAction != NULL; }
    virtual bool IsJumping() const;
    virtual void ParseIni();
    virtual void InitActions();
    virtual F32 InitialDepenetrationVelocity();
    virtual void BeforeAnimMatrices(xAnimPlay*, xQuat*, xVec3*, S32);
    virtual xHierarchyBoundInitData* GetHierarchyBoundInitData() const;
    virtual F32 GetHierarchyMasterRadius() const;

    void SetupRumbles();
    void Rumble(U32 rumbleType);
    void InitAtomicRenderOrder();
    void GetOrderedModels(xModelInstance** submodels, xModelInstance* model); // UNUSED
    void ImmediateRenderOrdered(xModelInstance* model); // UNUSED
    void BucketRenderOrdered(xModelInstance* model); // UNUSED
    void BoundUpdateBottomSphere(F32 radius);
    xVec3 GetBoundOffset(); // UNUSED
    void BoundUpdateMidSphere(U32, F32 radius);
    void Update_Particles(F32);
    void Update_FX(F32);
    void GetCenter(xVec3* pOutput);
#if DEBUG || RELEASE
    void DrawBounds() const;
#endif
    void Update_FlyCheatMode(F32);
    void LeanUpdate(F32 diffAngle, F32 dt);
    void HVCalcAnimSpeed(F32 mag, F32* animSpeeds);

    zContextObjectManager contextManager;
    bool isOn;
    F32 hitTimer;
    F32 leanLerp;
    F32 animWalk[3];
    F32 animRun[3];
#if DEBUG || RELEASE
    bool debugDrawDebugLines;
#endif
    F32 modelYaw;
    xVec3 velocity;
    xSndGroupHandle soundAsset[2];
    xSndHandle soundSkidLoopHandle;
    zInteraction* currAction;

private:
    U32* atomics_order;
    U32 atomics_count;
    xVec3 bone_adjust;
    xVec3 prev_at;
    xVec3 prev_bi_normal;
    F32 x_lean;
    F32 y_lean;
    F32 last_sign;
    U32 modelRender;
};

extern F32 PLSBOUND_RADIUS;
extern F32 PLSBOUND_MIDRADIUS;
extern F32 boundYShift;

#endif
