#include "xCommon.h"
#include "iSnd.h"

#include "xSnd.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "xFile.h"
#include "xGlobals.h"
#include "iCutscene.h"
#include "iSystem.h"
#include "iTRC.h"
#include "zGame.h"
#include "zMain.h"
#include "xutil.h"

#include "decomp.h"

#include <fmodgc.h>
#include <rwcore.h>
#include <string.h>

#define MAX_SND_TABLES (12)
#define MAX_TEMP_STREAMS (4)
#define MAX_EXTERNAL_STREAMS (2)

struct ExternalStreamInfo
{
    FSOUND_STREAM* stream;
    S32 bytesToConsume;
    S32 frontBuffer;
    void* pData;
};

struct sDSPADPCM : FSOUND_GCADPCM_HDR
{
    U32 uAssetID;
};

struct gcSoundInfoAsset
{
    U32 uSoundInfoID;
    union
    {
        U32 uSumFSBSize;
        S32 nOffsetFinalFSBEnd;
    };
    FMUSIC_MODULE* pFMusicMod;
    S32* pFSBFileArray;
    gcWavInfo* pWavInfoArray;
    sDSPADPCM* pCutsceneAudioHeaders;
    U16 nWavFiles;
    U16 nSounds;
    U16 nStreams;
    U8 nFSBFiles;
    U8 nCutsceneAudioHeaders;
};

static void* sSndStaticBuffer;
ExternalStreamInfo externStreamInfo[MAX_EXTERNAL_STREAMS];
gcSoundInfoAsset sinfoArray[MAX_SND_TABLES];
U32 sinfoArrayLoaded;
static xSndVoiceInfo* pTempStreams[MAX_TEMP_STREAMS];
static bool sOldPauseState[xSndCat_LAST_CATEGORY + 1];
static bool sReverbSetup;

static bool loadSoundInfoAsset(U32 uInfoArrayIndex, PKRAssetTOCInfo& rAssetInfo);
static bool unloadSoundInfoAsset(U32 uInfoArrayIndex);
static sDSPADPCM* iSndFindCutsceneHeader(U32 id);
static void iSndCloseStream(S32 idx, bool bFreeRef);
static bool iSndPlayDone(xSndVoiceInfo* vp);

#if DEBUG
U32 gChkStreams[10] = { 0 };
U32* pChk = gChkStreams;

static void DetectLeakStreamOpen(U32 sndID, const char* name)
{
    OSReport("Created stream %d name = %s\n", sndID, name);
    gNumStreamsOpened++;

    while (*pChk != 0) {
        pChk++;
        if (pChk == gChkStreams + ARRAY_LENGTH(gChkStreams))
            pChk = gChkStreams;
    }
    *pChk = sndID;
}

static void DetectLeakStreamClose(U32 sndID)
{
    gNumStreamsClosed++;

    for (U32 i = 0; i < ARRAY_LENGTH(gChkStreams); i++) {
        if (sndID == gChkStreams[i]) {
            gChkStreams[i] = 0;
            break;
        }
    }
    if (gNumStreamsOpened != gNumStreamsClosed)
        OSReport("still open: ");
    for (U32 i = 0; i < ARRAY_LENGTH(gChkStreams); i++) {
        if (gChkStreams[i] != 0) {
            OSReport("%d ", gChkStreams[i]);
        }
    }
}
#endif

void iSndInit()
{
    for (xSndVoiceInfo* voice = gSnd.voice; voice < gSnd.voice + XSND_TOTAL_VOICES; voice++) {
        voice->ps.nChannel = -1;
    }
    for (S32 i = 0; i < MAX_TEMP_STREAMS; i++) {
        pTempStreams[i] = NULL;
    }

    iSystemARAMInit();

    if (FSOUND_GetVersion() < FMOD_VERSION) {
        iprintf("Error : You are using the wrong LIB version!  You should be using FMOD %.02f\n", FMOD_VERSION);
        return;
    }

    if (!FSOUND_SetAramBlock(iSystemARAMBasePtrForFMOD(), iSystemARAMSizeForFMOD())) {
        return;
    }

    FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT);

    sSndStaticBuffer = RwMallocL(ISND_STATIC_BUFFER_SIZE, rwMEMHINTDUR_NADURATION, 196);
    xASSERT_L(197, sSndStaticBuffer != NULL);
    xASSERT_L(201, ((U32)sSndStaticBuffer & 0x0000001F) == 0);
    FSOUND_SetMemorySystem(sSndStaticBuffer, ISND_STATIC_BUFFER_SIZE, NULL, NULL, NULL);

    FSOUND_SetMaxHardwareChannels(ISND_MAX_VOICES);

    if (!FSOUND_Init(32000, 0, FSOUND_INIT_GC_INITLIBS)) {
        iprintf("Error!\n");
        return;
    }

    FSOUND_3D_SetRolloffFactor(0.0f);

    if (OSGetSoundMode() == OS_SOUND_MODE_MONO)
        xSndSetMode(xSndMode_Mono);
    else
        xSndSetMode(xSndMode_Stereo);
    sReverbSetup = false;
}

void iSndInitSceneLoaded()
{
    S32 nSInfos = xSTAssetCountByType('SNDI');
    xASSERT_L(236, nSInfos <= MAX_SND_TABLES);
    xASSERT_L(237, (sinfoArrayLoaded + nSInfos) <= MAX_SND_TABLES);

    for (S32 i = 0; i < nSInfos; i++) {
        PKRAssetTOCInfo ainfo;
        xSTGetAssetInfoByType('SNDI', i, &ainfo);
        for (S32 i = 0; i < sinfoArrayLoaded; i++) {
            if (ainfo.aid == sinfoArray[i].uSoundInfoID) goto next;
        }
        bool bOk = loadSoundInfoAsset(sinfoArrayLoaded, ainfo);
        if (bOk) {
            sinfoArrayLoaded++;
        }
    next:;
    }
}

