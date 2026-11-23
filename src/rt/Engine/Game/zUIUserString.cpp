#include "xCommon.h"
#include "zUIUserString.h"

#include "containers.h"
#include "xFont.h"
#include "xPad.h"
#include "xSndMgr.h"
#include "xTextAsset.h"
#include "zCheat.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zUI.h"

#include <new>
#include <string.h>

void zUIUserString::DoInit()
{
    zUIText::DoInit();
    if (linkCount) {
        link = (const xLinkAsset*)(GetAsset() + 1);
    }

    textBuffer = (char*)zUIAllocStaticMemory(GetAsset()->hardMaxChars + 1);
    SetText(textBuffer);
}

void zUIUserString::DoReset()
{
    zUIText::DoReset();
    SetText(textBuffer);

    const char* initialText = GetText();

    maxLength = GetAsset()->softMaxChars;
    length = 0;
    textBuffer[0] = '\0';

    if (initialText) {
        strcpy(textBuffer, initialText);
        length = strlen(initialText);
        xASSERT_L(58, length <= maxLength);
    }
}

void zUIUserString::DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                  U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventSetText: {
        U32 len;
        const char* asset = xTextFindString(toParamWidgetID, &len);
        if (!asset) {
            textBuffer[0] = '\0';
            length = 0;
            zEntEvent(this, this, eEventUIStringEmpty);
        } else {
            const char* text = asset + 1;
            strcpy(textBuffer, text);
            length = len;
            xASSERT_L(81, length <= maxLength);
            if (length >= maxLength) {
                // BUG: should be eEventUIStringFull
                zEntEvent(this, this, eEventUIStringEmpty);
            }
        }
        break;
    }

    case eEventUIAddChar: {
        char ch = (S32)toParam[0];
        if (length < maxLength) {
            textBuffer[length++] = ch;
            textBuffer[length] = '\0';
            if (length == maxLength) {
                zEntEvent(this, this, eEventUIStringFull);
            }
        }
        break;
    }

    case eEventUIDelChar:
        if (length > 0) {
            textBuffer[--length] = '\0';
            if (length == 0) {
                zEntEvent(this, this, eEventUIStringEmpty);
            }
        }
        break;

    case eEventUISetMaxChars:
        maxLength = toParam[1];
        xASSERT_L(114, maxLength >= 0);
        if (maxLength > GetAsset()->hardMaxChars)
            maxLength = GetAsset()->hardMaxChars;
        if (length > maxLength) {
            length = maxLength;
            textBuffer[length] = '\0';
        }
        break;

    case eEventUISendStringAsCheat:
        if (stricmp(textBuffer, "YourNameInLights") == 0)
            zEntEvent(this, this, eEventUISignalCreditCode);
        else
            switch (zCheatProcess(textBuffer)) {
            case zCheatStatus_Ok:
                zEntEvent(this, this, eEventUICheatOK);
                break;

            case zCheatStatus_Bad:
                zEntEvent(this, this, eEventUICheatBad);
                break;

            case zCheatStatus_CreditCode:
                zEntEvent(this, this, eEventUISignalCreditCode);
                break;

            case zCheatStatus_HILogo:
                xSndMgrPauseSounds(xSndCat_MUSIC, true, true);
                zEntEvent(this, this, eEventUISignalHILogo);
                xSndMgrPauseSounds(xSndCat_MUSIC, false, true);
                break;
            }
        break;

    default:
        zUI::DoHandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
        break;
    }
}

U32 zUIUserString::GetSortKey() const
{
    return GetAsset()->id;
}

void zUIUserString_Init(xBase& data, xDynAsset& asset, size_t)
{
    zUIUserString_Init((zUIUserString*)&data, (zUIUserStringAsset*)&asset);
}

void zUIUserString_Init(zUIUserString* userString, zUIUserStringAsset* asset)
{
    new (userString) zUIUserString();
    zUI_Init(userString, asset);
}
