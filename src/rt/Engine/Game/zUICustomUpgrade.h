#ifndef ZUICUSTOMUPGRADE_H
#define ZUICUSTOMUPGRADE_H

#include "zUICustom.h"

struct zPlayer;
class zUIText;

class zUICustomUpgrade : public zUICustom
{
public:
    static void StaticSetup(); // UNUSED

    zUICustomUpgrade(zUI* ui);

    virtual void Init();
    virtual void Reset();
    virtual void Setup();
    virtual void Exit();
    void SwapSides(S16 active, S16 inactive);
    void SelectActive();
    void DeselectActive();
    void RefreshAll(); // UNUSED
    virtual void PreUpdate(F32 dt);
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

    char temp[128];

private:
    S32 numItems;
    S32 activeItem;
    bool firstTime;
    S32 columnNum;
    S32 brainID;
    zPlayer* pPlayer;
    zUIText** upgrades;
    zUIText** counts;
    xBase* soundFXLeft;
    xBase* soundFXRight;
    xBase* soundFXUp;
    xBase* soundFXDown;
    xBase* soundFXDeny;
    xBase* soundFXConfirm;
};

extern zUICustomUpgrade* pUICustomUpgradeManager;

void zSystemMessageUpgradeCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID); // UNUSED

#endif