static bool loadSoundInfoAsset(U32 uInfoArrayIndex, PKRAssetTOCInfo& rAssetInfo)
{
    bool retval = true;
    gcSoundInfoAsset& rSoundAsset = sinfoArray[uInfoArrayIndex];
    U32 uFileSize = 0;
    const char* hipname = xST_xAssetID_HIPFullPath(rAssetInfo.aid);
    xASSERT_L(266, hipname);

    xFile hopFile;
    U32 open = iFileOpen(hipname, IFILE_READ, &hopFile);

    S32 uSoundInfoOffsetBytes = rAssetInfo.plus_offset + rAssetInfo.sector * iFileGetSectorSize(XFILE_DRIVE_CDVD);

    iFileSeek(&hopFile, uSoundInfoOffsetBytes, 0);
    iFileRead(&hopFile, &rSoundAsset, sizeof(gcSoundInfoAsset));

    xASSERT_L(275, rSoundAsset.uSoundInfoID);

    U32 dataBufferSize = 0;
    dataBufferSize += rSoundAsset.nFSBFiles * sizeof(S32);
    dataBufferSize += rSoundAsset.nWavFiles * sizeof(gcWavInfo);
    dataBufferSize += rSoundAsset.nCutsceneAudioHeaders * sizeof(sDSPADPCM);
    dataBufferSize = (dataBufferSize + 31) & ~31;
    U8* dataBuffer = (U8*)xMEMALLOC_L(dataBufferSize, 32, eMemMgrTag_Audio, 0, 282);

    iFileSeek(&hopFile, rSoundAsset.uSumFSBSize, 1);
    if (dataBufferSize) {
        iFileRead(&hopFile, dataBuffer, dataBufferSize);
    }
    iFileClose(&hopFile);

    if (rSoundAsset.nFSBFiles) {
        rSoundAsset.pFSBFileArray = (S32*)dataBuffer;
    }
    if (rSoundAsset.nWavFiles) {
        rSoundAsset.pWavInfoArray = (gcWavInfo*)(dataBuffer + rSoundAsset.nFSBFiles * sizeof(S32));
    }
    if (rSoundAsset.nCutsceneAudioHeaders) {
        rSoundAsset.pCutsceneAudioHeaders = (sDSPADPCM*)(rSoundAsset.pWavInfoArray + rSoundAsset.nWavFiles);
    }

    for (S32 i = 0; i < rSoundAsset.nFSBFiles; i++) {
        rSoundAsset.pFSBFileArray[i] += uSoundInfoOffsetBytes + sizeof(gcSoundInfoAsset);
    }

    for (S32 i = 0; i < rSoundAsset.nWavFiles; i++) {
        rSoundAsset.pWavInfoArray[i].uSoundInfoIndex = uInfoArrayIndex;
    }

    if (rSoundAsset.nSounds) {
        S32 uStaticBufferLength = 0;
        if (rSoundAsset.nFSBFiles > 1)
            uStaticBufferLength = rSoundAsset.pFSBFileArray[1] - rSoundAsset.pFSBFileArray[0];
        else
            uStaticBufferLength = rSoundAsset.uSumFSBSize;

        iprintf("%s static file size %f\n", hipname, uStaticBufferLength / 1024.0f);

        rSoundAsset.pFMusicMod = FMUSIC_LoadSongEx(hipname, rSoundAsset.pFSBFileArray[0], uStaticBufferLength,
                                                   FSOUND_NONBLOCKING | FSOUND_GCADPCM, NULL, 0);
        if (!rSoundAsset.pFMusicMod) {
            iprintf("failed static music load in %s\n", hipname);
            xFAIL_MSG_L(331, "failed static music load ");
        }

        volatile S32 nOpenState = FMUSIC_GetOpenState(rSoundAsset.pFMusicMod);

        while (nOpenState == -2) {
            nOpenState = FMUSIC_GetOpenState(rSoundAsset.pFMusicMod);
            if (iTRC_CHECK_L(338)) {
                zMainShowProgressBar(-1);
                zMainShowProgressBar(-1);
            }
            iVSync();
        }

        switch (nOpenState) {
        case 0:
            iprintf("soundbank in %s loaded!\n", hipname);
            retval = true;
            break;
        case -1:
            iprintf("soundbank in %s invalid handle\n", hipname);
            retval = false;
            break;
        case -2:
            iprintf("soundbank in %s still loading?!?\n", hipname);
            retval = true;
            break;
        case -3:
            xASSERT_FMT_L(362, 0, "soundbank in %s failed!\n", hipname);
            retval = false;
        }
    }

    rSoundAsset.nOffsetFinalFSBEnd += uSoundInfoOffsetBytes + sizeof(gcSoundInfoAsset);

    if (rSoundAsset.nStreams) {
        FSOUND_Stream_SetBufferSize(1000);
    }

    return retval;
}

static bool unloadSoundInfoAsset(U32 uInfoArrayIndex)
{
    gcSoundInfoAsset& rSoundAsset = sinfoArray[uInfoArrayIndex];
    if (rSoundAsset.nSounds) {
        xASSERT_L(395, rSoundAsset.pFMusicMod);
        S8 bOK = FMUSIC_FreeSong(rSoundAsset.pFMusicMod);
        iprintf("soundbank %d unloaded!\n", uInfoArrayIndex);
        rSoundAsset.pFMusicMod = NULL;
        return bOK;
    }
    return true;
}

