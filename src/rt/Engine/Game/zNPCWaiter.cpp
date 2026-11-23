#include "xCommon.h"
#include "zNPCWaiter.h"

#include "zComponentPool.h"
#include "zCompLogicWaiter.h"
#include "zICompNPCEntity.h"

static zICompNPCEntityParams gWaiterEntParameters;

bool zNPCWaiter::Activate()
{
    gWaiterEntParameters.boundRadius = 0.4f;
    gWaiterEntParameters.boundOffset.y = 0.2f;

    npcLogicComp = zComponentPool<zCompLogicWaiter, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, &gWaiterEntParameters);

    if (!npcLogicComp || !npcEntityComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCWaiter::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCWaiter::Update(xScene* xscn, F32 dt)
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

void zNPCWaiter::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity)
        compNPCEntity->Render();
}
