#ifndef ZUITEXT_H
#define ZUITEXT_H

#include "zUI.h"
#include "zUITextAsset.h"

class xtextbox;

class zUIText : public zUI
{
public:
    zUITextAsset* GetAsset() const { return (zUITextAsset*)zUI::GetAsset(); }

    virtual U32 GetSortKey() const { return GetAsset()->textID; }
    virtual bool IsUIText() { return true; }
    virtual void DoInit();
    virtual void DoReset();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void DoRender() const;
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);
#if DEBUG || RELEASE
    virtual void DoAddTweaks(const char* baseName);
#endif

    void setup_textbox(xtextbox& tb, const zUITextAsset* asset) const;
    void SetText(const char* text) { this->text = text; }
    void SetCached(bool value) { cached = value; }
    void SetClearLayoutCache(bool value) { clearLayoutCache = value; }
    const char* GetText() const { return text; }

private:
    xColor shadowColor;
    mutable const char* text;
    mutable bool cached;
    mutable bool clearLayoutCache;
};

void zUIText_Init(xBase& data, xDynAsset& asset, size_t);
void zUIText_Init(zUIText* text, zUITextAsset* asset);
void zUITextInitSystem();

#endif