void iSndSceneExit()
{
    for (U32 i = 0; i < sinfoArrayLoaded; i++) {
        bool bOk = unloadSoundInfoAsset(i);
    }
    sinfoArrayLoaded = 0;

    xSndMgrSetEffect(xSndEffect_NONE);

    FSOUND_AUXFX_ShutdownEffect(0);
    sReverbSetup = false;
}

void iSndExit()
{
    FSOUND_Close();
}

iSndFileInfo* iSndLookup(U32 id)
{
    gcSoundInfoAsset* pSoundBankInfo = NULL;
    for (S32 i = 0; i < sinfoArrayLoaded; i++) {
        pSoundBankInfo = &sinfoArray[i];
        S32 lowerIndex = 0;
        S32 upperIndex = pSoundBankInfo->nWavFiles;

        while (upperIndex > lowerIndex) {
            S32 thisIndex = (upperIndex + lowerIndex) / 2;

            if (id == pSoundBankInfo->pWavInfoArray[thisIndex].uAssetID) {
                return (iSndFileInfo*)&pSoundBankInfo->pWavInfoArray[thisIndex];
            }

            if (id > pSoundBankInfo->pWavInfoArray[thisIndex].uAssetID)
                lowerIndex = thisIndex + 1;
            else
                upperIndex = thisIndex;
        }
    }
    return NULL;
}

static sDSPADPCM* iSndFindCutsceneHeader(U32 id)
{
    gcSoundInfoAsset* pSoundBankInfo = NULL;
    for (S32 i = 0; i < sinfoArrayLoaded; i++) {
        pSoundBankInfo = &sinfoArray[i];
        sDSPADPCM* pHeader = NULL;
        for (S32 j = 0; j < pSoundBankInfo->nCutsceneAudioHeaders; j++) {
            pHeader = &pSoundBankInfo->pCutsceneAudioHeaders[j];
            if (pHeader->uAssetID == id) {
                return pHeader;
            }
        }
    }
    return NULL;
}

bool initBytesToConsume(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == NULL) {
            externStreamInfo[i].stream = stream;
            externStreamInfo[i].bytesToConsume = 0;
            externStreamInfo[i].frontBuffer = 1;
            externStreamInfo[i].pData = NULL;
            return true;
        }
    }
    return false;
}

bool releaseBytesToConsume(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].stream = NULL;
            externStreamInfo[i].bytesToConsume = 0;
            externStreamInfo[i].frontBuffer = 1;
            externStreamInfo[i].pData = NULL;
            return true;
        }
    }
    return false;
}

S32 getBytesToConsume(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream)
            return externStreamInfo[i].bytesToConsume;
    }
    return -1;
}

S32 getFrontBuffer(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream)
            return externStreamInfo[i].frontBuffer;
    }
    return -1;
}

void* getDataPtr(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream)
            return externStreamInfo[i].pData;
    }
    return NULL;
}

bool setBytesToConsume(FSOUND_STREAM* stream, S32 bytesToConsume)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].bytesToConsume = bytesToConsume;
            return true;
        }
    }
    return false;
}

bool setFrontBuffer(FSOUND_STREAM* stream, S32 frontBuffer)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].frontBuffer = frontBuffer;
            return true;
        }
    }
    return false;
}

bool setDataPtr(FSOUND_STREAM* stream, void* pData)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].pData = pData;
            return true;
        }
    }
    return false;
}

S8 iSndExternalCB(FSOUND_STREAM* stream, void* buff, S32 len, void* userdata)
{
    xASSERT_L(593, userdata);
    S32 bytesToConsume = getBytesToConsume(stream);
    S32 frontbuffer = getFrontBuffer(stream);
    void* pData = getDataPtr(stream);
    xASSERT_L(597, bytesToConsume != -1);
    if (bytesToConsume == -1)
        return FALSE;
    if (bytesToConsume == 0) {
        U32 datasize = 0;
        U32 assetID = (U32)userdata;
        pData = iCSSoundGetData(assetID, &datasize);
        bytesToConsume += datasize;
    }

    if (bytesToConsume < len) {
        if (bytesToConsume == 0)
            return FALSE;
        xASSERT_L(614, pData);
        memcpy(buff, pData, bytesToConsume);
        U8* pEndOfCpy = (U8*)buff + bytesToConsume;
        memset(pEndOfCpy, 0, len - bytesToConsume);
        bytesToConsume = 0;
    } else {
        xASSERT_L(622, pData);
        memcpy(buff, pData, len);
        bytesToConsume -= len;
        pData = (U8*)pData + len;
    }

    bool retval = setBytesToConsume(stream, bytesToConsume);
    xASSERT_L(628, retval);

    if (frontbuffer) {
        FSOUND_GC_Stream_SetGCADPCMLoopContext(stream);
    }
    frontbuffer ^= 1;
    retval = setFrontBuffer(stream, frontbuffer);
    xASSERT_L(639, retval);
    retval = setDataPtr(stream, pData);
    xASSERT_L(641, retval);

    return TRUE;
}

