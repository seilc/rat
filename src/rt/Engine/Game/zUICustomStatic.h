#ifndef ZUICUSTOMSTATIC_H
#define ZUICUSTOMSTATIC_H

#include "zUICustom.h"

class zUICustomStatic : public zUICustom
{
public:
    zUICustomStatic(zUI* ui);

    virtual void Reset();
    virtual void PreUpdate(F32 dt);
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif
    virtual void PreApplyMotionFrame(zUIMotionFrame* frame);

private:
    enum Stage
    {
        Stage_Off,
        Stage_FadeIn,
        Stage_On,
        Stage_FadeOut
    };

    Stage stage;
    F32 timer;
};

#endif
