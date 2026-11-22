#ifndef ZUICUSTOMSCENEIMAGE_H
#define ZUICUSTOMSCENEIMAGE_H

#include "zUICustom.h"
#include "xCounter.h"

class zUICustomSceneImage : public zUICustom
{
public:
    zUICustomSceneImage(zUI* ui);

    virtual void Reset();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

    void UpdateTexture();

private:
    xCounter* counter;
};

extern char sceneBaseImage[32];

#endif
