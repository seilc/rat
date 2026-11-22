#include "xCommon.h"
#include "zFMV.h"

#include "containers.h"
#include "xFont.h"
#include "xPad.h"
#include "xRegionSupport.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "xSubTitles.h"
#include "xutil.h"
#include "iFMV.h"
#include "zGlobals.h"
#include "zMenu.h"
#include "zScene.h"

#include <string.h>

const zFMVFile* zFMVFileGetFile(eFMVFile fmvId)
{
    for (S32 i = 0; i < (S32)ARRAY_LENGTH(zFMVFileTable); i++) {
        if (zFMVFileTable[i].fmvCode == fmvId)
            return &zFMVFileTable[i];
    }
    xFAIL_FMT_L(23, "No FMV found, code %d", fmvId);
    return NULL;
}

const char* zFMVFileGetName(eFMVFile fmvId)
{
    const zFMVFile* file = zFMVFileGetFile(fmvId);
    return file == NULL ? NULL : file->fileName;
}

U32 zFMVPlay(const char* name, U32 buttons, F32 time, U32 uSubtitlesAID, bool skippable, bool lockController,
             bool displayDemoBlink)
{
    U32 retval = 0;
    if (!globals.noMovies) {
        zMenuPause(true);

        F32 oldSize = xSubTitlesGetSize();
        xSubTitlesSetSize(0.18f);

        xSndEffect currentEffect = xSndMgrGetEffect();
        xSndMgrSetEffect(xSndEffect_NONE);

        if (strcmp(name, zFMVFileGetName(eFMVFile_IntroText)) == 0 ||
            strcmp(name, zFMVFileGetName(eFMVFile_AttractMode1)) == 0) {
            char localizedName[64];
            strcpy(localizedName, name);
            strcat(localizedName, "_");
            strcat(localizedName, xRegionCode(xSTGetLocalizationEnum()));
            retval = iFMVPlay(localizedName, buttons, time, uSubtitlesAID, skippable, lockController,
                              displayDemoBlink);
        } else {
            retval = iFMVPlay(name, buttons, time, uSubtitlesAID, skippable, lockController, displayDemoBlink);
        }

        xSndMgrSetEffect(currentEffect);

        xSubTitlesSetSize(oldSize);

        for (S32 i = 0; i < k_XPAD_MAX; i++)
            xPadUpdate(i, 0.0f);

        zMenuPause(false);

        globals.fmvJustFinished = true;
    }
    return retval;
}
