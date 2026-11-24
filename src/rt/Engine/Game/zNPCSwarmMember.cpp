#include "xCommon.h"
#include "zNPCSwarmMember.h"

#include "xRand.h"
#include "zComponentPool.h"
#include "zCompLogicSwarmMember.h"
#include "zCompSteeringFlocking.h"
#include "zCompSteeringFlockingBrain.h"
#include "zICompNPCEntity.h"
#include "zNPCAssetSwarm.h"

static zICompNPCEntityParams gSwarmBugEntParameters;
static zICompNPCEntityParams gSwarmOwlEntParameters;

bool zNPCSwarmMember::Activate()
{
    gSwarmOwlEntParameters.useBoundingBox = true;

    gSwarmBugEntParameters.hasCollision = false;

    switch (parent->GetNPCType()) {
    case eNPCType_Swarm_Bug:
        npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, &gSwarmBugEntParameters);
        break;
    case eNPCType_Swarm_Owl:
        npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, &gSwarmOwlEntParameters);
        break;
    default:
        xFAIL_MSG_L(50, "Unknown swarmer type");
        break;
    }

    npcLogicComp = zComponentPool<zCompLogicSwarmMember, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcSteeringComp = zComponentPool<zCompSteeringFlocking, zICompSteering, zNPCBase>::AttachComponent(this, NULL);

    if (!npcEntityComp || !npcLogicComp || !npcSteeringComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCSwarmMember::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCSwarmMember::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCSwarmMember::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}

void zNPCSwarmMember::Reset()
{
#if RELEASE
    zNPCBase::Reset();
#endif

    zNPCAssetSwarm* npcassSwarm = (zNPCAssetSwarm*)npcAsset;

    lastPos.x = xrand_RandomFloatRange(-1.0f, 1.0f);
    lastPos.y = 0.0f;
    lastPos.z = xrand_RandomFloatRange(-1.0f, 1.0f);
    lastPos.right_normalize();

    F32 distFromCenter = xrand_RandomFloatRange(0.0f, npcassSwarm->spawnRadius);

    lastPos.Scale(distFromCenter);
    lastPos.Add(npcassSwarm->ent_asset.pos);

    lastOrientation.x = xrand_RandomFloatRange(0.0f, 2.0f * PI);
    lastOrientation.y = 0.0f;
    lastOrientation.z = 0.0f;
}