static void iSndCloseStream(S32 idx, bool bFreeRef)
{
    if (gSnd.voice[idx].flags & xSnd_CUTSCENE) {
        bool retval = releaseBytesToConsume(gSnd.voice[idx].ps.pStream);
        xASSERT_L(652, retval == true);
    }

    S32 nOpenState = FSOUND_Stream_GetOpenState(gSnd.voice[idx].ps.pStream);
    if (nOpenState == -2) {
        iprintf("Couldn't close stream %d, still opening\n", gSnd.voice[idx].sndID.GetID());
        return;
    }
    if (nOpenState < 0) {
#if DEBUG
        iprintf("Closing broken stream %d? name = %s\n", gSnd.voice[idx].sndID.GetID(),
                xSTAssetName(gSnd.voice[idx].assetID));
#endif
    }

#if DEBUG
    iprintf("Closing stream %d ", gSnd.voice[idx].sndID.GetID());
    DetectLeakStreamClose(gSnd.voice[idx].sndID.GetID());
    iprintf("\n");
#endif

    xASSERT_L(680, gSnd.voice[idx].ps.pStream);

    iTRCDisk::SetInFModLockingCode(true);
    S8 closed = FSOUND_Stream_Close(gSnd.voice[idx].ps.pStream);
    iTRCDisk::SetInFModLockingCode(false);

    xASSERT_L(686, closed);
    gSnd.voice[idx].ps.pStream = NULL;
    gSnd.voice[idx].ps.nChannel = -1;
    gSnd.voice[idx].flags = 0;
    if (bFreeRef) {
        xSndMgrFreeRef(gSnd.voice[idx].sndUniqueID);
    }

    for (S32 nIndex = 0; nIndex < MAX_TEMP_STREAMS; nIndex++) {
        if (pTempStreams[nIndex] == &gSnd.voice[idx]) {
            pTempStreams[nIndex] = NULL;
            break;
        }
    }
}

bool iSndPlay(xSndVoiceInfo* vp)
{
    gcSoundInfoAsset* pSoundBankInfo = NULL;
    iSndFileInfo* pSndInfo = vp->pSndFileInfo;
    FSOUND_SAMPLE* pSample;

    if (!(vp->flags & xSnd_CUTSCENE)) {
        if (!pSndInfo) {
            goto failed;
        }

        pSoundBankInfo = &sinfoArray[pSndInfo->uSoundInfoIndex];
        xASSERT_L(749, pSndInfo->uFSBIndex < pSoundBankInfo->nFSBFiles);
    }

    if ((vp->flags & xSnd_CUTSCENE) || pSndInfo->isStreamed()) {
        U32 uOpenMode = FSOUND_GCADPCM;
        if (vp->flags & xSnd_CALC3D)
            uOpenMode |= FSOUND_HW3D;
        else
            uOpenMode |= FSOUND_HW2D;

        if ((pSndInfo && pSndInfo->isLooping()) || (vp->flags & xSnd_LOOP))
            uOpenMode |= FSOUND_LOOP_NORMAL;
        else
            uOpenMode |= FSOUND_LOOP_OFF;

        if (vp->flags & xSnd_CUTSCENE) {
            sDSPADPCM* pHeader = iSndFindCutsceneHeader(vp->assetID);
            if (!pHeader) {
                goto failed;
            }

            vp->ps.pStream = FSOUND_Stream_Create(iSndExternalCB, 0x4000, uOpenMode | FSOUND_MONO, pHeader->sample_rate,
                                                  (void*)vp->assetID);
            if (!vp->ps.pStream) {
#if DEBUG
                iprintf("Failed to create stream %d name = %s\n", vp->sndID.GetID(), xSTAssetName(vp->assetID));
#endif
                goto failed;
            }
#if DEBUG
            DetectLeakStreamOpen(vp->sndID.GetID(), xSTAssetName(vp->assetID));
#endif

            xASSERT_L(784, vp->ps.pStream);
            bool retval = initBytesToConsume(vp->ps.pStream);
            xASSERT_L(786, retval == TRUE);

            FSOUND_GCADPCMINFO* info = (FSOUND_GCADPCMINFO*)pHeader->coef;
            pSample = FSOUND_Stream_GetSample(vp->ps.pStream);
            xASSERT_L(791, pSample);
            if (!pSample) {
                iSndCloseStream(vp - gSnd.voice, true);
                goto failed;
            }
            FSOUND_GC_Sample_SetGCADPCMInfo(pSample, info);
            S32 nChannel = FSOUND_Stream_PlayEx(FSOUND_FREE, vp->ps.pStream, NULL, TRUE);
            if (nChannel == -1) {
                iSndCloseStream(vp - gSnd.voice, true);
                goto failed;
            }
            vp->ps.nChannel = nChannel;
        } else {
            xSndVoiceInfo* pVoice = xSndMgr_GetFreeVirtualVoiceInfo(vp->priority, vp->flags, true);
            if (pVoice) {
                uOpenMode |= FSOUND_NONBLOCKING;

                S32 nOffsetToStartStream = pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex];
                S32 nStreamSize = 0;
                if (pSndInfo->uFSBIndex == pSoundBankInfo->nFSBFiles - 1)
                    nStreamSize = pSoundBankInfo->nOffsetFinalFSBEnd - pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex];
                else
                    nStreamSize = pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex + 1] - pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex];

                const char* hipname = xST_xAssetID_HIPFullPath(pSoundBankInfo->uSoundInfoID);
                xASSERT_L(821, hipname);

                vp->ps.pStream = FSOUND_Stream_Open(hipname, uOpenMode, nOffsetToStartStream, nStreamSize);
                if (!vp->ps.pStream) {
                    xASSERT_FMT_L(826, vp->ps.pStream, "iSndPlay: FSOUND_Stream_Open failed, name = %s\n",
                                  xSTAssetName(vp->assetID));
                    goto failed;
                }

#if DEBUG
                DetectLeakStreamOpen(vp->sndID.GetID(), xSTAssetName(vp->assetID));
