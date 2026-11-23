#include "xCommon.h"
#include "iCutscene.h"

#include "xCutscene.h"
#include "xstransvc.h"
#include "xSndMgr.h"
#include "iDebug.h"
#include "iFile.h"
#include "iModel.h"
#include "zAssetTypes.h"

#include <string.h>

U32 read_sizzze;

void iCSSoundSetup(xCutscene* csn)
{
    U32 dataIndex;
    xCutsceneData* data = (xCutsceneData*)(csn->Play + 1);
    U32 numData = csn->Play->NumData;

    xRegion eMyRegion = xSTGetLocalizationEnum();

    csn->SndAssetID[0] = csn->Info->AudioTracks[eMyRegion].uLeftSoundId;
    csn->SndAssetID[1] = csn->Info->AudioTracks[eMyRegion].uRightSoundId;
    csn->useDefaultTrack = false;
    csn->useDefaultTrackID = 0;

    if (csn->SndAssetID[0] == 0 && csn->SndAssetID[1] == 0) {
        for (S32 i = 0; i < eRegionCount; i++)
            if (csn->Info->AudioTracks[i].uLeftSoundId != 0) {
                csn->SndAssetID[0] = csn->Info->AudioTracks[i].uLeftSoundId;
                csn->SndAssetID[1] = csn->Info->AudioTracks[i].uRightSoundId;
                csn->useDefaultTrack = true;
                break;
            }
    }

    xASSERT_L(47, csn->SndNumChannel == 0);

    dataIndex = 0;
    bool bLeftFound = false;
    U32 uDefaultLeftTrack = 0, uDefaultRightTrack = 0;

    while (dataIndex < numData) {
        if (data->DataType == 5) {
            if (uDefaultLeftTrack == 0) {
                uDefaultLeftTrack = data->AssetID;
            }
            if (data->AssetID == csn->SndAssetID[0]) {
                bLeftFound = true;
            }
        }
        data = (xCutsceneData*)((U8*)data + sizeof(xCutsceneData) + ((data->ChunkSize + 15) & ~15));
        dataIndex++;
    }

    if (csn->SndAssetID[0] != 0xFFFFFFFF)
        csn->SndNumChannel++;
    if (csn->SndAssetID[1] != 0xFFFFFFFF) {
        csn->SndNumChannel++;
    }

    if (!bLeftFound) {
        csn->SndAssetID[0] = uDefaultLeftTrack;
        csn->SndAssetID[1] = 0;
        for (U32 uRegion = 0; uRegion < eRegionCount; uRegion++) {
            if (uDefaultLeftTrack == csn->Info->AudioTracks[uRegion].uLeftSoundId) {
                csn->SndAssetID[0] = csn->Info->AudioTracks[uRegion].uLeftSoundId;
                csn->SndAssetID[1] = csn->Info->AudioTracks[uRegion].uRightSoundId;
            }
        }
    }
}

void* iCSSoundGetData(U32 assetID, U32* len)
{
    xCutscene* csn = xCutscene_CurrentCutscene();
    void* retdata = NULL;
    xCutsceneData* data;
    U32 dataIndex, numData, foundChannel = 0xFFFF;
    S32 channel = -1, chunk, i;

    for (i = 0; csn->SndAssetID[i] != 0xFFFFFFFF && i < 2; i++) {
        if (csn->SndAssetID[i] == assetID) {
            channel = i;
            chunk = csn->SndChannelReq[channel];
        }
    }
    if (channel == -1) {
        return NULL;
    }

    if (!csn->Waiting && csn->Stream->ChunkIndex == chunk) {
        data = (xCutsceneData*)(csn->Stream + 1);
        numData = csn->Stream->NumData;
    } else {
        numData = csn->Play->NumData;
        data = (xCutsceneData*)(csn->Play + 1);
        if (csn->SndChannelReq[channel] != csn->Play->ChunkIndex) {
            csn->SndChannelReq[channel] = csn->Play->ChunkIndex;
        }
    }

    dataIndex = 0;
    while (dataIndex < numData) {
        if (data->DataType == 5) {
            if (data->AssetID == csn->SndAssetID[channel]) {
                foundChannel = channel;
                retdata = data + 1;
                *len = data->ChunkSize;
                break;
            }
        }
        data = (xCutsceneData*)((U8*)data + sizeof(xCutsceneData) + ((data->ChunkSize + 15) & ~15));
        dataIndex++;
    }

    if (retdata == NULL) {
        iprintf("ERROR called cutscene streaming but no data at all!\n");
        return NULL;
    }

    if (channel != foundChannel) {
        iprintf("ERROR finding sound channel:  desired (%d)  found (%d)\n", channel, foundChannel);
    }

    while ((U32)retdata & 31) {
        retdata = (U8*)retdata + 16;
        *len -= 16;
    }

    csn->SndChannelReq[channel]++;
    return retdata;
}

