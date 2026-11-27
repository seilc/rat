#ifndef ZNPCGATE_H
#define ZNPCGATE_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xMath3.h"
#include "zNPCGateAsset.h"

class zNPCBase;
struct zEntTrigger;

enum eNPCGateDir
{
    eNPCGateDir_At,
    eNPCGateDir_Right,
    eNPCGateDir_AtM,
    eNPCGateDir_RightM
};

class zNPCGate : public xBase
{
public:
    static void InitDynamic(xBase& data, xDynAsset& asset, size_t asset_size);

    zNPCGate(zNPCGateAsset* asset, U32);

    static void EventHandler(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

    void Setup();
    void SetupAttachedNPCs();
    void SetupPhysicalData();
    void Update();
    F32 ConvertWorldToGateLocalZ(const xVec3& iWorld);
    void ConvertWorldToBoxLocal(const xVec3& iWorld, xVec3& oLocal);
    void ConvertBoxLocalToWorld(const xVec3& iLocal, xVec3& oWorld);
    void HandlePlayerCollision();
#if DEBUG || RELEASE
    void DebugRender(); // UNUSED
#endif
    void Destroy();

private:
    zNPCGateAsset* npcGateAsset;
    S32 numberOfNPCs;
    zNPCBase** npcPointers;
    zEntTrigger* gateTrigger;
    xBox gateBox;
    xMat4x3* gateMat;
    eNPCGateDir gateFacing;
    xVec3 gateDir;
    F32 gateMax;
    F32 gateMin;
    bool gateIsOn;
    bool gateIsEnabled;
};

class zNPCGateMgr
{
public:
    zNPCGateMgr() { numberOfGates = 0; npcGates = NULL; curGate = 0; }
    void IncrementNumber() { numberOfGates++; }

    static void SceneSetup();
    void Assign(zNPCGate* npcGate);
    static void SceneUpdate();
    static void SceneExit();
    static void HandlePlayerCollisions();

private:
    S32 numberOfGates;
    zNPCGate** npcGates;
    S32 curGate;
};

#endif
