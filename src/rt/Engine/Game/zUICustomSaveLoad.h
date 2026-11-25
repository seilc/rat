#ifndef ZUICUSTOMSAVELOAD_H
#define ZUICUSTOMSAVELOAD_H

#include "zUICustom.h"

class zUIText;

class zUICustomNewGameManager : public zUICustom
{
public:
    zUICustomNewGameManager(zUI* ui); // UNUSED

    virtual void Init(); // UNUSED
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID); // UNUSED

    zUIText* backText;
};

class zUICustomSaveLoadManager : public zUICustom
{
public:
    zUICustomSaveLoadManager(zUI* ui);

    virtual void Init();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

    zUIText* backText;
    zUIText* selectText;
};

extern zUICustomSaveLoadManager* pUICustomSaveLoadManager;

#endif
