#ifndef ZUICUSTOMSYSMESSAGE_H
#define ZUICUSTOMSYSMESSAGE_H

#include "zUICustom.h"
#include "xBase.h"
#include "zUIText.h"
#include "zUIImage.h"

class zUICustomSysMessageManager : public zUICustom
{
public:
    zUICustomSysMessageManager(zUI* ui);

    virtual void Init();
    virtual bool IsSignalAllowed(U32 toEvent);
    void SetInvisible(zUI* uiElement);
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void PreUpdate(F32 dt);
    void ShowSystemMessage(xBaseEventCallback cb, U32, U32 _toCBEvent, const F32* _toCBParams, xBase* _toCBto,
                           xBase* _toCBParamWidget, U32 _toCBParamWidgetID, const char* sys_message);

    xBaseEventCallback cb_function;
    U32 toCBEvent;
    F32 toCBParams[4];
    xBase* toCBto;
    xBase* toCBParamWidget;
    U32 toCBParamWidgetID;
    U8 enablePrevScreenHandler;
    U8 infoNoPressTriangle;
    zUI* fromUI;
    zUIText* itemOk;
    zUIText* itemYes;
    zUIText* itemNo;
    zUIText* itemMsg;
    zUIText* itemOkExtra;
    zUIText* itemYesExtra;
    zUIText* itemNoExtra;
    zUIImage* panelLowLeft;
    zUIImage* panelLowRight;
    zUIImage* panelUpLeft;
    zUIImage* panelUpRight;
    zUI* invisibleUI[4];
    S32 invisibleUICount;

private:
    S32 padPortInitiatedSystemMessage;
    S32 lastPadPortInitiatedSystemMessage;
    F32 infoTimer;
};

extern zUI* pSysMessageManager;
extern zUICustom* pSysMessageManager_Custom;

#endif
