#include "xCommon.h"
#include "zHeistManager.h"

#include "xFile.h"
#include "xFont.h"
#include "zFX.h"
#include "zGlobals.h"
#include "zHeistInteractionObject.h"
#include "zHeistLookoutPoint.h"
#include "zHeistNPCInterface.h"
#include "zPad.h"

zHeistManager* zHeistManager::heistManager;

void zHeistManager::Create()
{
    heistManager = (zHeistManager*)xMEMALLOC_L(sizeof(zHeistManager), 0, eMemMgrTag_Heists, 0, 27);
    heistManager->Init();
}

void zHeistManager::Exit()
{
    if (heistManager != NULL) {
        heistManager->ShutDown();
        heistManager = NULL;
    }
}

void zHeistManager::ShutDown()
{
    heistNPCInterfaceManager->Exit();
    heistInteractionObjectManager->Exit();

    heistNPCInterfaceManager = NULL;
    heistInteractionObjectManager = NULL;

    zHeistLookoutPoint::SetHeistManager(NULL);
}

void zHeistManager::Init()
{
    heistNPCInterfaceManager = (zHeistNPCInterfaceManager*)xMEMALLOC_L(sizeof(zHeistNPCInterfaceManager), 0, eMemMgrTag_Heists, 0, 59);
    heistInteractionObjectManager = (zHeistInteractionObjectManager*)xMEMALLOC_L(sizeof(zHeistInteractionObjectManager), 0, eMemMgrTag_Heists, 0, 60);

    heistNPCInterfaceManager->Init();
    heistInteractionObjectManager->Init();

    zHeistLookoutPoint::SetHeistManager(this);

    inputTimer = 0.0f;
}

void zHeistManager::RenderOverlays()
{
    heistInteractionObjectManager->RenderOverlays();
    heistNPCInterfaceManager->RenderOverlays();
}

void zHeistManager::HandleInput(zPad* currPad, F32 dt, zHeistLookoutPoint* lookoutPoint)
{
    inputTimer += dt;
    if (inputTimer > 0.2f) {
        if (currPad->On(0x10)) {
            inputTimer = 0.0f;
            heistNPCInterfaceManager->SelectNextGangMember(lookoutPoint);
        }

        if (currPad->On(0x40)) {
            inputTimer = 0.0f;
            heistNPCInterfaceManager->SelectPrevGangMember(lookoutPoint);
        }

        if (currPad->On(0x20)) {
            inputTimer = 0.0f;
            heistNPCInterfaceManager->SelectNextPath();
        }

        if (currPad->On(0x80)) {
            inputTimer = 0.0f;
            heistNPCInterfaceManager->SelectPrevPath();
        }

        if (currPad->On(0x10000)) {
            inputTimer = 0.0f;
            heistNPCInterfaceManager->ExecuteCommand();
        }
    }

    heistNPCInterfaceManager->CheckSelected(lookoutPoint);
}

void zHeistManager::Update(F32 dt)
{
    heistNPCInterfaceManager->Update(dt);
    heistInteractionObjectManager->Update(dt);
}

void zHeistManager::BeginInteraction(zHeistLookoutPoint* lookoutPoint)
{
    heistNPCInterfaceManager->ResetSelection(lookoutPoint);
}

void zHeistManager::EndInteraction()
{
}