static void iCSAsyncReadCB(xFile* txf)
{
    S32 bytes, key;
    IFILE_READSECTOR_STATUS cdstat;

    if (!txf) {
        return;
    }

    key = txf->ps.akey;
    cdstat = iFileReadAsyncStatus(key, &bytes);
    if (cdstat == IFILE_RDSTAT_DONE) {
        iFileSeek(txf, bytes, 1);
    } else {
        xASSERT_L(177, 0);
    }

    xCutscene* xc = xCutscene_CurrentCutscene();
    xc->Waiting = FALSE;
}

U32 iCSFileOpen(xCutscene* csn)
{
    U32 headerskip = (csn->Info->HeaderSize + 0x7FF) & ~0x7FF;
    U32 filesize = csn->TimeChunkOffs[csn->Info->NumTime];
    PKRAssetTOCInfo ainfo;
    const char* filename;

    if (!xSTGetAssetInfo(csn->Info->AssetID, &ainfo)) {
        return FALSE;
    }

    xASSERT_L(194, ainfo.plus_offset == 0);
    filename = xST_xAssetID_HIPFullPath(csn->Info->AssetID);
    xASSERT_L(196, filename);
    if (iFileOpen(filename, IFILE_READ, &csn->File) == 0) {
        S32 uOffsetBytes = ainfo.sector * iFileGetSectorSize(XFILE_DRIVE_CDVD);
        uOffsetBytes += headerskip;
        iFileSeek(&csn->File, uOffsetBytes, 0);
        csn->File.ps.akey = -1;
    } else {
        return FALSE;
    }

    return TRUE;
}

void iCSFileAsyncRead(xCutscene* csn, void* dest, U32 size)
{
    xASSERT_FMT_L(221, (S32)size > 0, "%d bytes, %6.2fKB\n", size, (size + 511) / 1024.0f);
    read_sizzze = size;

    xASSERT_L(224, csn->File.ps.akey == -1);
    csn->Waiting = TRUE;
    U32* tp = (U32*)dest;
#if DEBUG
    for (S32 i = 0; i < size / 4; i++) {
        tp[i] = 0xDEADBEEF;
    }
#endif

    iFileReadAsync(&csn->File, dest, size, iCSAsyncReadCB, 0);
}

void iCSFileAsyncSkip(xCutscene* csn, U32 size)
{
    csn->Waiting = TRUE;
    iFileSeek(&csn->File, size, 1);
}

void iCSFileClose(xCutscene* csn)
{
    iFileReadStop();
    csn->Opened = FALSE;
}

