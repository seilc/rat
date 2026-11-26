#ifndef ZRATBALLPLAYER_H
#define ZRATBALLPLAYER_H

#include "zPlayer.h"

struct xHierarchyBoundInitData;

struct zRatBallPlayer : zPlayer
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
    virtual S32* GetHeadBones();
    virtual void RenderEffects();
#if DEBUG || RELEASE
    virtual void SetupTweaks();
    virtual const char** GetBoneNames() const;
#endif
    virtual bool IsInteractioning() { return currAction != NULL; }
    virtual bool IsJumping() const;
    virtual void InitActions();
    virtual F32 InitialDepenetrationVelocity();
    virtual void BeforeAnimMatrices(xAnimPlay*, xQuat*, xVec3*, S32);
    virtual xHierarchyBoundInitData* GetHierarchyBoundInitData() const;
    virtual F32 GetHierarchyMasterRadius() const;

    void SetupRumbles(); // UNUSED
    void Rumble(U32); // UNUSED
    void InitAtomicRenderOrder();
    void GetOrderedModels(xModelInstance** submodels, xModelInstance* model); // UNUSED
    void ImmediateRenderOrdered(xModelInstance* model); // UNUSED
    void BucketRenderOrdered(xModelInstance* model); // UNUSED
    void BoundUpdateBottomSphere(F32 radius);
    xVec3 GetBoundOffset();
    void BoundUpdateMidSphere(U32 chestBone, F32 radius);
    void Update_Particles(F32);
    void Update_FX(F32);
    void GetCenter(xVec3* pOutput); // UNUSED
#if DEBUG || RELEASE
    void DrawBounds() const;
#endif
    void Update_FlyCheatMode(F32);
    void DoCollision(xScene* sc, F32 dt, F32 settleRate); // UNUSED

#if DEBUG || RELEASE
    bool debugDrawDebugLines;
#endif
    F32 modelYaw;
    bool isOn;
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

extern F32 RB_PLSBOUND_RADIUS;
extern F32 RB_boundYShift;

#endif
