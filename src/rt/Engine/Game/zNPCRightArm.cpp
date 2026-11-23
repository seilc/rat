#include "xCommon.h"
#include "zNPCRightArm.h"

#include "zComponentPool.h"
#include "zCompLogicRightArm.h"
#include "zICompNPCEntity.h"

bool zNPCRightArm::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicRightArm, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCRightArm::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCRightArm::Update(xScene* xscn, F32 dt)
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

void zNPCRightArm::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
