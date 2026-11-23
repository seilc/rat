#ifndef ZUICUSTOMSOUNDMODE_H
#define ZUICUSTOMSOUNDMODE_H

#include "zUICustom.h"

class zUIText;

class zUICustomSoundMode : public zUICustom
{
public:
    zUICustomSoundMode(zUI* ui);

    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

private:
    zUIText* optionMono;
    zUIText* optionStereo;
    zUIText* optionDolby;
};

extern zUICustomSoundMode* pUICustomSoundMode;

#endif
