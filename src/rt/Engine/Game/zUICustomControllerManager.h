#ifndef ZUICUSTOMCONTROLLERMANAGER_H
#define ZUICUSTOMCONTROLLERMANAGER_H

#include "zUICustom.h"

class zUIText;
class zUIImage;

class zUICustomControllerManager : public zUICustom
{
public:
    zUICustomControllerManager(zUI* ui);

    virtual void Init();
    void ResetMriOnLeft();
    void SwapPlayerPadPorts(); // UNUSED
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    void ShowSystemMessage(xBase* from, U32 strHash);

    S32 portPressedStart;
    zUIText* acceptText;
    zUIText* backText;
    zUIText* secondPlayerText;
    zUIText* characterSelectText;
    zUIText* infoText;
    zUIImage* infoImage;
};

extern zUICustomControllerManager* pUICustomControllerManager;

#endif
