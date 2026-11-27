#ifndef ZUICUSTOMCONTROLOVERLAY_H
#define ZUICUSTOMCONTROLOVERLAY_H

#include "zUICustom.h"

class zUICustomControlOverlay : public zUICustom
{
public:
    zUICustomControlOverlay(zUI* ui);

    virtual void Init();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void PreUpdate(F32 dt);
    virtual bool PreRender();

    void UpdatePlayerText();
    void RenderLines();

private:
    U32 controlCount;
    bool currPlayer;
};

#endif