#endif

                *pVoice = *vp;

                xSndMgrUpdateRef(pVoice->sndUniqueID, pVoice - gSnd.voice);

                vp->flags = 0;
                xASSERT_L(839, vp->ps.nChannel == -1);

                U32 nIndex;
                for (nIndex = 0; nIndex < MAX_TEMP_STREAMS; nIndex++) {
                    if (!pTempStreams[nIndex]) {
                        pTempStreams[nIndex] = pVoice;
                        break;
                    }
                }
                if (nIndex == MAX_TEMP_STREAMS) {
                    xASSERT_FMT_L(854, enableAudioAnnihilator, "Unable to save stream pointer, out of streams");
                    iSndCloseStream(pVoice - gSnd.voice, true);
                    goto failed;
                }
            } else {
                iprintf("failed to find free voice, need to fix!!!\n");
                goto failed;
            }

            iSndSetVol(pVoice, pVoice->vol);
            return true;
        }
    } else {
        xASSERT_L(870, pSndInfo && (pSndInfo->isStreamed()==false));
        xASSERT_L(871, pSoundBankInfo->pFMusicMod);
        xASSERT_L(872, FMUSIC_GetOpenState(pSoundBankInfo->pFMusicMod) == 0);
        xASSERT_L(873, pSndInfo->uAudioSampleIndex < FMUSIC_GetNumSamples(pSoundBankInfo->pFMusicMod));
        pSample = FMUSIC_GetSample(pSoundBankInfo->pFMusicMod, pSndInfo->uAudioSampleIndex);
        xASSERT_L(875, pSample);
        if (!pSample) {
            goto failed;
        }

        U32 samplemode = 0;
        if (vp->flags & xSnd_CALC3D)
            samplemode = FSOUND_HW3D;
        else
            samplemode = FSOUND_HW2D;

        if (pSndInfo->isLooping())
            FSOUND_Sample_SetMode(pSample, samplemode | FSOUND_LOOP_NORMAL);
        else
            FSOUND_Sample_SetMode(pSample, samplemode | FSOUND_LOOP_OFF);

        S32 nChannel = FSOUND_PlaySoundEx(FSOUND_FREE, pSample, NULL, TRUE);
        if (nChannel == -1) {
            goto failed;
        }

        xASSERT_L(894, nChannel!= -1);
        vp->ps.nChannel = nChannel;
    }

    S32 nRawChanZero = FSOUND_GetSubChannel(vp->ps.nChannel, 0);

    xASSERT_L(900, nRawChanZero > -1);
    if (vp != &gSnd.voice[nRawChanZero]) {
        if (gSnd.voice[nRawChanZero].flags & xSnd_ACTIVE) {
            iprintf("stolen voice detected %s(%d)\n", xSTAssetName(gSnd.voice[nRawChanZero].assetID), nRawChanZero);

            if (!gSnd.voice[nRawChanZero].pSndFileInfo) {
                iprintf("Dropping an external stream, this is bad.\n");
                iSndCloseStream(nRawChanZero, true);
            }
            else if (gSnd.voice[nRawChanZero].pSndFileInfo->isStreamed()) {
                iprintf("Please try not to drop streams, it's bad.\n");
                iSndCloseStream(nRawChanZero, true);
            }
            else if (gSnd.voice[nRawChanZero].pSndFileInfo->isLooping()) {
                S32 savePriority = vp->priority;
                vp->priority = 0xFF;
                xSndMgr_SetInaudible(&gSnd.voice[nRawChanZero]);
                vp->priority = savePriority;
            } else {
                iprintf("dropped a voice chnl %s (%d)\n", xSTAssetName(gSnd.voice[nRawChanZero].assetID), nRawChanZero);
                xSndMgrFreeRef(gSnd.voice[nRawChanZero].sndUniqueID);
            }
        }

        gSnd.voice[nRawChanZero] = *vp;

        xSndMgrUpdateRef(gSnd.voice[nRawChanZero].sndUniqueID, nRawChanZero);

        gSnd.voice[nRawChanZero].flags = vp->flags;

        vp->flags = 0;
        vp->ps.nChannel = -1;
    }
    iSndSetVol(&gSnd.voice[nRawChanZero], gSnd.voice[nRawChanZero].vol);
    return iSndPlayDone(&gSnd.voice[nRawChanZero]);

failed:
    return false;
}

static bool iSndPlayDone(xSndVoiceInfo* vp)
{
    S32 nRawChanZero = FSOUND_GetSubChannel(vp->ps.nChannel, 0);
    xASSERT_L(958, nRawChanZero > -1);

    FSOUND_SetPriority(vp->ps.nChannel, vp->priority);

    FSOUND_AUXFX_SetLevels(vp->ps.nChannel, 0, 0);

    iSndSetPitch(&gSnd.voice[nRawChanZero], gSnd.voice[nRawChanZero].xpitch);

    if (gSnd.voice[nRawChanZero].flags & xSnd_CALC3D) {
        S32 nChannelCount = FSOUND_GetNumSubChannels(vp->ps.nChannel);
        xASSERT_FMT_L(971, (nChannelCount < 2), "%s is stereo?!? Invalid 3d sound", xSTAssetName(vp->assetID));
        xASSERT_L(972, nChannelCount);

        F32 fMinDist = gSnd.voice[nRawChanZero].innerRadius;
        F32 fMaxDist = gSnd.voice[nRawChanZero].outerRadius;
        xASSERT_FMT_L(979, (fMinDist <= fMaxDist), "%s fMinDist(%f) > fMaxDist(%f)", xSTAssetName(vp->assetID), fMinDist, fMaxDist);
        xASSERT_FMT_L(980, (fMaxDist >= fMinDist), "%s fMinDist(%f) > fMaxDist(%f)", xSTAssetName(vp->assetID), fMinDist, fMaxDist);
        FSOUND_3D_SetMinMaxDistance(gSnd.voice[nRawChanZero].ps.nChannel, fMinDist, fMaxDist);

        xVec3 vPosInvX = gSnd.voice[nRawChanZero].actualPos;
        vPosInvX.x = -vPosInvX.x;
        S8 bOK = FSOUND_3D_SetAttributes(gSnd.voice[nRawChanZero].ps.nChannel, (F32*)&vPosInvX, NULL);
        xASSERT_L(997, bOK);
    }

    if (m_eCurrentEffect == xSndEffect_CAVE) {
        if (gSnd.voice[nRawChanZero].flags & xSnd_EFFECTS) {
            if (!sReverbSetup) {
                FSOUND_AUXFX_SetLevels(gSnd.voice[nRawChanZero].ps.nChannel, 0xFF, 0);

                S8 ret;
                switch (gSnd.sndMode) {
                case xSndMode_Dolby:
                    ret = FSOUND_AUXFX_SetReverbHiDpl2(1.5f, 0.008f, 0.0f, 1.0f, 0.04f);
                    break;
                default:
                    ret = FSOUND_AUXFX_SetReverbHi(0, 1.5f, 0.008f, 0.0f, 1.0f, 0.0f, 0.04f);
                }

                if (ret) {
                    sReverbSetup = true;
                }
            }
        }
    }

    if (!xSndMgrIsPaused(gSnd.voice[nRawChanZero].category) && !(gSnd.voice[nRawChanZero].flags & xSnd_PAUSED)) {
        FSOUND_SetPaused(gSnd.voice[nRawChanZero].ps.nChannel, FALSE);
    }
    xASSERT_L(1044, FSOUND_IsPlaying(gSnd.voice[nRawChanZero].ps.nChannel));

    return true;
}

