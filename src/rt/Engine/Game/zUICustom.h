#ifndef ZUICUSTOM_H
#define ZUICUSTOM_H

#include "types.h"

class zUI;
struct xBase;
struct zUIMotionFrame;

class zUICustom
{
public:
    zUICustom(zUI* ui) : ui(ui) {}

    virtual void Init();
    virtual void Setup();
    virtual void Reset();
    virtual void ResetMotion();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void PreUpdate(F32 dt);
    virtual void PostUpdate(F32 dt);
    virtual bool PreRender();
    virtual void PostRender();
    virtual void Exit();
    virtual bool PreAutoMenu();
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif
    virtual void PreInitMotion();
    virtual void PostInitMotion();
    virtual void PreApplyMotionFrame(zUIMotionFrame* frame);
    virtual void PostApplyMotionFrame(zUIMotionFrame* frame);
    virtual bool IsSystem(zUI*) { return false; }
    virtual bool IsSignalAllowed(U32) { return true; }
    virtual bool IsForcePreUpdate() { return false; }

    zUI* GetUI() const { return ui; }

protected:
    zUI* ui;
};

#endif
