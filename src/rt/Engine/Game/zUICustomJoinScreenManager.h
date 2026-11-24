#ifndef ZUICUSTOMJOINSCREENMANAGER_H
#define ZUICUSTOMJOINSCREENMANAGER_H

#include "zUICustom.h"

class zUIText;

class zUICustomJoinScreenManager : public zUICustom
{
public:
    zUICustomJoinScreenManager(zUI* ui);

    void SetDropJoinText(S32 trccount);
    void SetLockPauseText(bool value);
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void Init();

    zUIText* textJoinDrop;
    zUIText* textReturn;
    zUIText* textPause;
};

extern zUICustom* pUICustomDropJoinSceneManager;

#endif
