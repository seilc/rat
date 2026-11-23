#include "xCommon.h"
#include "zNPCStirringPlayer.h"

#include "zComponentPool.h"
#include "zCompLogicStirringPlayer.h"
#include "zICompNPCEntity.h"

bool zNPCStirringPlayer::Activate()
{
    npcLogicComp = zComponentPool<zCompLogicStirringPlayer, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCStirringPlayer::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCStirringPlayer::Update(xScene* xscn, F32 dt)
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

void zNPCStirringPlayer::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
