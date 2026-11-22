#include "xCommon.h"
#include "zNPCContextSensitive.h"

#include "zComponentPool.h"
#include "zCompLogicContextSensitive.h"
#include "zCompSteeringBasic.h"
#include "zICompNPCEntity.h"

bool zNPCContextSensitive::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicContextSensitive, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);
    npcSteeringComp = zComponentPool<zCompSteeringBasic, zICompSteering, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp || !npcSteeringComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCContextSensitive::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCContextSensitive::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCContextSensitive::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();

    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
