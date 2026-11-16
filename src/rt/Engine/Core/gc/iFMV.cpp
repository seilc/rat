#include "xCommon.h"
#include "iFMV.h"

#include "xCam.h"
#include "xColor.h"
#include "xFile.h"
#include "xFMV.h"
#include "xGlobals.h"
#include "xPad.h"
#include "xstransvc.h"
#include "xSubTitles.h"
#include "xTRC.h"
#include "iARAMTmp.h"
#include "iCamera.h"
#include "iDebug.h"
#include "iFile.h"
#include "iPad.h"
#include "iSystem.h"
#include "iTime.h"
#include "iTRC.h"
#include "zGame.h"
#include "zGameState.h"
#include "zMain.h"

#include <bink.h>
#include <rwcore.h>
#include <dolphin.h>
#include <string.h>

static U32 iFMVPlay(const char* filename, U32 buttons, F32, U32 uSubtitlesAID, bool skippable, bool, bool displayDemoBlink, S32 uAudioTrack);

inline S32 NextPow2(S32 in)
{
    S32 ret = 1;
    while (ret < in)
        ret <<= 1;
    return ret;
}

BINK* hBinkCurrMovie;
static bool sARAMtmp;
static U32 amt;

static void* BigMovieAlloc(U32 uNumBytes)
{
    void* addr;

    if (sARAMtmp) {
        if (uNumBytes <= amt) {
            addr = iARAMTmpMalloc('ANIM', uNumBytes);
            if ((U32)addr == 0xBEEEEEEF)
                addr = RwMallocL(uNumBytes, rwMEMHINTDUR_NADURATION, 85);
            else
                amt -= uNumBytes;
        } else
            addr = RwMallocL(uNumBytes, rwMEMHINTDUR_NADURATION, 93);
    } else {
        addr = RwMallocL(uNumBytes, rwMEMHINTDUR_NADURATION, 95);
    }

    return addr;
}

static void BigMovieFree(void* pMemToFree)
{
    if (iARAMTmpInARAMTemp('ANIM', pMemToFree)) {
        iARAMTmpFree('ANIM', pMemToFree);
    } else {
        RwFree(pMemToFree);
    }
}

static void* LilMovieAlloc(U32 uNumBytes); // UNUSED
static void LilMovieFree(void* pMemToFree); // UNUSED

bool iFMVSystemInit()
{
    iprintf("Bink RGB Converter loaded\n");
    return true;
}

static BINK* iFMVOpen(const char* fullFile, U32 uBinkFlags)
{
    RADARAMCALLBACKS OurGCCallbacks;
    OurGCCallbacks.aram_malloc = iSystemARAMAlloc;
    OurGCCallbacks.aram_free = iSystemARAMFree;
    BinkSoundUseAX(&OurGCCallbacks);

    iTRCDisk::SetMovieResetCallBack(iFMVResetCallback);
    iTRCDisk::EnableMovieResetCallBack(true);

    BinkSetMemory(BigMovieAlloc, BigMovieFree);

    BINK* hBinkMovie = NULL;
    if (!iFileExists(fullFile, 0)) {
        return hBinkMovie;
    }

    S32 numRetries = 0;
    do {
        if (iTRCDisk::IsDiskIDed()) {
            xASSERT_L(167, !hBinkMovie);

            if ((gGameMode == eGameMode_Game || gGameMode == eGameMode_Pause) && !sARAMtmp) {
                sARAMtmp = true;
                amt = iARAMTmpBegin('ANIM', 0x280000, true);
            }

            hBinkMovie = BinkOpen(fullFile, uBinkFlags);
            if (!hBinkMovie) {
                iprintf("BinkOpen failed:  %s\n", BinkGetError());
            }
        }

        if (iTRC_CHECK_L(188)) {
            iprintf("iFMVOpen: Disc error %s! Retrying (%d)...\n", fullFile, numRetries + 1);
            numRetries++;
            if (hBinkMovie) {
                iprintf("iFMVOpen: Movie %s was opened before disc error. Attempting to re-open again...\n", fullFile);
                BinkClose(hBinkMovie);
                hBinkMovie = NULL;
            }
            continue;
        }

        if (!hBinkMovie) {
            iprintf("iFMVOpen: BinkOpen failed while opening %s! Retrying (%d)...\n", fullFile, numRetries + 1);
            numRetries++;
            xFile tfile;
            iFileOpen(fullFile, 0, &tfile);
            DVDSeekAsyncPrio(&tfile.ps.file, 0, NULL, 2);
            iTRC_CHECK_L(211);
            DVDCancel(&tfile.ps.file.cb);
            iFileClose(&tfile);
        }
    } while (!hBinkMovie && numRetries < 100);

    if (numRetries > 0) {
        OSReport("numRetries = %d\n", numRetries);
    }

    xRegion reg = xSTGetLocalizationEnum();
    // BUG: always true, || instead of &&
    if (reg > eRegionUS && reg < eRegionUK && (reg != eRegionJP || reg != eRegionKR) && hBinkMovie->Frames == 0x511) {
        if (strstr(fullFile, "FMVOM01.")) {
            hBinkMovie->Frames = 0x50E;
        }
    }

    if (strstr(fullFile, "PROMO2.") && reg == eRegionJP) {
        BinkGoto(hBinkMovie, 60, 0);
    }

    return hBinkMovie;
}

