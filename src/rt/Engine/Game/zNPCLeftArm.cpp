#include "xCommon.h"
#include "zNPCLeftArm.h"

#include "zComponentPool.h"
#include "zCompLogicLeftArm.h"
#include "zICompNPCEntity.h"

bool zNPCLeftArm::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicLeftArm, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCLeftArm::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCLeftArm::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();
    zICompNPCEntity* compNPCEntity = GetEntityComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    if (compNPCEntity) {
        compNPCEntity->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCLeftArm::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
