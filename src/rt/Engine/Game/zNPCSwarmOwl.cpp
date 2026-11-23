#include "xCommon.h"
#include "zNPCSwarmOwl.h"

#include "zComponentPool.h"
#include "zCompLogicSwarmOwl.h"
#include "zCompSound.h"
#include "zCompSteeringFlockingBrain.h"
#include "zNPCAssetSwarm.h"

S32 zNPCSwarmOwl::CountChildrenInAsset(const xDynAsset* dynAsset, U32, const char*)
{
    zNPCAssetSwarm* npcassSwarm = (zNPCAssetSwarm*)dynAsset;
    return npcassSwarm->memberNumber;
}

bool zNPCSwarmOwl::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicSwarmOwl, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcSteeringComp = zComponentPool<zCompSteeringFlockingBrain, zICompSteering, zNPCBase>::AttachComponent(this, NULL);
    npcSoundComp = zComponentPool<zCompSound, zICompSound, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcSteeringComp || !npcSoundComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCSwarmOwl::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCSwarmOwl::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCSwarmOwl::UpdateNPCBaseData()
{
    zCompSteeringFlockingBrain* flockingBrain = (zCompSteeringFlockingBrain*)npcSteeringComp;
    xASSERT_FMT_L(DEBUG ? 72 : 66, flockingBrain, "This Swarmer has no steering component attached. Its position and orientation cannot be determined. Please either add steering component or fix the UpdateNPCBase function");

    xVec3Copy(&lastPos, flockingBrain->GetFlockingCenter());
}

void zNPCSwarmOwl::Render()
{
    DebugRenderAllComponents();
}
