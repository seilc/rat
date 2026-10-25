#include "xFMV.h"

#include "xSndMgr.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zScene.h"
#include "xutil.h"
#include "zMenu.h"
#include "xDebug.h"

#include "decomp.h"

#include <bink.h>

static void DefaultFMVPauseSoundCB(bool doPause);

static F32 sFMVVolumeScale = 1.0f;
static xFMVPauseSoundCallback sxFMVPauseSoundCB = DefaultFMVPauseSoundCB;

void xFMVPauseSound(bool doPause)
{
    sxFMVPauseSoundCB(doPause);

    if (doPause) {
        xSndMgrUpdate(0.0f);
    }
}

static void DefaultFMVPauseSoundCB(bool doPause)
{
    if (zGameIsPaused()) {
        xSndMgrPauseSounds(6, doPause, true);

        if (globals.sceneCur && globals.sceneCur->sceneID == IDTAG('M','N','U','S')) {
            if (zMenuRunning()) {
                xSndMgrPauseSounds(-3, doPause, true);
            }
        }

        if (doPause) {
            xSndMgrStopSounds(2, true, false);
        }
    } else {
        xSndMgrPauseSounds(-3, doPause, true);
    }
}

// RESTORED: from TSSM GC
xFMVPauseSoundCallback xFMVSetPauseSoundCB(xFMVPauseSoundCallback pauseSoundCB)
{
    xFMVPauseSoundCallback curCB = sxFMVPauseSoundCB;

    if (!pauseSoundCB) {
        pauseSoundCB = DefaultFMVPauseSoundCB;
    }

    sxFMVPauseSoundCB = pauseSoundCB;

    return curCB;
}
// END RESTORED

S32 xFMVGetBinkCompliantVolume()
{
    S32 volume = (S32)(32768.0f * xSndMixerGetOutputVolume(5) * sFMVVolumeScale);
    return volume;
}

RwTexture* xFMVGetTexture(MOVIE* pMovie, U16 looping, U16 freezeframed, U32 startframe, U32 endframe, U32)
{
    if (!pMovie) {
        iprintf("NULL MOVIE ERROR ENCOUNTERED\n");
        return NULL;
    }

    if (!pMovie->hBink) {
        return NULL;
    }

    if (endframe == 0) {
        endframe = pMovie->hBink->Frames;
    }

    if (pMovie->hBink->LastFrameNum == 0xFFFFFFFF) {
        BinkGoto(pMovie->hBink, startframe + 1, 0);
    }

#if DEBUG || RELEASE
    if (xDebugModeGet() == 18) {
        S32 frameSkipped;
        DECOMP_FORCEREGISTER(frameSkipped);
        xprintf("Time Code:       %02d:%02d:%02d:%02d\nFrame %d/%d\nSkipped %d\n",
                pMovie->hBink->FrameNum / (30 * 60 * 40),
                pMovie->hBink->FrameNum / (30 * 60) % 60,
                pMovie->hBink->FrameNum / 30 % 60,
                pMovie->hBink->FrameNum % 30,
                pMovie->hBink->FrameNum,
                pMovie->hBink->Frames,
                pMovie->hBink->skippedblits);
    }
#endif

    RwRaster* pBinkRaster = RwTextureGetRaster(pMovie->pBinkTexture);

    if (!BinkWait(pMovie->hBink)) {
        BinkDoFrame(pMovie->hBink);

        RwUInt8* pBits = RwRasterLock(pBinkRaster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);

        RwInt32 format = RwRasterGetFormat(pBinkRaster);
        RwInt32 pixformat = format & rwRASTERFORMATPIXELFORMATMASK;

        xASSERT_L(194, pBits);

        RwInt32 nDestPitch = RwRasterGetStride(pBinkRaster);

        S32 bFailed = BinkCopyToBuffer(pMovie->hBink, pBits, nDestPitch, RwRasterGetHeight(pBinkRaster), 0, 0, BINKCOPYALL | BINKSURFACE32);
        xASSERT_L(197, !bFailed);
        if (bFailed) {
            return NULL;
        }

        RwRaster* pNullOnErr = RwRasterUnlock(pBinkRaster);
        xASSERT_L(201, pNullOnErr);

        if (pMovie->hBink->Paused != freezeframed) {
            BinkPause(pMovie->hBink, freezeframed);
        }

        if (looping) {
            if (endframe == pMovie->hBink->LastFrameNum) {
                if (startframe != 0) {
                    BinkGoto(pMovie->hBink, startframe, 0);
                }
            }
            BinkNextFrame(pMovie->hBink);
            BinkWait(pMovie->hBink);
        }
        else if (endframe != pMovie->hBink->LastFrameNum) {
            BinkNextFrame(pMovie->hBink);
            BinkWait(pMovie->hBink);
        }
    }

    return pMovie->pBinkTexture;
}
