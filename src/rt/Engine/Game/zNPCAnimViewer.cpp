#include "xCommon.h"
#include "zNPCAnimViewer.h"

#include "zComponentPool.h"
#include "zCompLogicAnimViewer.h"
#include "zICompNPCEntity.h"

bool zNPCAnimViewer::mIsPresent;

bool zNPCAnimViewer::Activate()
{
    mIsPresent = true;

    npcLogicComp = zComponentPool<zCompLogicAnimViewer, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCAnimViewer::Deactivate()
{
    DetachAllComponents();
}

void zNPCAnimViewer::SceneExit()
{
    mIsPresent = false;
    zNPCBase::SceneExit();
}
#else
void zNPCAnimViewer::Deactivate()
{
    mIsPresent = false;
    zNPCBase::Deactivate();
}
#endif

void zNPCAnimViewer::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCAnimViewer::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
