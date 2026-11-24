#ifndef ZUIBOX_H
#define ZUIBOX_H

#include "zUI.h"
#include "xTextureManager.h"

class zUIBox : public zUI
{
public:
    virtual void DoInit();
    virtual void DoReset();
    virtual void DoResetMotion();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);
    virtual void DoRender() const;
#if DEBUG || RELEASE
    virtual void DoAddTweaks(const char* baseName);
#endif

    void RenderPart(S32 p, F32 x1, F32 y1, F32 x2, F32 y2, F32 uScale, F32 vScale, F32 rotation, F32 xPivot, F32 yPivot) const;
    void RefreshTextures();

private:
    xTextureHandle textures[9];
    F32 curRotation;
};

void zUIBox_Init(xBase& data, xDynAsset& asset, size_t);

#endif
