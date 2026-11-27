#ifndef ZUICUSTOMOPTIONSMANAGER_H
#define ZUICUSTOMOPTIONSMANAGER_H

#include "zUICustom.h"

class zUIBox;
class zUIText;

class zUICustomOptionsManager : public zUICustom
{
public:
    zUICustomOptionsManager(zUI* ui);

    virtual void Init();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void PreUpdate(F32 dt);

    void AdjustScreenOnOff();

    F32 musicVolume;
    F32 sfxVolume;
    S32 soundMode;
    bool vibrationOnP1;
    bool vibrationOnP2;

protected:
    bool doupdate;
    zUIText* optionVibP1Text;
    zUIText* optionVibP2Text;
    zUIBox* optionVibP1Box;
    zUIBox* optionVibP2Box;
};

extern bool inOverlayScreen;
extern F32 inOverlayScreenExitTimer;

bool zUICustomOptionsScreenIsInSystemMessage();
void zUICustomOptionsScreenAdjust(bool value);

#endif
