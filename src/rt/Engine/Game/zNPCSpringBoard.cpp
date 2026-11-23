#include "xCommon.h"
#include "zNPCSpringBoard.h"

#include "zComponentPool.h"
#include "zCompLogicSpringBoard.h"
#include "zCompSteeringBasic.h"
#include "zICompNPCEntity.h"

static zICompNPCEntityParams gSpringBoardEntParameters;

bool zNPCSpringBoard::Activate()
{
    gSpringBoardEntParameters.useBoundingBox = true;

    npcLogicComp = zComponentPool<zCompLogicSpringBoard, zICompLogic, zNPCBase>::AttachComponent(this, NULL);
    npcEntityComp = zComponentPool<zICompNPCEntity, zICompNPCEntity, zNPCBase>::AttachComponent(this, &gSpringBoardEntParameters);
    npcSteeringComp = zComponentPool<zCompSteeringBasic, zICompSteering, zNPCBase>::AttachComponent(this, NULL);

    if (!npcLogicComp || !npcEntityComp || !npcSteeringComp) {
        Deactivate();
        return false;
    }

    return true;
}

#if !RELEASE
void zNPCSpringBoard::Deactivate()
{
    DetachAllComponents();
}
#endif

void zNPCSpringBoard::Update(xScene* xscn, F32 dt)
{
    PreUpdateAllComponents(xscn, dt);

    zICompLogic* logicComponent = GetLogicComponent();

    if (logicComponent) {
        logicComponent->Update(xscn, dt);
    }

    PostUpdateAllComponents(xscn, dt);
}

void zNPCSpringBoard::Render()
{
    zICompNPCEntity* compNPCEntity = GetEntityComponent();
    if (compNPCEntity) {
        compNPCEntity->Render();
    }

    DebugRenderAllComponents();
}
