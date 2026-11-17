#ifndef ZUICUSTOMCHEATSMANAGER_H
#define ZUICUSTOMCHEATSMANAGER_H

#include "zUICustom.h"

class zUICustomCheatsManager : public zUICustom
{
public:
    zUICustomCheatsManager(zUI* ui);

    virtual void Init();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
};

extern zUICustomCheatsManager* pUICustomCheatsManager;

#endif