S32 iCSLoadStep(xCutscene* csn)
{
    xASSERT_L(274, csn->Opened);

    if (csn->Waiting) {
        return 0;
    }

    if (csn->DataLoading >= 0) {
        U32 skipAccum = 0;
        while (csn->DataLoading < (S32)csn->Info->NumData) {
            U32 tmpSize;
            void* foundModel;

            if (csn->Data[csn->DataLoading].DataType == 6) {
                foundModel = NULL;
            } else {
                foundModel = xSTFindAsset(csn->Data[csn->DataLoading].AssetID, &tmpSize);
            }

            if (foundModel || csn->Data[csn->DataLoading].ChunkSize == 0) {
                if (!foundModel) {
                    iprintf("WARNING:  Model id 0x%08x missing!\n", csn->Data[csn->DataLoading].AssetID);
                }
                csn->Data[csn->DataLoading].DataPtr = foundModel;
                skipAccum += (csn->Data[csn->DataLoading].ChunkSize + 0x7FF) & ~0x7FF;
                csn->DataLoading++;
            } else {
                if (skipAccum) {
                    iCSFileAsyncSkip(csn, skipAccum);
                    skipAccum = 0;
                }

                if (csn->GotData) {
                    if (csn->Data[csn->DataLoading].DataType == 6) {
                        csn->Data[csn->DataLoading].DataPtr = RwMallocL(csn->Data[csn->DataLoading].ChunkSize, rwMEMHINTDUR_NADURATION, 334);
                        xASSERT_L(335, csn->Data[csn->DataLoading].DataPtr);
                        memcpy(csn->Data[csn->DataLoading].DataPtr, csn->AlignBuf, csn->Data[csn->DataLoading].ChunkSize);
                    } else {
#if DEBUG || RELEASE
                        gModelReadAssetIDDebug = csn->Data[csn->DataLoading].AssetID;
#endif
                        csn->Data[csn->DataLoading].DataPtr = iModelFileNew(csn->AlignBuf, csn->Data[csn->DataLoading].ChunkSize);
                    }

                    csn->Data[csn->DataLoading].DataType |= 0x80000000;
                    xVALIDATE_L(348, csn->Data[csn->DataLoading].DataPtr);
                    csn->DataLoading++;
                    csn->GotData = FALSE;
                    csn->Stream->ChunkIndex = -1;
                } else {
                    iCSFileAsyncRead(csn, csn->AlignBuf, (csn->Data[csn->DataLoading].ChunkSize + 0x7FF) & ~0x7FF);
                    csn->GotData = TRUE;
                    return 0;
                }
            }
        }

        if (skipAccum) {
            iCSFileAsyncSkip(csn, skipAccum);
        }
        csn->DataLoading = -1;
    }

    if (csn->DataLoading == -1) {
        if (csn->GotData) {
            iCSSoundSetup(csn);

            if (csn->SndNumChannel) {
                xSndMgrUpdate(0.0f);

                if (csn->SndAssetID[0] && csn->SndAssetID[1]) {
                    xSndGroup* pLeftGroup = xSndMgr_AllocGroup(1);
                    pLeftGroup->header.eSoundCategory = xSndCat_CUTSCENE;
                    pLeftGroup->header.uPriority = 254;
                    pLeftGroup->aSndGroupInfo[0].uSoundNameHash = csn->SndAssetID[0];
                    xSndGroupHandle leftSoundGroup = xSndMgr_GetHandle(pLeftGroup);

                    xSndGroup* pRightGroup = xSndMgr_AllocGroup(1);
                    pRightGroup->aSndGroupInfo[0].uSoundNameHash = csn->SndAssetID[1];
                    pRightGroup->header.uPriority = 254;
                    pRightGroup->header.eSoundCategory = xSndCat_CUTSCENE;
                    xSndGroupHandle rightSoundGroup = xSndMgr_GetHandle(pRightGroup);

                    csn->SndChannelReq[0] = 0;
                    csn->SndChannelReq[1] = 0;

                    csn->SndHandle[0] = xSndMgrPlay(leftSoundGroup, xSnd_CUTSCENE | xSnd_PANLEFT, NULL, NULL, NULL, NULL, NULL);
                    csn->SndHandle[1] = xSndMgrPlay(rightSoundGroup, xSnd_CUTSCENE | xSnd_PANRIGHT, NULL, NULL, NULL, NULL, NULL);
                } else {
                    xSndGroup* pLeftGroup = xSndMgr_AllocGroup(1);
                    pLeftGroup->header.eSoundCategory = xSndCat_CUTSCENE;
                    pLeftGroup->header.uPriority = 254;
                    pLeftGroup->aSndGroupInfo[0].uSoundNameHash = csn->SndAssetID[0];
                    xSndGroupHandle leftSoundGroup = xSndMgr_GetHandle(pLeftGroup);
                    csn->SndHandle[0] = xSndMgrPlay(leftSoundGroup, xSnd_CUTSCENE, NULL, NULL, NULL, NULL, NULL);
                    csn->SndHandle[1] = XSNDMGR_INVALID_SND_HANDLE;
                }
            } else {
                csn->SndHandle[0] = XSNDMGR_INVALID_SND_HANDLE;
                csn->SndHandle[1] = XSNDMGR_INVALID_SND_HANDLE;
            }

            csn->GotData = FALSE;
            csn->DataLoading = -2;
        } else {
            iCSFileAsyncRead(csn, csn->Play, csn->TimeChunkOffs[1] - csn->TimeChunkOffs[0]);
            csn->GotData = TRUE;
            return 0;
        }
    }

    if (csn->DataLoading == -2) {
        if (csn->Info->NumTime > 1) {
            iCSFileAsyncRead(csn, csn->Stream, csn->TimeChunkOffs[2] - csn->TimeChunkOffs[1]);
        }
        csn->DataLoading = -3;
    }

    for (U32 i = 0; csn->SndHandle[i] != XSNDMGR_INVALID_SND_HANDLE && i < 2; i++) {
        if (!xSndMgrIsReady(csn->SndHandle[i])) {
            return 0;
        }
    }

    csn->Ready = TRUE;
    return 1;
}
