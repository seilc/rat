#include "xCommon.h"
#include "zNPCStirringRemy.h"

#include "zComponentPool.h"
#include "zCompLogicStirringRemy.h"
#include "zCompSteeringBasic.h"
#include "zICompNPCEntity.h"

bool zNPCStirringRemy::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicStirringRemy, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);
    npcSteeringComp = zComponentPool<zCompSteeringBasic, zICompSteering, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp || !npcSteeringComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCStirringRemy::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCStirringRemy::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCStirringRemy::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
