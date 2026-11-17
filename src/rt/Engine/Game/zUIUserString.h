#ifndef ZUIUSERSTRING_H
#define ZUIUSERSTRING_H

#include "zUIText.h"
#include "zUIUserStringAsset.h"

class zUIUserString : public zUIText
{
public:
    zUIUserStringAsset* GetAsset() const { return (zUIUserStringAsset*)zUI::GetAsset(); }

    virtual U32 GetSortKey() const;
    virtual void DoInit();
    virtual void DoReset();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

private:
    char* textBuffer;
    S32 length;
    S32 maxLength;
};

#endif