void iSndStartStereo(xSndHandle, xSndHandle)
{
}

void iSndSetMode()
{
    switch (gSnd.sndMode) {
    case xSndMode_Mono:
        FSOUND_SetSpeakerMode(FSOUND_SPEAKERMODE_MONO);
        break;
    case xSndMode_Stereo:
        FSOUND_SetSpeakerMode(FSOUND_SPEAKERMODE_STEREO);
        break;
    case xSndMode_Dolby:
        FSOUND_SetSpeakerMode(FSOUND_SPEAKERMODE_PROLOGIC2);
        break;
    }
}

void iSndSetEnvironmentalEffect(S32 effectType)
{
    xASSERT_L(1086, m_eCurrentEffect == effectType);

    switch (effectType) {
    case xSndEffect_NONE:
        FSOUND_AUXFX_SetLevels(FSOUND_ALL, 0, 0);
        break;
    case xSndEffect_CAVE:
        break;
    }
}

bool iSndGetFreeStreamInfo(U32 priority)
{
    S32 numStreams = 0;

    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if (gSnd.voice[i].flags & xSnd_STREAMED)
            numStreams++;
    }

    if (numStreams < MAX_TEMP_STREAMS) {
        return true;
    }

    U32 lo_priority = 0xFFFFFFFF;
    U32 lo_voice = 0;
    for (U32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if (gSnd.voice[i].flags & xSnd_STREAMED) {
            if (gSnd.voice[i].priority < lo_priority) {
                lo_priority = gSnd.voice[i].priority;
                lo_voice = i + 4;
            }
        }
    }
    if (lo_priority < priority) {
        iSndStop(&gSnd.voice[lo_voice], true);
        return true;
    }

    return false;
}

xSndVoiceInfo* iSndGetFreeVoiceInfo(U32 priority, S32 flags)
{
    if (flags & xSnd_STREAMED) {
        if (!iSndGetFreeStreamInfo(priority))
            return NULL;
    }

    U32 lo_priority = 0xFFFFFFFF;
    U32 lo_voice = 0;
    S32 voiceIsUnused = FALSE;
    U32 i = 0;
    for (; i < ISND_MAX_VOICES; i++) {
        if (gSnd.voice[i].flags) {
            if (gSnd.voice[i].priority < lo_priority) {
                lo_priority = gSnd.voice[i].priority;
                lo_voice = i;
            }
        } else {
            xASSERT_L(1167, gSnd.voice[i].flags == 0);
            lo_voice = i; lo_priority = 0;
            voiceIsUnused = TRUE;
            break;
        }
    }

#if DEBUG
    if (i == ISND_MAX_VOICES) {
        iprintf("Voices are maxed. Channel %d : priority %d vs. PRI %d\n", lo_voice, lo_priority, priority);
    }
#endif

    if (voiceIsUnused || priority >= lo_priority) {
        if (gSnd.voice[lo_voice].ps.nChannel != -1) {
            U32 loopMode = FSOUND_GetLoopMode(gSnd.voice[lo_voice].ps.nChannel);
            bool islooped = loopMode && loopMode != FSOUND_LOOP_OFF;
            bool isstreaming = (gSnd.voice[lo_voice].flags & xSnd_CUTSCENE) || gSnd.voice[lo_voice].pSndFileInfo->isStreamed();
            if (islooped || isstreaming) {
                iprintf("Emergency stop on voice %d (%s)\n", lo_voice, xSTAssetName(gSnd.voice[lo_voice].assetID));
                iSndStop(&gSnd.voice[lo_voice], true);
            }
        }

        if (gSnd.voice[lo_voice].ps.nChannel != -1 && FSOUND_IsPlaying(gSnd.voice[lo_voice].ps.nChannel))
            return NULL;
        gSnd.voice[lo_voice].ps.nChannel = -1;
        return &gSnd.voice[lo_voice];
    }

    return NULL;
}

