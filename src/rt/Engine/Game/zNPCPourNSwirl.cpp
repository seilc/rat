#include "xCommon.h"
#include "zNPCPourNSwirl.h"

#include "zComponentPool.h"
#include "zCompLogicPourNSwirl.h"
#include "zICompNPCEntity.h"

bool zNPCPourNSwirl::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicPourNSwirl, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCPourNSwirl::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCPourNSwirl::Update(xScene* xscn, F32 dt)
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

void zNPCPourNSwirl::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
