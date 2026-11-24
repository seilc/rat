#ifndef ZLOADINGSCREEN_H
#define ZLOADINGSCREEN_H

#include "zAsync.h"

struct xBase;
class zUI;

class zLoadingScreen : public zAsync::Screen
{
public:
    virtual void Init();
    virtual void Update(F32 dt);
    virtual void Render();
    virtual void Exit();
    void* CustomAllocate(U32 size);
    void CustomSignal(zUI* from, U32 dest, U32 event, const F32* param, xBase* paramWidget, U32 paramWidgetID);

    static void UnSetShowStats();

private:
    static const U32 MAX_CONTROLS = 16;
    static const U32 MAX_ALLOCATION = 32;

    zUI* controls[MAX_CONTROLS];
    S32 controlCount;
    void* allocations[MAX_ALLOCATION];
    S32 allocationCount;
};

#endif