void iSndUpdate()
{
    for (S32 i = 0; i < MAX_TEMP_STREAMS; i++) {
        if (pTempStreams[i]) {
            S32 nOpenState = FSOUND_Stream_GetOpenState(pTempStreams[i]->ps.pStream);

            if (nOpenState == 0) {
                if (FSOUND_Stream_GetMode(pTempStreams[i]->ps.pStream) == 0)
                    nOpenState = -3;
            }
            if (nOpenState == -2) continue;

            if (nOpenState < 0) {
                if (nOpenState != -1)
                    iSndCloseStream(pTempStreams[i] - gSnd.voice, true);
                if (pTempStreams[i]) {
                    pTempStreams[i]->flags = 0;
                    pTempStreams[i] = NULL;
                }
                continue;
            }

            xASSERT_L(1231, nOpenState == 0);

            bool bCloseStream = false;
            if (pTempStreams[i]->flags & xSnd_ACTIVE) {
                S32 nChannel = FSOUND_Stream_PlayEx(FSOUND_FREE, pTempStreams[i]->ps.pStream, NULL, TRUE);

                if (nChannel != -1) {
                    S32 nRawChanZero = FSOUND_GetSubChannel(nChannel, 0);
                    xASSERT_L(1249, nRawChanZero > -1);

#if DEBUG
                    iprintf("pTempStream[%d] %d landed at chn:%d\n", i, pTempStreams[i]->sndID.GetID(), nRawChanZero);
#endif

                    gSnd.voice[nRawChanZero] = *pTempStreams[i];
                    gSnd.voice[nRawChanZero].ps.nChannel = nChannel;

                    xSndMgrUpdateRef(gSnd.voice[nRawChanZero].sndUniqueID, nRawChanZero);

                    iSndPlayDone(&gSnd.voice[nRawChanZero]);
                } else
                    bCloseStream = true;
            } else {
                bCloseStream = true;
            }

            if (bCloseStream) {
#if DEBUG
                iprintf("Closing pTempStream[%d] %d ", i, pTempStreams[i]->sndID.GetID());
                DetectLeakStreamClose(pTempStreams[i]->sndID.GetID());
                iprintf("\n");
#endif

                S8 closed = FSOUND_Stream_Close(pTempStreams[i]->ps.pStream);
                xASSERT_L(1276, closed);
                pTempStreams[i]->ps.pStream = NULL;
                pTempStreams[i]->ps.nChannel = -1;
                xSndMgrFreeRef(pTempStreams[i]->sndUniqueID);
            }

            pTempStreams[i]->flags = 0;
            xASSERT_L(1283, pTempStreams[i]->ps.nChannel == -1);
            pTempStreams[i] = NULL;
        }
    }

    bool bDialogActive = false;
    for (S32 i = 0; i < ISND_MAX_VOICES; i++) {
        xASSERT_L(1292, (gSnd.voice[i].assetID==NULL) ? (gSnd.voice[i].flags == NULL) :1);
        if (!(gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_PAUSED)) continue;

        xASSERT_L(1295, !(gSnd.voice[i].flags & xSnd_INAUDIBLE));
        S8 playing = FSOUND_IsPlaying(gSnd.voice[i].ps.nChannel);
        S8 paused = FSOUND_GetPaused(gSnd.voice[i].ps.nChannel);
        bool is3d = gSnd.voice[i].flags & xSnd_CALC3D;

        U32 loopMode = FSOUND_GetLoopMode(gSnd.voice[i].ps.nChannel);
        bool islooped = loopMode && loopMode != FSOUND_LOOP_OFF;

        if (!playing && !paused) {
            if ((gSnd.voice[i].flags & xSnd_CUTSCENE) || gSnd.voice[i].pSndFileInfo->isStreamed()) {
                xASSERT_L(1314, gSnd.voice[i].ps.pStream);
                iSndCloseStream(i, true);
            }
            else if (islooped) {
                if (xglobals->fromLauncher) xFAIL_MSG_L(1321, "Yes, we can detect stolen voices");
                if (gSnd.voice[i].pSndFileInfo->isStreamed()) {
                    xFAIL_MSG_L(1324, "Please try not to drop streams, it's bad.");
                }

                bool bGoneForever = xSndMgr_SetInaudible(&gSnd.voice[i]);
                if (xglobals->fromLauncher) xASSERT_L(1328, !bGoneForever);
                gSnd.voice[i].ps.nChannel = -1;
                gSnd.voice[i].flags = 0;
                xSndMgrFreeRef(gSnd.voice[i].sndUniqueID);
            } else {
                gSnd.voice[i].ps.nChannel = -1;
                gSnd.voice[i].flags = 0;
                xSndMgrFreeRef(gSnd.voice[i].sndUniqueID);
            }
        } else {
            if (is3d) {
                xVec3 vPosInvX = gSnd.voice[i].actualPos;
                vPosInvX.x = -vPosInvX.x;
                S8 bOK = FSOUND_3D_SetAttributes(gSnd.voice[i].ps.nChannel, (F32*)&vPosInvX, NULL);
                xASSERT_L(1346, bOK);

                if (gSnd.voice[i].flags & xSnd_DOPPLER) {
                    S32 nSampleFreq;
                    FSOUND_SAMPLE* pSample = FSOUND_GetCurrentSample(gSnd.voice[i].ps.nChannel);
                    xASSERT_L(1352, pSample);
                    FSOUND_Sample_GetDefaults(pSample, &nSampleFreq, NULL, NULL, NULL);
                    F32 iShiftedFreq = xSndFreqFromXPitch(gSnd.voice[i].xpitch, nSampleFreq);
                    S32 iDoppleredFreq = iShiftedFreq * gSnd.voice[i].dopplerFreqCoef;
                    FSOUND_SetFrequency(gSnd.voice[i].ps.nChannel, iDoppleredFreq);
                }
            }

            iSndSetVol(&gSnd.voice[i], gSnd.voice[i].vol);
            bDialogActive |= (gSnd.voice[i].category == xSndCat_DIALOG) | (gSnd.voice[i].category == xSndCat_DIALOG_3D);
        }
    }
    xSndMgrSetDialogPlaying(bDialogActive);

    xVec3 vListenerPosInvX = gSnd.pos;
    vListenerPosInvX.x = -vListenerPosInvX.x;
    FSOUND_3D_Listener_SetAttributes((F32*)&vListenerPosInvX, NULL, -gSnd.at.x, gSnd.at.y, gSnd.at.z, -gSnd.up.x,
                                     gSnd.up.y, gSnd.up.z);
    FSOUND_Update();

    FSOUND_GetMemoryStats(&gCurrSoundMemoryUsed, &gHighwaterSoundMemoryUsed);

    xASSERT_FMT_L(1374, (gHighwaterSoundMemoryUsed < ISND_STATIC_BUFFER_SIZE), "Sound memory limit exceeded");
}

