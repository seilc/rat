#include "xCommon.h"
#include "zUICustomSoundMode.h"

#include "containers.h"
#include "xEvent.h"
#include "xFont.h"
#include "xPad.h"
#include "xSnd.h"
#include "xString.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIText.h"

zUICustomSoundMode* pUICustomSoundMode;

zUICustomSoundMode::zUICustomSoundMode(zUI* ui) : zUICustom(ui)
{
    xASSERT_L(16, ui->asset->type == xStrHash("ui:text"));
    optionMono = NULL;
    optionStereo = NULL;
    optionDolby = NULL;
}

bool zUICustomSoundMode::HandleEvent(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventVisible:
        optionMono = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS-SOUND VALUE MONO"));
        optionStereo = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS-SOUND VALUE STEREO"));
        optionDolby = (zUIText*)zSceneFindObject(xStrHash("UI OPTIONS-SOUND VALUE DOLBY"));

        xASSERT_L(32, optionMono);
        xASSERT_L(33, optionStereo);
        xASSERT_L(34, optionDolby);

        switch (gSnd.sndMode) {
        case xSndMode_Mono:
            zEntEvent(optionMono, eEventUIFocusOn_Select);
            break;

        case xSndMode_Stereo:
            zEntEvent(optionStereo, eEventUIFocusOn_Select);
            break;

        case xSndMode_Dolby:
            zEntEvent(optionDolby, eEventUIFocusOn_Select);
            break;
        }
        break;

    case eEventIncrement:
        if (globals.ForceMono) {
            ui->Signal(eEventIncrementFailed);
            return false;
        }

        if (gSnd.sndMode == xSndMode_Mono) {
            S32 oldMode = gSnd.sndMode, newMode = gSnd.sndMode + 1;
            if (newMode >= xSndMode_Count)
                newMode = xSndMode_Count - 1;
            if (newMode != gSnd.sndMode)
                xSndSetMode((xSndMode)newMode);
            ui->Signal(oldMode != newMode ? eEventIncrementSuccess : eEventIncrementFailed);
        } else {
            ui->Signal(eEventIncrementFailed);
        }
        break;

    case eEventDecrement:
        if (globals.ForceMono) {
            ui->Signal(eEventDecrementFailed);
            return false;
        }

        if (gSnd.sndMode == xSndMode_Stereo) {
            S32 oldMode = gSnd.sndMode, newMode = gSnd.sndMode - 1;
            if (newMode < 0)
                newMode = 0;
            if (newMode != gSnd.sndMode)
                xSndSetMode((xSndMode)newMode);
            ui->Signal(oldMode != newMode ? eEventDecrementSuccess : eEventDecrementFailed);
        } else {
            ui->Signal(eEventDecrementFailed);
        }
        break;
    }

    return true;
}
