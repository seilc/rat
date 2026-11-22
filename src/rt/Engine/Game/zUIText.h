#ifndef ZUITEXT_H
#define ZUITEXT_H

#include "zUI.h"
#include "zUITextAsset.h"

class xtextbox;

class zUIText : public zUI
{
public:
    virtual U32 GetSortKey() const;
    virtual bool IsUIText();
    virtual void DoInit();
    virtual void DoReset();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void DoRender() const;
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);
#if DEBUG || RELEASE
    virtual void DoAddTweaks(const char* baseName);
#endif

    void setup_textbox(xtextbox& tb, const zUITextAsset* asset) const;
    void SetText(const char* text)
    {
        this->text = text;
    }

private:
    xColor shadowColor;
    const char* text;
    U8 cached;
    U8 clearLayoutCache;
};

#endif
