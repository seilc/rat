#include "xCommon.h"
#include "zNPCSimpleton.h"

#include "zComponentPool.h"
#include "zCompLogicSimpleton.h"
#include "zICompNPCEntity.h"
#include "zCompSteeringBasic.h"

bool zNPCSimpleton::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicSimpleton, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);
    npcSteeringComp = zComponentPool<zCompSteeringBasic, zICompSteering, zNPCBase>::AttachComponent(this, NULL);

    if (npcLogicComp == NULL || npcEntityComp == NULL || npcSteeringComp == NULL) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCSimpleton::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCSimpleton::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();
    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCSimpleton::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
