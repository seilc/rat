#ifndef ZUI_H
#define ZUI_H

#include "xBase.h"
#include "xColor.h"
#include "zUIAsset.h"
#include "zUICustom.h"
#include "zUIMotion.h"

struct zUIMotionAsset;

class zUI : public xBase
{
public:
    struct State
    {
        F32 x;
        F32 y;
        F32 width;
        F32 height;
        xColor color;
        U8 brightness;
        U8 pad[3];
    };

    const zUIAsset* asset;
    State current;
    State startMovement;
    F32 z;
    zUIMotionAsset* selectedMotion;
    zUIMotionAsset* unselectedMotion;
    zUICustom* custom;
    S32 padport;
    bool visible;
    bool focus;
    bool lastFocus;
    bool selected;
    bool brighten;
    bool hdrPass;
    bool locked;
    bool forcepreupdate;
    bool forcehdr;
    bool restoreFocus;

    zUIAsset* GetAsset() const { return (zUIAsset*)asset; }
    
    virtual U32 GetSortKey() const = 0;
    virtual bool Blends() const;
    virtual bool IsUIText() { return false; }
    virtual bool IsForcePreUpdate() { return forcepreupdate; }
    virtual bool IsForceHDR() { return forcehdr; }
    virtual bool IsRestoreFocus() { return restoreFocus; }
    virtual void DoInit();
    virtual void DoSetup();
    virtual void DoReset();
    virtual void DoResetMotion();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void DoRender() const = 0;
    virtual void DoUpdate(F32 dt);
    virtual void DoExit();
    virtual void DoInitMotion();
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);
    void Signal(xBase* from, U32 event);
    void AutoMenu(U32 zUIAsset::*item, U32 event);
    virtual void FocusOn();
    virtual void FocusOff();
    virtual void Select();
    virtual void Unselect();
    virtual void Visible();
    virtual void Invisible();
#if DEBUG || RELEASE
    virtual void DoAddTweaks(const char* baseName);
#endif

    static bool bAllowInput;
    static bool bAllowSelectInput;

private:
    zUIMotionAsset* motion;
    F32 motionTime;
    bool motionFiredEvent;
    bool motionLoop;
};

S32 zUIGetPadPortInitiatedPause();
void zUI_Init(zUI* text, zUIAsset* asset);

#endif
