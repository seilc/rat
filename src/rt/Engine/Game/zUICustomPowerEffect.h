#ifndef ZUICUSTOMPOWEREFFECT_H
#define ZUICUSTOMPOWEREFFECT_H

#include "zUICustom.h"

class zUICustomPowerEffect : public zUICustom
{
public:
    zUICustomPowerEffect(zUI* ui);

    virtual void Reset();
    virtual void PreUpdate(F32 dt);
    virtual bool PreRender();
    virtual void PostRender();
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif
    virtual void PreApplyMotionFrame(zUIMotionFrame* frame);

private:
    F32 scrollTime;
    F32 scrollTotal;
    F32 waitTime;
};

#endif
