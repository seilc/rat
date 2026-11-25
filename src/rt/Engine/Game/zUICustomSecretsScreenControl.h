#ifndef ZUICUSTOMSECRETSSCREENCONTROL_H
#define ZUICUSTOMSECRETSSCREENCONTROL_H

#include "zUICustom.h"

class zUICustomSecretsScreenControl : public zUICustom
{
public:
    zUICustomSecretsScreenControl(zUI* ui);

    virtual void Init();
    virtual void Setup();
    virtual void PostUpdate(F32 dt);
    virtual bool PreRender();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void Reset();

private:
    void changeFocusX(S32 dir);
    void changeFocusY(S32 dir);

    xBase* input;
    U16 currFocus;
    S8 currRow;
    S8 currCol;
};

#endif
