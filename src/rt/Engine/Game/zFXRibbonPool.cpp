#include "xCommon.h"
#include "zFXRibbonPool.h"

#include "xFX.h"
#include "xModel.h"

#include <float.h>
#include <string.h>

namespace
{
    U32 ribbonCount;
    xFXRibbon* ribbonPool;
    bool* ribbonInUse;
} // namespace

void zFXRibbonPoolInit(U32 maxNumRibbons)
{
    ribbonCount = maxNumRibbons;
    if (ribbonCount == 0) {
        return;
    }

    ribbonPool = (xFXRibbon*)xMEMALLOC_L(ribbonCount * sizeof(xFXRibbon), 0, eMemMgrTag_FXRibbon, 0, 39);
    ribbonInUse = (bool*)xMEMALLOC_L(ribbonCount, 0, eMemMgrTag_FXRibbon, 0, 40);
    for (U32 i = 0; i < ribbonCount; i++) {
        ribbonPool[i].create(FALSE);

        xFXRibbon::config_type* cfg = new (eMemStaticType_0, eMemMgrTag_FXRibbon) xFXRibbon::config_type;
        ribbonPool[i].load_default_config(*cfg);
        ribbonPool[i].set_config(cfg);
    }
}

xFXRibbon* zFXRibbonPoolNew()
{
    U32 index = (U32)-1;
    for (U32 i = 0; i < ribbonCount; i++) {
        if (ribbonPool[i].size() == 0 && !ribbonInUse[i]) {
            index = i;
            break;
        }
    }

    if (index == (U32)-1) {
        F32 remainingTime = FLT_MAX;
        for (U32 i = 0; i < ribbonCount; i++) {
            if (!ribbonInUse[i]) {
                F32 remain = ribbonPool[i].time_remaining();
                if (remain < remainingTime) {
                    remainingTime = remain;
                    index = i;
                }
            }
        }
    }

    if (index != (U32)-1) {
        ribbonPool[index].clear();
        ribbonInUse[index] = true;
        return &ribbonPool[index];
    } else {
        xFAIL_MSG_L(76, "All streaks are in use, increase size, or disable this assert (program can still run fine if you skip this)");
        return NULL;
    }
}

void zFXRibbonPoolDelete(xFXRibbon*& ribbon)
{
    if (!ribbon)
        return;
    S32 index = ribbon - ribbonPool;
    xASSERT_FMT_L(86, index >= 0 && index < (S32)ribbonCount, "Trying to delete something that is not in the ribbon pool");
    ribbonInUse[index] = false;
    ribbon = NULL;
}

void zFXRibbonPool_SceneReset()
{
    if (ribbonCount < 1)
        return;
    memset(ribbonInUse, 0, ribbonCount);
}
