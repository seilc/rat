#ifndef ZHEISTMANAGER_H
#define ZHEISTMANAGER_H

#include "types.h"

class zHeistInteractionObjectManager;
class zHeistLookoutPoint;
class zHeistNPCInterfaceManager;
class zPad;

class zHeistManager
{
public:
    static void Create();
    void Reset(); // UNUSED
    static void Exit();

    void ShutDown();
    void Init();
    void RenderOverlays();
    void HandleInput(zPad* currPad, F32 dt, zHeistLookoutPoint* lookoutPoint);
    void Update(F32 dt);
    void BeginInteraction(zHeistLookoutPoint* lookoutPoint);
    void EndInteraction();

protected:
    static zHeistManager* heistManager;

    zHeistNPCInterfaceManager* heistNPCInterfaceManager;
    zHeistInteractionObjectManager* heistInteractionObjectManager;
    F32 inputTimer;
};

#endif