static void iFMVClose(BINK* hMovie)
{
    BinkClose(hMovie);

    iTRCDisk::SetMovieResetCallBack(NULL);
    iTRCDisk::EnableMovieResetCallBack(false);

    if (sARAMtmp) {
        sARAMtmp = false;
        iARAMTmpEnd('ANIM', true);
    }
}

static MOVIE* movie_array;
static S32 movie_asset_count;

void iFMVScenePostInit()
{
    xASSERT_L(268, movie_array==NULL);

    movie_asset_count = xSTAssetCountByType('BINK');
    movie_array = (MOVIE*)xMEMALLOC_L(movie_asset_count * sizeof(MOVIE), 0, eMemMgrTag_FMV, 0, 271);
    xASSERT_L(272, movie_array);
    memset(movie_array, 0, movie_asset_count * sizeof(MOVIE));

    PKRAssetTOCInfo temp_asset_info;
    for (S32 i = 0; i < movie_asset_count; i++) {
        xSTGetAssetInfoByType('BINK', i, &temp_asset_info);
        const char* hipname = xST_xAssetID_HIPFullPath(temp_asset_info.aid);
        xASSERT_L(280, hipname);
        char fullname[255];
        iFileFullPath(hipname, fullname);

        S32 bFileOpenOk = DVDOpen(fullname, &movie_array[i].file);
        if (!bFileOpenOk) {
            xFAIL_MSG_L(287, "Trouble with the movie");
            iprintf("failed to open movie\n");
            continue;
        }
        movie_array[i].aid = temp_asset_info.aid;
    }
}

void iFMVSceneExit()
{
    for (S32 i = 0; i < movie_asset_count; i++) {
        DVDClose(&movie_array[i].file);
    }
    movie_asset_count = 0;
    movie_array = NULL;
}

