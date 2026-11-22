#ifndef ZUICUSTOMPAUSESCREENMANAGER_H
#define ZUICUSTOMPAUSESCREENMANAGER_H

#include "zUICustom.h"

class zUICustomPauseScreenManager : public zUICustom
{
public:
    zUICustomPauseScreenManager(zUI* ui);

    virtual void Init();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
};

#endif