bool iSndIsReady(xSndHandle)
{
    return true;
}

void iSndStop(xSndVoiceInfo* vp, bool bFreeRef)
{
    S32 nRawChanZeroCheck = vp - gSnd.voice;

    xASSERT_L(1402, vp);

    if ((vp->flags & xSnd_CUTSCENE) || vp->pSndFileInfo->isStreamed()) {
        iSndCloseStream(vp - gSnd.voice, bFreeRef);
    }
    else if (vp->ps.nChannel == -1) {
        vp->flags = 0;
        if (bFreeRef)
            xSndMgrFreeRef(vp->sndUniqueID);
    } else {
        xASSERT_L(1419, vp->ps.nChannel > -1);
        if (!FSOUND_IsPlaying(vp->ps.nChannel)) {
            S8 paused = FSOUND_GetPaused(vp->ps.nChannel);
            U32 loopMode = FSOUND_GetLoopMode(vp->ps.nChannel);
            bool islooped = loopMode && loopMode != FSOUND_LOOP_OFF;
            xASSERT_L(1426, vp->flags & xSnd_ACTIVE);
        } else {
            S32 nCountChannels = FSOUND_GetNumSubChannels(vp->ps.nChannel);
            xASSERT_L(1437, nCountChannels!= -1);
            S32 nRawChanZero = FSOUND_GetSubChannel(vp->ps.nChannel, 0);

            if (nRawChanZero == -1) {
#if DEBUG
                for (S32 i = 0; i < ISND_MAX_VOICES; i++) {
                    if (vp->sndUniqueID == gSnd.voice[i].sndUniqueID) {
                        xASSERT_L(1447, !(gSnd.voice[i].flags & xSnd_ACTIVE));
                    }
                }
#endif
            } else {
                xASSERT_L(1454, &gSnd.voice[nRawChanZero] == vp);
                FSOUND_StopSound(vp->ps.nChannel);
            }
            xASSERT_L(1457, !FSOUND_IsPlaying(vp->ps.nChannel));
        }
        vp->ps.nChannel = -1;
        vp->flags = 0;
        if (bFreeRef) {
            xSndMgrFreeRef(vp->sndUniqueID);
        }
    }
}

void iSndPause(xSndVoiceInfo* vp, U32 paused)
{
    xASSERT_L(1473, vp);

    FSOUND_SetPaused(vp->ps.nChannel, !!paused);
}

void iSndSetVol(xSndVoiceInfo* vp, F32 xVolume)
{
    xASSERT_L(1481, vp);

    F32 fVolume = xVolume * xSndMixerGetOutputVolume(vp->category);

    U8 iVolume = iSndVolFromX(fVolume);

    if (vp->flags & xSnd_PANRIGHT)
        FSOUND_SetPan(vp->ps.nChannel, 255);
    else if (vp->flags & xSnd_PANLEFT)
        FSOUND_SetPan(vp->ps.nChannel, 0);

    FSOUND_SetVolume(vp->ps.nChannel, iVolume);
}

void iSndSetPitch(xSndVoiceInfo* pVoice, F32 xpitch)
{
    xASSERT_L(1500, pVoice);

    S32 nSampleFreq;
    FSOUND_SAMPLE* pSample = FSOUND_GetCurrentSample(pVoice->ps.nChannel);
    xASSERT_L(1504, pSample);
    FSOUND_Sample_GetDefaults(pSample, &nSampleFreq, NULL, NULL, NULL);
    F32 fShiftedFreq = xSndFreqFromXPitch(xpitch, nSampleFreq);
    S32 iDoppleredFreq = fShiftedFreq * pVoice->dopplerFreqCoef;
    FSOUND_SetFrequency(pVoice->ps.nChannel, iDoppleredFreq);
}

void iSndKill()
{
    bool oneOpen;

    iprintf("iSndKill: Stopping all sounds...\n");
    xSndMgrStopSounds(xSndCat_ALL, false, true);

    do {
        iSndUpdate();

        oneOpen = false;
        for (U32 i = 0; i < MAX_TEMP_STREAMS; i++) {
            if (pTempStreams[i]) {
                oneOpen = true;
                break;
            }
        }
    } while (oneOpen);

    iprintf("iSndKill: Closing FMOD...\n");
    FSOUND_Close();
}

void iSndSetExternalCallback(void*)
{
}

// UNUSED
void iSndDelayFrame()
{
}

void iSndSuspendCD(U32)
{
}

void iSndSuspend()
{
    for (S16 i = 0; i < xSndCat_LAST_CATEGORY + 1; i++) {
        sOldPauseState[i] = xSndMgrIsPaused(i);
    }
    xSndMgrPauseSounds(xSndCat_ALL, true, true);
    xSndMgrUpdate(0.0f);
}

void iSndResume()
{
    for (S16 i = 0; i < xSndCat_LAST_CATEGORY + 1; i++) {
        if (!sOldPauseState[i]) {
            xSndMgrPauseSounds(i, false, true);
        }
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("ear at x%3f y%3f z%3f\n", "v%2d: %s", "x%3f y%3f z%3f  ", "inner %3f outer %3f", "\n")
#endif