MOVIE* iFMVOpen(PKRAssetTOCInfo& rAssetInfo)
{
    RADARAMCALLBACKS OurGCCallbacks;
    OurGCCallbacks.aram_malloc = iSystemARAMAlloc;
    OurGCCallbacks.aram_free = iSystemARAMFree;
    BinkSoundUseAX(&OurGCCallbacks);

    iTRCDisk::SetMovieResetCallBack(iFMVResetCallback);
    iTRCDisk::EnableMovieResetCallBack(true);

    BinkSetMemory(BigMovieAlloc, BigMovieFree);

    MOVIE* pMovie = NULL;
    for (S32 i = 0; i < movie_asset_count; i++) {
        if (rAssetInfo.aid == movie_array[i].aid) {
            pMovie = &movie_array[i];
            break;
        }
    }

    if (!pMovie) {
        iprintf("failed to open movie\n");
        iFMVClose(pMovie);
    }
    xASSERT_L(335, pMovie);

    if ((gGameMode == eGameMode_Game || gGameMode == eGameMode_Pause) && !sARAMtmp) {
        sARAMtmp = true;
        amt = iARAMTmpBegin('ANIM', 0x280000, true);
    }

    pMovie->uMovieOffsetBytes = rAssetInfo.sector * iFileGetSectorSize(XFILE_DRIVE_CDVD) + rAssetInfo.plus_offset;
    pMovie->file.cb.userData = (void*)pMovie->uMovieOffsetBytes;
    pMovie->pBinkTexture = NULL;
    pMovie->hBink = BinkOpen((const char*)&pMovie->file, BINKFILEHANDLE | BINKNOFILLIOBUF);
    if (!pMovie->hBink) {
        iprintf("disk cover opened, bailing out\n");
        iFMVClose(pMovie);
    } else {
        S32 width = NextPow2(pMovie->hBink->Width);
        S32 height = NextPow2(pMovie->hBink->Height);
        RwRaster* pBinkRaster = RwRasterCreate(width, height, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
        if (!pBinkRaster) {
            iprintf("failed to allocate lpBinkTexture, Uhoh\n");
            iFMVClose(pMovie);
        } else {
            RwRaster* pNullOnErr = RwRasterPushContext(pBinkRaster);
            xASSERT_L(373, pNullOnErr);
            RwRasterClear(*(RwInt32*)&g_BLACK);
            pNullOnErr = RwRasterPopContext();
            xASSERT_L(376, pNullOnErr);

            pMovie->pBinkTexture = RwTextureCreate(pBinkRaster);
            if (!pMovie->pBinkTexture) {
                iprintf("failed to allocate lpBinkTexture, Uhoh\n");
                iFMVClose(pMovie);
            }
        }
    }

    return pMovie;
}

void iFMVClose(MOVIE*& pMovie)
{
    iTRCDisk::SetMovieResetCallBack(NULL);
    iTRCDisk::EnableMovieResetCallBack(false);

    if (pMovie->pBinkTexture) {
        RwTextureDestroy(pMovie->pBinkTexture);
        pMovie->pBinkTexture = NULL;
    }

    BinkClose(pMovie->hBink);
    pMovie->hBink = NULL;
    pMovie = NULL;

    if (sARAMtmp) {
        sARAMtmp = false;
        iARAMTmpEnd('ANIM', true);
    }
}

U32 iFMVPlay(const char* filename, U32 buttons, F32 time, U32 uSubtitlesAID, bool skippable, bool lockController, bool displayDemoBlink)
{
    char fullname[64];
    char BIK[5] = ".BIK";
    char BIP[5] = ".BIP";

    const char* extension = zMainIsNTSC() ? BIK : BIP;

    if (IsPS2Demo()) {
        strcpy(fullname, xglobals->PS2demo->subdir);
        strcat(fullname, "\\");
        strcat(fullname, filename);
        strcat(fullname, extension);
    } else {
        strcpy(fullname, filename);
        strcat(fullname, extension);
    }

    if (!iFileExists(fullname, 0)) {
        return 1;
    }

    xFMVPauseSound(true);

    eGameOstrich old = zGameGetOstrich();
    zGameSetOstrich(eGameOstrich_PlayingMovie);

    xRegion localRegion = xSTGetLocalizationEnum();
    U32 ret = iFMVPlay(fullname, buttons, time, uSubtitlesAID, skippable, lockController, displayDemoBlink, localRegion);

    zGameSetOstrich(old);

    xFMVPauseSound(false);
    return ret;
}

void iFMVSuspend()
{
    if (hBinkCurrMovie) {
        BinkPause(hBinkCurrMovie, 1);
    }

    for (S32 i = 0; i < movie_asset_count; i++) {
        if (movie_array[i].hBink && !movie_array[i].movie_is_suspended) {
            movie_array[i].disk_open_frame = movie_array[i].hBink->LastFrameNum;
            BinkPause(movie_array[i].hBink, 1);
            movie_array[i].movie_is_suspended = 1;
        }
    }
}

void iFMVResume()
{
    for (S32 i = 0; i < movie_asset_count; i++) {
        if (movie_array[i].movie_is_suspended) {
            movie_array[i].file.cb.userData = (void*)movie_array[i].uMovieOffsetBytes;
            BinkClose(movie_array[i].hBink);
            movie_array[i].hBink = NULL;

            BINK* hBink = BinkOpen((const char*)&movie_array[i].file, BINKFILEHANDLE | BINKNOFILLIOBUF);
            if (hBink) {
                BinkGoto(hBink, movie_array[i].disk_open_frame, 0);
                movie_array[i].hBink = hBink;
                movie_array[i].movie_is_suspended = 0;
            }
        }
    }
}

static U32 iFMVPlay(const char* filename, U32 buttons, F32, U32 uSubtitlesAID, bool skippable, bool, bool displayDemoBlink, S32 uAudioTrack)
{
    for (S32 j = 0; j < k_XPAD_MAX; j++) {
        if (xTRCPadIsPort(j, TRC_PadActive))
            iPadStopRumble(&mPad[j]);
    }

    if (skippable) {
        buttons |= 0x20002;
    }

    char fullFile[256];
    U32 uBinkFlags = 0;
    if (!filename) {
        return 1;
    }
    iFileFullPath(filename, fullFile);

    uBinkFlags = 0;
    BinkSetSoundTrack(1, (U32*)&uAudioTrack);
    uBinkFlags |= BINKSNDTRACK;

    BINK* hBinkMovie = iFMVOpen(fullFile, uBinkFlags);
    if (!hBinkMovie) {
        iprintf("Unable to open bink file %s\n", fullFile);

        iTime delay = iTimeGet();
        iTime now = iTimeGet();
        while (delay + 200 > now)
            now = iTimeGet();

        if (sARAMtmp) {
            sARAMtmp = false;
            iARAMTmpEnd('ANIM', true);
        }

        return 1;
    }

    bool bTrackFound = false;
    for (S32 track = 0; track < hBinkMovie->NumTracks; track++) {
        if (uAudioTrack == hBinkMovie->trackIDs[track]) {
            bTrackFound = true;
            break;
        }
    }

    if (!bTrackFound) {
        iFMVClose(hBinkMovie);

        hBinkMovie = iFMVOpen(fullFile, uBinkFlags & ~BINKSNDTRACK);
        uAudioTrack = BinkGetTrackID(hBinkMovie, 0);
    }

    if (stricmp(filename, "FMV\\Attract1.BIK") == 0)
        BinkSetVolume(hBinkMovie, uAudioTrack, 0x8000);
    else
        BinkSetVolume(hBinkMovie, uAudioTrack, xFMVGetBinkCompliantVolume());

    RwRaster* pCamRaster = RwCameraGetRaster(xglobals->screen->icam);
    xASSERT_L(585, pCamRaster);
    RwInt32 iBackBufWidth = RwRasterGetWidth(pCamRaster);
    RwInt32 iBackBufHeight = RwRasterGetHeight(pCamRaster);
    S32 iSafeWidth = iBackBufWidth * 1.0f;
    S32 iSafeHeight = iBackBufHeight * 1.0f;
    iSafeWidth = (iSafeWidth + 3) & ~3;
    iSafeHeight = (iSafeHeight + 3) & ~3;

    RwRect MovieRect = { 0, 0, hBinkMovie->Width, hBinkMovie->Height };
    F32 aspect_ratio_mult = 1.0f;

    if (MovieRect.w > iSafeWidth) {
        MovieRect.h = (F32)MovieRect.h * iSafeWidth / MovieRect.w;
        MovieRect.w = iSafeWidth;
    }

    if (MovieRect.h > iSafeHeight) {
        MovieRect.w = (F32)MovieRect.w * iSafeHeight / MovieRect.h;
        MovieRect.h = iSafeHeight;
    }

    MovieRect.h = MovieRect.h * aspect_ratio_mult;
    if (MovieRect.h > iSafeHeight) {
        MovieRect.w = (F32)MovieRect.w * iSafeHeight / MovieRect.h;
        MovieRect.h = iSafeHeight;
    }

    S32 iHorizPad = iBackBufWidth - MovieRect.w;
    S32 iVertPad = iBackBufHeight - MovieRect.h;
    if (iHorizPad < 0)
        iHorizPad = 0;
    if (iVertPad < 0)
        iVertPad = 0;
    MovieRect.x = iHorizPad / 2;
    MovieRect.y = iVertPad / 2;

    RwCamera* cam = iCameraCreate(FB_XRES, FB_YRES, 0);
    RwRGBA bg = { 0, 0, 0, 0 };
    RwCameraClear(cam, &bg, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
    RwCameraShowRaster(cam, NULL, 1);
    RwCameraShowRaster(cam, NULL, 1);
    iCameraDestroy(cam);
    VIWaitForRetrace();
    VISetBlack(FALSE);
    VIFlush();

    iTime fmvAbortDelay = iTimeGet();
    bool aborted = false;

    iTime dwTimeStart = iTimeGet();
    iTime dwTimeNow = iTimeGet();

    void* xfbDisp, *xfbCopy;
    RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
    void* xfbDest = xfbCopy;

    bool bUseYUVPath = (uSubtitlesAID == 0);
    RwRaster* pBinkRaster = NULL;
    RwGameCubeRasterExtension* pRasterExt = NULL;
    S32 iBinkMovieHalfHeight = (S32)hBinkMovie->Height / 2;
    if (!bUseYUVPath) {
        xASSERT_L(652, 0==(hBinkMovie->Height & 1));

        pBinkRaster = RwRasterCreate(hBinkMovie->Width, iBinkMovieHalfHeight, 32, rwRASTERTYPENORMAL | rwRASTERFORMAT888 | rwRASTERDONTALLOCATE);
        pRasterExt = RwGameCubeRasterGetExtension(pBinkRaster);
    } else {
        VISetBlack(FALSE);
    }

    hBinkCurrMovie = hBinkMovie;
    bool waiting = false;

    while (hBinkMovie->FrameNum != hBinkMovie->Frames && !aborted) {
        if (iTRC_CHECK_L(670)) {
            RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
            VISetBlack(TRUE);

            VISetNextFrameBuffer(xfbDisp);

            VIFlush();
            VISetBlack(FALSE);

            break;
        }

        if (!waiting) {
            dwTimeNow = iTimeGet();
            if (fmvAbortDelay + ((DEBUG || RELEASE) ? 500.0f : 2000.0f) <= dwTimeNow && skippable) {
                iPadUpdateDataAll();

                for (S32 i = 0; i < k_XPAD_MAX; i++) {
                    xPad* pad = &mPad[i];
                    pad->pressed = 0;
                    pad->released = 0;
                    xPadUpdate(i, 0.0f);

                    if ((pad->pressed & buttons) && (!xTRCPadCountWereActiveTotal() || xTRCPadPortWasActive(i))) {
                        pad->pressed = 0;
                        pad->released = 0;
                        aborted = true;
                        break;
                    }
                }
            }

            if (gSoak) {
                F32 time = hBinkMovie->FrameNum / ((F32)hBinkMovie->FrameRate / hBinkMovie->FrameRateDiv);
                if (time > 2.0f) {
                    aborted = true;
                }
            }

            S32 oneIfSkip = BinkDoFrame(hBinkMovie);

            if (bUseYUVPath) {
                oneIfSkip = BinkCopyToBuffer(hBinkMovie, xfbDest, iBackBufWidth * 2, hBinkMovie->Height, MovieRect.x, MovieRect.y, BINKCOPYALL | BINKSURFACEYUY2);

                S32 offset = (MovieRect.x + iBackBufWidth * MovieRect.y) * 2;
                void* newDest = (U8*)xfbDest + offset;
                DCStoreRange(newDest, hBinkMovie->Height * (iBackBufWidth * 2));

                if (oneIfSkip == 0) {
                    VISetNextFrameBuffer(xfbDest);

                    VIFlush();

                    if (xfbDest == xfbCopy)
                        xfbDest = xfbDisp;
                    else
                        xfbDest = xfbCopy;
                } else {
                    iprintf("iFMVOpen: Skipping frame %d\n", hBinkMovie->FrameNum);
                }
            } else {
                RwRaster* pNullOnErr = RwRasterPushContext(pCamRaster);
                xASSERT_L(759, pNullOnErr);

                RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
                pRasterExt->pixels = (RwUInt8*)xfbCopy;

                U8* pBits = RwRasterLock(pBinkRaster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);
                xASSERT_L(766, pBits);

                oneIfSkip = BinkCopyToBufferRect(hBinkMovie, pBits, RwRasterGetStride(pBinkRaster), iBinkMovieHalfHeight, 0, 0, 0, 0, RwRasterGetWidth(pBinkRaster), iBinkMovieHalfHeight, BINKCOPYALL | BINKSURFACE32);

                pNullOnErr = RwRasterUnlock(pBinkRaster);

                if (oneIfSkip == 0) {
                    RwRasterClear(0xFF000000);
                    pNullOnErr = RwRasterRenderFast(pBinkRaster, MovieRect.x, MovieRect.y);

                    pBits = RwRasterLock(pBinkRaster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);
                    xASSERT_L(792, pBits);

                    BinkCopyToBufferRect(hBinkMovie, pBits, RwRasterGetStride(pBinkRaster), iBinkMovieHalfHeight, 0, 0, 0, iBinkMovieHalfHeight, RwRasterGetWidth(pBinkRaster), iBinkMovieHalfHeight, BINKCOPYALL | BINKNOSKIP | BINKSURFACE32);

                    pNullOnErr = RwRasterUnlock(pBinkRaster);
                    pNullOnErr = RwRasterRenderFast(pBinkRaster, MovieRect.x, MovieRect.y + iBinkMovieHalfHeight);
                } else {
                    iprintf("iFMVOpen: Skipping frame %d\n", hBinkMovie->FrameNum);
                }

                pNullOnErr = RwRasterPopContext();

                pRasterExt->pixels = NULL;
                if (uSubtitlesAID || displayDemoBlink) {
                    xglobals->screen->begin_render(false);
                    F32 fTime = (F32)(hBinkMovie->FrameNum * hBinkMovie->FrameRateDiv) / hBinkMovie->FrameRate;
                    xSubTitlesRender(uSubtitlesAID, fTime);
                    if (displayDemoBlink)
                        zGameDemoText(fTime);
                    xglobals->screen->end_render();
                }

                iCameraShowRaster(xglobals->screen->icam);
            }

            if (hBinkMovie->FrameNum != hBinkMovie->Frames) {
                BinkNextFrame(hBinkMovie);
            }
        }

        waiting = BinkWait(hBinkMovie);
    }

    if (bUseYUVPath) {
        RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
        VISetBlack(TRUE);

        VISetNextFrameBuffer(xfbDisp);

        VIFlush();
        VIWaitForRetrace();
        VISetBlack(FALSE);
    } else {
        RwRasterDestroy(pBinkRaster);
    }

    iFMVClose(hBinkMovie);
    hBinkCurrMovie = NULL;

    xTRCPadRestoreAll();

    if (aborted) {
        return 2;
    } else {
        return 0;
    }
}

void iFMVResetCallback()
{
    iprintf("iFMV: In iFMVResetCallBack\n");
    iTRC_CHECK_L(882);
    iprintf("iFMV: Resetting iFMVResetCallBack\n");

    OSSetResetCallback(iFMVResetCallback);
}

void iFMVKill()
{
    for (S32 i = 0; i < movie_asset_count; i++) {
        if (movie_array[i].hBink) {
            BinkClose(movie_array[i].hBink);
            movie_array[i].hBink = NULL;
        }
    }

    if (hBinkCurrMovie) {
        BinkClose(hBinkCurrMovie);
        hBinkCurrMovie = NULL;
    }

    iTRCDisk::SetMovieResetCallBack(NULL);
    iTRCDisk::EnableMovieResetCallBack(false);

    if (sARAMtmp) {
        sARAMtmp = false;
        iARAMTmpEnd('ANIM', false);
    }
}
