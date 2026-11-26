#ifndef ZUICUSTOMSCROLLERS_H
#define ZUICUSTOMSCROLLERS_H

#include "zUICustom.h"
#include "xCounter.h"

class zUICustomNumberScroller : public zUICustom
{
public:
    zUICustomNumberScroller(zUI* ui);

    virtual void Reset();
    virtual void PreUpdate(F32 dt);
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif

private:
    char buffer[128];
    S32 current;
    F32 timeLeft;
};

class zUICustomDescriptionScroller : public zUICustom
{
public:
    zUICustomDescriptionScroller(zUI* ui);

    virtual void Reset();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void PreUpdate(F32 dt);
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif

private:
    xCounter* counter;
    const char* baseText;
    char buffer[384];
    F32 currentTime;
    bool running;
};

class zUICustomMapScroller : public zUICustom
{
public:
    zUICustomMapScroller(zUI* ui);

    virtual void Reset();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void PreUpdate(F32 dt);
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif
    virtual void PreApplyMotionFrame(zUIMotionFrame* frame);

    void SignalEffect();

private:
    xCounter* counter;
    F32 currentX;
    F32 currentY;
    F32 destX;
    F32 destY;
    F32 currentSpeed;
    bool lastRunning;
};

#endif
