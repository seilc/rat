#include "xSndMgr.h"

#include "containers.h"
#include "iMemMgr.h"
#include "iTRC.h"
#include "xDebug.h"
#include "xDebugTweak.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xIni.h"
#include "xMemMgr.h"
#include "xPad.h"
#include "xRand.h"
#include "xString.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zBase.h"
#include "zCutsceneMgr.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zNPCCommon.h"
#include "zQuaternion.h"
#include "zScene.h"

#include "decomp.h"

#include <fmodgc.h>
#include <string.h>

#define XSND_DEFAULT_PRIORITY ((U8)128)

struct SndMixerFadersEntry
{
    xSndMixerFaderName fader;
    F32 volume;
    F32 dampVolume;
    xSndMixerFaderName groupFader;
};

SndMixerFadersEntry g_SndMixerFaders[xSndMixerFader_FADERS_END] =
{
    { xSndMixerFader_GAME_3D, 0.45f, 0.0f, xSndMixerFader_SFX_GROUP },
    { xSndMixerFader_GAME_2D, 0.58f, 0.0f, xSndMixerFader_SFX_GROUP },
    { xSndMixerFader_DIALOG, 1.0f, 0.0f, xSndMixerFader_SFX_GROUP },
    { xSndMixerFader_DIALOG_3D, 1.0f, 0.0f, xSndMixerFader_SFX_GROUP },
    { xSndMixerFader_MUSIC, 0.46f, 0.0f, xSndMixerFader_MUSIC_GROUP },
    { xSndMixerFader_CUTSCENE, 1.0f, 0.0f, xSndMixerFader_SFX_GROUP },
    { xSndMixerFader_UI, 0.32f, 0.0f, xSndMixerFader_SFX_GROUP },
    { xSndMixerFader_MUSIC_GROUP, 0.72727272f, 0.0f, xSndMixerFader_NONE },
    { xSndMixerFader_SFX_GROUP, 0.72727272f, 0.0f, xSndMixerFader_NONE }
};

xTWEAKABLE F32 xSndGlobals::speedOfSound = 200.0f;

xSndGlobals gSnd;

xSndEffect m_eCurrentEffect;

struct SndListenerSpringTableEntry
{
    eSndListenerSpringType type;
    F32 stiffness;
};

SndListenerSpringTableEntry g_SndListenerSpringTable[] =
{
    { eListenerSpringType_None, 0.0f },
    { eListenerSpringType_Soft, 8.0f },
    { eListenerSpringType_Hard, 40.0f }
};

xTWEAKABLE bool gShowSoundMessages = false;
xTWEAKABLE bool gDrawSoundDebug = false;

inline bool xSndMgr_IsInited() { return gSnd.m_uSndMgrFlags & 0x1; }

inline void xSndMgr_SetInitFlag(bool bIsInit) { bIsInit ? (gSnd.m_uSndMgrFlags |= 0x1) : (gSnd.m_uSndMgrFlags &= ~0x1); }

#if DEBUG || RELEASE
struct SoundControlList
{
    char* string;
    U32 count;
    U32* ids;
};

SoundControlList IniList[] =
{
    { "PlaySounds", 0, NULL },
    { "DontPlaySounds", 0, NULL },
    { "PlaySoundGroups", 0, NULL },
    { "DontPlaySoundGroups", 0, NULL }
};

static bool InIniList(U32 id, S32 whichList)
{
    SoundControlList* list = &IniList[whichList];
    for (U32 i = 0; i < list->count; i++) {
        if (id == list->ids[i])
            return true;
    }
    return false;
}
#endif

void xSndMgr_ParseINI(xIniFile* ini)
{
#if DEBUG || RELEASE
    SoundControlList* ctl = IniList;
    for (S32 i = 0; i < ARRAY_LENGTH(IniList); i++, ctl++) {
        if (const char* str = xIniGetString(ini, ctl->string, NULL)) {
            iprintf("SoundControl %s:\n", ctl->string);
            xASSERT_L(197, ctl->count == 0);
            xASSERT_L(198, ctl->ids == NULL);
            ctl->count = xStrTokCount(str, " ,", NULL);
            ctl->ids = (U32*)RwMallocL(ctl->count * sizeof(U32), rwMEMHINTDUR_NADURATION, 200);
            char local_str[1024];
            strcpy(local_str, str);
            char* tok = strtok(local_str, " ,");
            S32 n = 0;
            while (tok) {
                ctl->ids[n] = xStrHash(tok);
                iprintf("  %08X (%s)\n", ctl->ids[n], tok);
                tok = strtok(NULL, " ,");
                n++;
            }
            xASSERT_L(211, n == ctl->count);
        }
    }
#endif
}

namespace
{
    struct sound_fader
    {
        xSndHandle handle;
        F32 volume;
        F32 end;
        F32 rate;
    };

    linear_pool<sound_fader> faders;

    void setup_faders()
    {
        faders.create(XSND_TOTAL_VOICES, eMemMgrTag_Audio);
    }

    void reset_faders()
    {
        faders.clear();
    }

    void update_faders(F32 dt)
    {
        sound_fader* it = faders.begin(), *end = faders.end();
        while (it != end) {
            xASSERT_L(245, it->rate != 0.0f);
            it->volume += it->rate * dt;
            if (it->rate < 0.0f && it->volume < it->end) {
                xSndMgrStop(it->handle);
                faders.erase(it);
                end--;
            }
            else if (it->rate >= 0.0f && it->volume >= it->end) {
                xSndMgrSetVolume(it->handle, it->end, true);
                faders.erase(it);
                end--;
            } else {
                xSndMgrSetVolume(it->handle, it->volume, true);
                it++;
            }
        }
    }

    xSndVoiceInfo* xSndMgrGetVoice(xSndHandle& xHandle)
    {
        iSndHandle* iHandle = (iSndHandle*)&xHandle;

        S32 iVoiceRef = iHandle->GetVoiceRef();
        xASSERT_L(279, (iVoiceRef >= 0) && (iVoiceRef < XSND_TOTAL_VOICES));

        if (gSnd.voiceRefs[iVoiceRef].GetID() != iHandle->GetID()) {
            return NULL;
        }

        S32 iVoiceIndex = gSnd.voiceRefs[iVoiceRef].GetVoiceIndex();
        xASSERT_L(285, (iVoiceIndex >= 0) && (iVoiceIndex < XSND_TOTAL_VOICES));
        xSndVoiceInfo* pVoice = &gSnd.voice[iVoiceIndex];

        if (!(pVoice->flags & xSnd_ACTIVE)) {
            return NULL;
        }

        return pVoice;
    }

    sound_fader* find_fader(xSndHandle handle)
    {
        sound_fader* it = faders.begin(), *end = faders.end();
        for (; it != end; it++) {
            if (it->handle == handle)
                return it;
        }
        return NULL;
    }
}

S32 xSndMgrGetVoiceIndex(xSndHandle& xHandle)
{
    iSndHandle* iHandle = (iSndHandle*)&xHandle;

    S32 iVoiceRef = iHandle->GetVoiceRef();
    xASSERT_L(312, (iVoiceRef >= 0) && (iVoiceRef < XSND_TOTAL_VOICES));

    if (gSnd.voiceRefs[iVoiceRef].GetID() != iHandle->GetID()) {
        return 0xFF;
    }

    S32 iVoiceIndex = gSnd.voiceRefs[iVoiceRef].GetVoiceIndex();
    xASSERT_L(318, (iVoiceIndex >= 0) && (iVoiceIndex < XSND_TOTAL_VOICES));
    return iVoiceIndex;
}

xSndGroup* xSndMgr_AllocGroup(U8 nSoundAssets)
{
    U32 datasize = sizeof(xSndGroup);
    datasize += nSoundAssets * sizeof(xSndGroupInfo);
    void* pAllocated = xMEMALLOC_L(datasize, 0, eMemMgrTag_Audio, 0, 326);
    xSndGroup* pRetGroup = (xSndGroup*)pAllocated;
    pRetGroup->header.uPlayedMask = 0;
    pRetGroup->header.uLastSelected = 0;
    pRetGroup->header.uSetBits = 0;
    pRetGroup->header.nMaxPlays = 0;
    pRetGroup->header.eSoundCategory = 0;
    pRetGroup->header.uPriority = XSND_DEFAULT_PRIORITY;
    pRetGroup->header.uFlags = 0;
    pRetGroup->header.fInnerRadius = 25.0f;
    pRetGroup->header.fOuterRadius = 100.0f;
    pRetGroup->header.pszGroupName = "";
    pRetGroup->header.uInfoCount = nSoundAssets;

    for (U32 i = 0; i < nSoundAssets; i++) {
        pRetGroup->aSndGroupInfo[i].fMaxPitchMult = 0.0f;
        pRetGroup->aSndGroupInfo[i].fMinPitchMult = 0.0f;
        pRetGroup->aSndGroupInfo[i].fVolume = 1.0f;
        pRetGroup->aSndGroupInfo[i].uSoundNameHash = 0;
    }
    return pRetGroup;
}

void xSndMgrInit()
{
    memset(&gSnd, 0, sizeof(gSnd));

    iSndInit();

    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        gSnd.voice[i].flags = 0;
        gSnd.voice[i].soundGroupHandle = ISND_INVALID_SND_GROUP_HANDLE;
        gSnd.voice[i].sndUniqueID = XSNDMGR_INVALID_SND_HANDLE;
        gSnd.voice[i].ps.nChannel = -1;
        gSnd.voiceRefs[i].SetID(0xFFFFFF);
    }

    xSndMgrSelectListenerMode(xSndListener_PLAYER);

    m_eCurrentEffect = xSndEffect_NONE;
    iSndSetEnvironmentalEffect(xSndEffect_NONE);

    xSndMgrSetListenerSpringType(eListenerSpringType_None);

#if DEBUG || RELEASE
    xTWEAK("Sound|Speed of sound in m/s", &xSndGlobals::speedOfSound, 0.0f, 10000.0f, NULL, NULL, 0);
    xTWEAK_BOOL("Debugging Options|Draw Sound Debug", &gDrawSoundDebug, NULL, NULL, 0);

    xTWEAK("Sound|CategoryVolume|Game 3D", &g_SndMixerFaders[xSndMixerFader_GAME_3D].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|Game 2D", &g_SndMixerFaders[xSndMixerFader_GAME_2D].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|Dialog", &g_SndMixerFaders[xSndMixerFader_DIALOG].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|Dialog 3D", &g_SndMixerFaders[xSndMixerFader_DIALOG_3D].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|Music", &g_SndMixerFaders[xSndMixerFader_MUSIC].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|Cutscenes", &g_SndMixerFaders[xSndMixerFader_CUTSCENE].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|UI", &g_SndMixerFaders[xSndMixerFader_UI].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|_Group Music", &g_SndMixerFaders[xSndMixerFader_MUSIC_GROUP].volume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryVolume|_Group SFX", &g_SndMixerFaders[xSndMixerFader_SFX_GROUP].volume, 0.0f, 1.0f, NULL, NULL, 0);

    xTWEAK("Sound|CategoryDamp|Game 3D", &g_SndMixerFaders[xSndMixerFader_GAME_3D].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|Game 2D", &g_SndMixerFaders[xSndMixerFader_GAME_2D].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|Dialog", &g_SndMixerFaders[xSndMixerFader_DIALOG].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|Dialog 3D", &g_SndMixerFaders[xSndMixerFader_DIALOG_3D].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|Music", &g_SndMixerFaders[xSndMixerFader_MUSIC].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|Cutscenes", &g_SndMixerFaders[xSndMixerFader_CUTSCENE].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|UI", &g_SndMixerFaders[xSndMixerFader_UI].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|_Group Music", &g_SndMixerFaders[xSndMixerFader_MUSIC_GROUP].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Sound|CategoryDamp|_Group SFX", &g_SndMixerFaders[xSndMixerFader_SFX_GROUP].dampVolume, 0.0f, 1.0f, NULL, NULL, 0);
#endif

    iSndSuspendCD(1);
}

void xSndMgrSetListenerSpringType(eSndListenerSpringType type)
{
    gSnd.sndListenerSpringType = type;
    F32 stiffness = g_SndListenerSpringTable[type].stiffness;
    gSnd.springyPos.SetTension(stiffness);
}

void xSndMgrSelectListenerMode(xSndListener Mode)
{
    gSnd.listenerMode = Mode;
}

void xSndMgrSetEffect(xSndEffect Effect)
{
    if (m_eCurrentEffect != Effect) {
        m_eCurrentEffect = Effect;
        iSndSetEnvironmentalEffect(Effect);
    }
}

xSndEffect xSndMgrGetEffect()
{
    return m_eCurrentEffect;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("ListenerType < xSndListener_MAX_TYPES")
#endif

void xSndMgrProcessSoundPosition(const xVec3* pActual, xVec3* pProcessed)
{
    switch (gSnd.listenerMode) {
    case xSndListener_PLAYER:
    {
        xVec3 playerDelta, cameraDelta;
        playerDelta.Sub(*pActual, gSnd.listenerMat[xSndListener_PLAYER].pos);
        cameraDelta.Sub(*pActual, gSnd.listenerMat[xSndListener_CAMERA].pos);
        F32 playerDist = xVec3Length(&playerDelta);
        F32 cameraDist = xVec3Length(&cameraDelta);
        if (cameraDist < playerDist) {
            F32 inwardShift = playerDist - cameraDist;
            inwardShift *= 0.5f;
            F32 factor = (playerDist - inwardShift) / playerDist;
            playerDelta *= factor;
            pProcessed->Add(playerDelta, gSnd.listenerMat[xSndListener_PLAYER].pos);
        }
        else if (xsqr(playerDelta.x) + xsqr(playerDelta.z) < 0.1f) {
            xVec3 fudgeAwayFromPlayer = cameraDelta;
            fudgeAwayFromPlayer.at_normalize();
            fudgeAwayFromPlayer *= 0.25f;
            pProcessed->Add(*pActual, fudgeAwayFromPlayer);
        } else {
            *pProcessed = *pActual;
        }
        break;
    }
    case xSndListener_CAMERA:
        *pProcessed = *pActual;
        break;
    }
}

void xSndMgr_UpdateListenerPosition(F32 dt)
{
    xMat4x3* pMat = &gSnd.listenerMat[gSnd.listenerMode];
    gSnd.right = pMat->right;
    gSnd.up = pMat->up;
    gSnd.at = pMat->at;
    gSnd.velocity = gSnd.listenerVel[gSnd.listenerMode];

    if (gSnd.sndListenerSpringType == eListenerSpringType_None) {
        gSnd.springyPos = pMat->pos;
        gSnd.springyPos.Reset();
        gSnd.pos = gSnd.springyPos;
    } else {
        gSnd.springyPos = pMat->pos;
        gSnd.springyPos.Update(dt);
        gSnd.pos = gSnd.springyPos;
    }
}

void xSndMgr_InitListenerPosition()
{
    xMat4x3* pMat = &gSnd.listenerMat[gSnd.listenerMode];
    gSnd.right = pMat->right;
    gSnd.up = pMat->up;
    gSnd.at = pMat->at;
    gSnd.velocity = gSnd.listenerVel[gSnd.listenerMode];

    gSnd.springyPos.SnapTo(pMat->pos);
    gSnd.pos = pMat->pos;
}

bool xSndMgr_UpdateVoicePosition(xSndVoiceInfo* pVoice)
{
    xASSERT_L(605, pVoice->flags & xSnd_CALC3D);
    if (pVoice->flags & xSnd_FOLLOW) {
        if (pVoice->pPosition) {
            if (pVoice->pVelocity) {
                pVoice->actualVelocity = *pVoice->pVelocity;
            }
            else if ((pVoice->flags & xSnd_DOPPLER) && !xfeq0(xglobals->update_dt)) {
                xVec3 vel;
                vel.ScaleDiff(*pVoice->pPosition, pVoice->actualPos, 1.0f / xglobals->update_dt);
                xVec3Copy(&pVoice->actualVelocity, &vel);
            }
            pVoice->actualPos = *pVoice->pPosition;
        }
        else if (pVoice->pParent) {
            xEnt* ent = pVoice->pParent;
            xASSERT_L(633, ent != NULL);
            if ((ent->baseFlags & k_XBASE_IS_ENABLED) && !xSndMgrIsPaused(pVoice->category)) {
                xVec3Copy(&pVoice->actualPos, xEntGetPos(ent));
                if ((pVoice->flags & xSnd_DOPPLER) && !xfeq0(xglobals->update_dt)) {
                    xVec3 vel;
                    vel.ScaleDiff(ent->frame->mat.pos, ent->frame->oldmat.pos, 1.0f / xglobals->update_dt);
                    if (vel.length2() > xsqr(xSndGlobals::speedOfSound)) {
                        if (ent->baseType == eBaseTypeNPC) {
                            zNPC::zNPCCommon* npc = (zNPC::zNPCCommon*)ent;
                        }
                    }
                    xVec3Copy(&pVoice->actualVelocity, &vel);
                }
            }
            else if (!(ent->baseFlags & k_XBASE_IS_ENABLED)) {
                pVoice->flags &= ~xSnd_FOLLOW;
            }
        }
    }

    xSndMgrProcessSoundPosition(&pVoice->actualPos, &pVoice->playPos);

    if (pVoice->flags & xSnd_DOPPLER) {
        xASSERT_FMT_L(686, pVoice->pVelocity || pVoice->pParent || pVoice->pPosition, "This sound is supposed to have the doppler effect, but has no way of updating it's velocity");
        xVec3 soundDir;
        soundDir.Sub(gSnd.pos, pVoice->actualPos);
        F32 length2 = soundDir.length2();
        if (!xfeq0(length2)) {
            soundDir *= xinvsqrt(length2);
            xVec3* ptr = &pVoice->actualVelocity;
            F32 velListener = gSnd.velocity.dot(soundDir);
            F32 velSource = pVoice->actualVelocity.dot(soundDir);
#if DEBUG
            if (xabs(velListener) > xSndGlobals::speedOfSound) {
                xFAIL_ONCE_L(702, "%s", "The listener has broken the sound barrier... Unfortunately, this isn't supported correctly");
            }
            if (xabs(velSource) > xSndGlobals::speedOfSound) {
                xFAIL_ONCE_L(706, "%s", "The source of the sound has broken the sound barrier... Unfortunately, this isn't supported correctly");
            }
#endif
            F32 newDoppler = (xSndGlobals::speedOfSound - velListener) / (xSndGlobals::speedOfSound - velSource);
            if (newDoppler > 0.0f)
                pVoice->dopplerFreqCoef = newDoppler;
            else
                pVoice->dopplerFreqCoef = 1000000.0f;
        }
    }

    xASSERT_L(716, pVoice->flags & xSnd_CALC3D);
    xVec3 soundDirectionXYZ;
    xVec3Sub(&soundDirectionXYZ, &pVoice->playPos, &gSnd.pos);
    pVoice->distToListener = soundDirectionXYZ.length();
    if (!(pVoice->flags & xSnd_INAUDIBLE)) {
        if (pVoice->distToListener > pVoice->outerRadius) {
            if (!pVoice->pSndFileInfo->isLooping() && pVoice->distToListener > 1.2f * pVoice->outerRadius)
                return false;
            return xSndMgr_SetInaudible(pVoice);
        }
    } else {
        xASSERT_L(735, pVoice->flags & xSnd_INAUDIBLE);
        if (pVoice->distToListener <= pVoice->outerRadius)
            return xSndMgr_SetAudible(pVoice);
    }
    return true;
}

xSndVoiceInfo* xSndMgr_GetFreeVirtualVoiceInfo(U32 priority, S32 flags, bool virtualOnly)
{
    xSndVoiceInfo* pVoice = NULL;
    for (S32 i = XSND_MAX_VOICES; i < XSND_TOTAL_VOICES; i++) {
        if (gSnd.voice[i].flags == 0)
            return &gSnd.voice[i];
    }
    if (!virtualOnly) {
        return iSndGetFreeVoiceInfo(priority, flags);
    }
    return NULL;
}

void xSndMgr_Stop(xSndVoiceInfo* pVoice, bool bFreeRef)
{
    S32 voiceId = pVoice - gSnd.voice;
    if (voiceId < XSND_MAX_VOICES) {
        iSndStop(pVoice, bFreeRef);
    } else {
        iSndStop(pVoice, bFreeRef);
    }
}

void xSndMgr_Pause(xSndVoiceInfo* pVoice, bool bPaused)
{
    S32 voiceId = pVoice - gSnd.voice;
    if (voiceId < XSND_MAX_VOICES) {
        iSndPause(pVoice, bPaused);
    }
    if (bPaused)
        pVoice->flags |= xSnd_PAUSED;
    else
        pVoice->flags &= ~xSnd_PAUSED;
}

void xSndMgr_CutscenePause(xSndVoiceInfo* pVoice, bool bPaused)
{
    if (pVoice->category != xSndCat_CUTSCENE)
        if (pVoice->pSndFileInfo && pVoice->pSndFileInfo->isLooping()) {
            xSndMgr_Pause(pVoice, bPaused);
        }
        else if (bPaused) {
            xSndMgr_Stop(pVoice, true);
        }
}

bool xSndMgr_SetInaudible(xSndVoiceInfo* pVoice)
{
    if (pVoice->pSndFileInfo->isStreamed()) {
        return true;
    }
    if (!pVoice->pSndFileInfo->isLooping()) {
        return true;
    }
    xASSERT_L(831, pVoice->flags & xSnd_ACTIVE);
    xSndVoiceInfo* pVoiceStorage = xSndMgr_GetFreeVirtualVoiceInfo(pVoice->priority, pVoice->flags, false);
    if (!pVoiceStorage || pVoiceStorage - gSnd.voice < XSND_MAX_VOICES)
        return true;
    xASSERT_L(835, pVoiceStorage);

    *pVoiceStorage = *pVoice;
    xSndMgr_Stop(pVoice, false);
    xSndMgrUpdateRef(pVoiceStorage->sndUniqueID, pVoiceStorage - gSnd.voice);
    pVoiceStorage->flags |= xSnd_INAUDIBLE;

    S32 xVoiceId = pVoice - gSnd.voice;
    S32 xNewVoiceId = pVoiceStorage - gSnd.voice;
    if (gShowSoundMessages) iprintf("%d Set Inaudible to %d\n", xVoiceId, xNewVoiceId);
    return false;
}

bool xSndMgr_SetAudible(xSndVoiceInfo* pVoiceStorage)
{
    xASSERT_L(853, pVoiceStorage->flags & xSnd_CALC3D);
    xSndVoiceInfo* vp = iSndGetFreeVoiceInfo(pVoiceStorage->priority, pVoiceStorage->flags);
    if (!vp)
        return false;
    *vp = *pVoiceStorage;
    xSndMgrUpdateRef(vp->sndUniqueID, vp - gSnd.voice);
    vp->flags &= ~xSnd_INAUDIBLE;

    bool bOk = iSndPlay(vp);
    if (bOk) {
        pVoiceStorage->flags = 0;
        if (xSndMgrIsPaused(vp->category))
            xSndMgrPause(vp->sndUniqueID, true);
    }
    return false;
}

void xSndMgrSceneInit()
{
    xASSERT_L(881, !xSndMgr_IsInited());
    xSndMgr_SetInitFlag(true);

    xSndMixerResetDampVolumes();

    xSndSelectListenerMode(xSndListener_PLAYER);

    for (S32 i = 0; i < xSndListener_MAX_TYPES; i++) {
        gSnd.listenerMat[i].at.assign(1.0f, 0.0f, 0.0f);
        gSnd.listenerMat[i].right.assign(0.0f, 1.0f, 0.0f);
        gSnd.listenerMat[i].up.assign(0.0f, 0.0f, 1.0f);
        gSnd.listenerMat[i].pos.assign(999999.0f, 999999.0f, 999999.0f);
    }
    gSnd.at.assign(1.0f, 0.0f, 0.0f);
    gSnd.right.assign(0.0f, 1.0f, 0.0f);
    gSnd.up.assign(0.0f, 0.0f, 1.0f);
    gSnd.pos.assign(999999.0f, 999999.0f, 999999.0f);
    iSndUpdate();

    setup_faders();
}

void xSndMgrUpdatePreScene()
{
    xSndMgr_InitListenerPosition();

    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if (gSnd.voice[i].flags & xSnd_ACTIVE)
            if (!(gSnd.voice[i].flags & xSnd_PAUSED))
                if (gSnd.voice[i].flags & xSnd_CALC3D) {
                    xSndMgr_UpdateVoicePosition(&gSnd.voice[i]);
                }
    }
}

void xSndMgrUpdate(F32 dt)
{
    xSndMgr_UpdateListenerPosition(dt);

    for (U32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if (gSnd.voice[i].flags & xSnd_ACTIVE)
            if (!(gSnd.voice[i].flags & xSnd_PAUSED))
                if (gSnd.voice[i].flags & xSnd_CALC3D) {
                    xSndMgr_UpdateVoicePosition(&gSnd.voice[i]);
                }
    }

    iSndUpdate();
    update_faders(xglobals->update_dt);
}

void xSndMgrSceneExit()
{
    xSndMgrStopSounds(xSndCat_ALL, false, false);

    xSndMgrUpdate(0.0f);

    iSndSceneExit();

#if DEBUG
    for (U32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        xSndVoiceInfo* pVoice = &gSnd.voice[i];
        xASSERT_L(983, (pVoice->flags & xSnd_ACTIVE) == NULL);
        xASSERT_L(984, (pVoice->flags & xSnd_INAUDIBLE) == NULL);
    }
#endif

    xSndMgr_SetPaused(xSndCat_ALL, false);
    xSndMgr_SetInitFlag(false);
}

void xSndMgrExit()
{
    iSndExit();
}

bool xSndMgrDoesEffects(S16 eSoundCategory)
{
    return eSoundCategory == xSndCat_GAME_3D || eSoundCategory == xSndCat_GAME_2D || eSoundCategory == xSndCat_DIALOG || eSoundCategory == xSndCat_DIALOG_3D;
}

void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused, bool bInReset)
{
    if (eSoundCategory == xSndCat_ALL) {
        bool done = false;
        while (!done) {
            done = true;
            for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
                if (((gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_INAUDIBLE)) && (!bSkipPaused || !(gSnd.voice[i].flags & xSnd_PAUSED))) {
                    xSndMgr_Stop(&gSnd.voice[i], true);
                    if (gSnd.voice[i].flags != 0)
                        done = false;
                }
            }
            if (!bInReset) {
                xPadUpdateAllPads(SECS_PER_VBLANK);
                iTRC_CHECK_L(1025);
            }
        }
        FSOUND_AUXFX_ShutdownEffect(0);
        reset_faders();
    }
    else if (eSoundCategory == xSndCat_ALL_BUT_MUSIC) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if (((gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_INAUDIBLE)) && (!bSkipPaused || !(gSnd.voice[i].flags & xSnd_PAUSED)))
                if (gSnd.voice[i].category != xSndCat_MUSIC)
                    xSndMgr_Stop(&gSnd.voice[i], true);
        }
    } else {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if (((gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_INAUDIBLE)) && (!bSkipPaused || !(gSnd.voice[i].flags & xSnd_PAUSED)))
                if (eSoundCategory == gSnd.voice[i].category)
                    xSndMgr_Stop(&gSnd.voice[i], true);
        }
    }
}

void xSndMgrStopPausedSounds(S16 eSoundCategory)
{
    if (eSoundCategory == xSndCat_ALL) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if (((gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_INAUDIBLE)) && (gSnd.voice[i].flags & xSnd_PAUSED)) {
                xSndMgr_Stop(&gSnd.voice[i], true);
            }
        }
        FSOUND_AUXFX_ShutdownEffect(0);
        reset_faders();
    }
    else if (eSoundCategory == xSndCat_ALL_BUT_MUSIC) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if (((gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_INAUDIBLE)) && (gSnd.voice[i].flags & xSnd_PAUSED))
                if (gSnd.voice[i].category != xSndCat_MUSIC)
                    xSndMgr_Stop(&gSnd.voice[i], true);
        }
    } else {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if (((gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_INAUDIBLE)) && (gSnd.voice[i].flags & xSnd_PAUSED))
                if (eSoundCategory == gSnd.voice[i].category)
                    xSndMgr_Stop(&gSnd.voice[i], true);
        }
    }
}

void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType)
{
    if (eSoundCategory == xSndCat_ALL) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if (gSnd.voice[i].flags & xSnd_ACTIVE) {
                xSndMgr_Pause(&gSnd.voice[i], bPaused);
            }
        }
    }
    else if (eSoundCategory == xSndCat_ALL_BUT_MUSIC) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && gSnd.voice[i].category != xSndCat_MUSIC) {
                xSndMgr_Pause(&gSnd.voice[i], bPaused);
            }
        }
    } else {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && eSoundCategory == gSnd.voice[i].category) {
                xSndMgr_Pause(&gSnd.voice[i], bPaused);
            }
        }
    }

    xSndMgr_SetPaused(eSoundCategory, bPaused && bPauseFutureSoundsOfThisType);
}

void xSndMgrCutscenePause(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType)
{
    if (eSoundCategory == xSndCat_ALL) {
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if (gSnd.voice[i].flags & xSnd_ACTIVE) {
                xSndMgr_CutscenePause(&gSnd.voice[i], bPaused);
            }
        }
    }
    else if (eSoundCategory == xSndCat_ALL_BUT_MUSIC) {
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && gSnd.voice[i].category != xSndCat_MUSIC) {
                xSndMgr_CutscenePause(&gSnd.voice[i], bPaused);
            }
        }
    } else {
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && eSoundCategory == gSnd.voice[i].category) {
                xSndMgr_CutscenePause(&gSnd.voice[i], bPaused);
            }
        }
    }

    xSndMgr_SetPaused(eSoundCategory, bPaused && bPauseFutureSoundsOfThisType);
}

void xSndMixerSetVolume(xSndMixerFaderName eMixerFader, F32 fVolume)
{
    xASSERT_L(1175, eMixerFader >= xSndMixerFader_CATEGORY_FADERS_START && eMixerFader < xSndMixerFader_FADERS_END);
    xASSERT_L(1176, fVolume <= 1.0001f && fVolume>= 0.0000f);

    g_SndMixerFaders[eMixerFader].volume = fVolume;
}

F32 xSndMixerGetVolume(xSndMixerFaderName eMixerFader)
{
    xASSERT_L(1183, eMixerFader >= xSndMixerFader_CATEGORY_FADERS_START && eMixerFader < xSndMixerFader_FADERS_END);
    xASSERT_L(1184, (g_SndMixerFaders[eMixerFader].volume <= 1.0001f) && (g_SndMixerFaders[eMixerFader].volume >= 0.0000f));

    return g_SndMixerFaders[eMixerFader].volume;
}

void xSndMixerSetDampVolume(xSndMixerFaderName eMixerFader, F32 fVolume)
{
    xASSERT_L(1192, eMixerFader >= xSndMixerFader_CATEGORY_FADERS_START && eMixerFader < xSndMixerFader_FADERS_END);
    g_SndMixerFaders[eMixerFader].dampVolume = fVolume;
}

void xSndMixerResetDampVolumes()
{
    for (S32 i = 0; i < xSndMixerFader_FADERS_END; i++) {
        g_SndMixerFaders[i].dampVolume = 0.0f;
    }
}

F32 xSndMixerGetOutputVolume(S16 eSoundCategory)
{
    xASSERT_L(1216, eSoundCategory >= xSndCat_FIRST_CATEGORY && eSoundCategory <= xSndCat_LAST_CATEGORY);

    xSndMixerFaderName eMixerFader = (xSndMixerFaderName)eSoundCategory;
    SndMixerFadersEntry* CurFader = &g_SndMixerFaders[eMixerFader];
    xASSERT_L(1221, (CurFader->volume <= 1.0001f) && (CurFader->volume >= 0.0000f));

    F32 outputVolume = CurFader->volume - CurFader->dampVolume;
    outputVolume = CLAMP(outputVolume, 0.0f, 1.0f);

    while (CurFader->groupFader != xSndMixerFader_NONE) {
        eMixerFader = CurFader->groupFader;
        CurFader = &g_SndMixerFaders[eMixerFader];
        xASSERT_L(1232, (CurFader->volume <= 1.0001f) && (CurFader->volume >= 0.0000f));

        F32 CurFaderVolume = CurFader->volume - CurFader->dampVolume;
        CurFaderVolume = CLAMP(CurFaderVolume, 0.0f, 1.0f);
        outputVolume *= CurFaderVolume;
    }
    return outputVolume;
}

// UNUSED: speculative
U32 xSndMgrGetGroupId(xSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);
    return pSndGroup->header.id;
}

U8 xSndMgrGetSoundCount(xSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);
    return pSndGroup->header.uInfoCount;
}

U32 xSndMgrGetSoundId(xSndGroupHandle hSound, U32 uIndexOfSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);
    if (uIndexOfSound < pSndGroup->header.uInfoCount) {
        return pSndGroup->aSndGroupInfo[uIndexOfSound].uSoundNameHash;
    }
    return 0;
}

xSndGroupHandle xSndMgrGetSoundGroup(U32 uSoundGroupNameHash)
{
    if (!uSoundGroupNameHash) {
        if (gShowSoundMessages) iprintf("bAD HASH\n");
        return ISND_INVALID_SND_GROUP_HANDLE;
    }
    xSndGroupHandle retHandle = (xSndGroupHandle)xSTFindAsset(uSoundGroupNameHash, NULL);
    if (!retHandle) {
        if (gShowSoundMessages) iprintf("No group by ID %X\n", uSoundGroupNameHash);
        return ISND_INVALID_SND_GROUP_HANDLE;
    }
    xSndGroup* pSndGroup = (xSndGroup*)retHandle;
    pSndGroup->header.pszGroupName = xSTAssetName(uSoundGroupNameHash);
    return retHandle;
}

bool xSndMgrIsPlaying(xSndGroupHandle hSoundGroup)
{
    if (hSoundGroup == ISND_INVALID_SND_GROUP_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound group handle queried isPlaying\n");
        return false;
    }
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hSoundGroup == gSnd.voice[i].soundGroupHandle)
            return true;
    }
    return false;
}

U8 xSndMgrGetPriority(xSndGroupHandle hSound, bool bStreaming, bool bLooping)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroupHeader* pSnd = &xSndMgr_GetGroup(hSound)->header;
    U32 uPriority = pSnd->uPriority;
    if (bStreaming) {
        uPriority = 224;
    }
    uPriority += pSnd->eSoundCategory;
    if (bLooping) {
        uPriority += 10;
    }
    if (uPriority > 255) {
        uPriority = 255;
    }
    return uPriority;
}

xSndVoiceInfo* xSndGetFreeVoiceInfo(const xSndGroupHeader* pSnd, S32 iFlags)
{
    F32 dstFurthest = 0.0f;
    S32 idxFurthest = 0;
    U32 ageOldest = 0xFFFFFFFF;
    S32 idxOldest = 0;

    if (iFlags & xSnd_STREAMED) {
        if (!iSndGetFreeStreamInfo(0)) {
            return NULL;
        }
    }

    if (pSnd->nMaxPlays > 0) {
        U8 uInstanceCount = 0;
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if (gSnd.voice[i].flags & xSnd_ACTIVE) {
                xASSERT_L(1469, gSnd.voice[i].soundGroupHandle);
                xASSERT_L(1470, gSnd.voice[i].sndUniqueID);
                if (pSnd->id == xSndMgr_GetGroup(gSnd.voice[i].soundGroupHandle)->header.id) {
                    uInstanceCount++;
                    if (pSnd->eSoundCategory == xSndCat_GAME_3D) {
                        if (gSnd.voice[i].distToListener > dstFurthest) {
                            dstFurthest = gSnd.voice[i].distToListener;
                            idxFurthest = i;
                        }
                        if (uInstanceCount == pSnd->nMaxPlays) {
                            xSndMgrStop(gSnd.voice[idxFurthest].sndUniqueID);
                            return &gSnd.voice[idxFurthest];
                        }
                    } else {
                        if (gSnd.voice[i].sndUniqueID < ageOldest) {
                            ageOldest = gSnd.voice[i].sndUniqueID;
                            idxOldest = i;
                        }
                        if (uInstanceCount == pSnd->nMaxPlays) {
                            xSndMgrStop(gSnd.voice[idxOldest].sndUniqueID);
                            return &gSnd.voice[idxOldest];
                        }
                    }
                }
            }
        }
    }

    xSndVoiceInfo* vp = iSndGetFreeVoiceInfo(pSnd->uPriority, iFlags);
    if (!vp) {
        return NULL;
    }
    return vp;
}

xSndHandle xSndMgrPlay(xSndGroupHandle hSoundToPlay, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch)
{
    xASSERT_L(1516, hSoundToPlay != xSndGroupHandle(0));

    if (hSoundToPlay == ISND_INVALID_SND_GROUP_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle attempted to play\n");
        return XSNDMGR_INVALID_SND_HANDLE;
    }

#if DEBUG || RELEASE
    if (IniList[2].count && !InIniList(xSndMgr_GetGroup(hSoundToPlay)->header.id, 2))
        return XSNDMGR_INVALID_SND_HANDLE;
    if (IniList[3].count && InIniList(xSndMgr_GetGroup(hSoundToPlay)->header.id, 3)) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }
#endif

    if (uFlags & xSnd_UNIQUE) {
        if (xSndMgrIsPlaying(hSoundToPlay)) {
            return XSNDMGR_INVALID_SND_HANDLE;
        }
    }

    S32 iFlags = uFlags & ~0x3FF;
    xSndGroupHeader* pSnd = &xSndMgr_GetGroup(hSoundToPlay)->header;
    U32 uInfoIndex = xrand_RandomChoice(pSnd->uPlayedMask, pSnd->uSetBits, pSnd->uInfoCount, (xRandomChoiceRule)pSnd->ePlayRule);
    xSndGroupInfo* pSndInfo = &((xSndGroup*)pSnd)->aSndGroupInfo[uInfoIndex];
    pSnd->uLastSelected = uInfoIndex;

#if DEBUG || RELEASE
    if (IniList[0].count && !InIniList(pSndInfo->uSoundNameHash, 0))
        return XSNDMGR_INVALID_SND_HANDLE;
    if (IniList[1].count && InIniList(pSndInfo->uSoundNameHash, 1)) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }
#endif

    iSndFileInfo* ip = iSndLookup(pSndInfo->uSoundNameHash);

    if (uFlags & xSnd_CUTSCENE) {
        iFlags |= xSnd_STREAMED;
    } else {
        if (!ip) {
            return XSNDMGR_INVALID_SND_HANDLE;
        }

        if (!ip->isStreamed())
            iFlags |= xSnd_SAMPLE;
        else
            iFlags |= xSnd_STREAMED;

        if (xSndMgrDoesEffects(pSnd->eSoundCategory) && !(uFlags & xSnd_NOEFFECTS)) {
            iFlags |= xSnd_EFFECTS;
        }
    }

    if (iFlags & xSnd_STREAMED) {
        if ((pSnd->uFlags & 0x1) || (uFlags & xSnd_LOOP)) {
            iFlags |= xSnd_LOOP;
        }
    }

    if (globals.cmgr && ip && !ip->isLooping() && globals.cmgr->csn && globals.cmgr->csn->Ready && pSnd->eSoundCategory != xSndCat_CUTSCENE) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    if (pSnd->eSoundCategory == xSndCat_CUTSCENE) {
        xSndMgrStopSounds(xSndCat_DIALOG, false, false);
        xSndMgrStopSounds(xSndCat_DIALOG_3D, false, false);
    }
    else if (pSnd->eSoundCategory == xSndCat_DIALOG || pSnd->eSoundCategory == xSndCat_DIALOG_3D) {
        xSndMgrStopSounds(xSndCat_DIALOG, true, false);
        xSndMgrStopSounds(xSndCat_DIALOG_3D, true, false);
    }

    xSndVoiceInfo* vp = xSndGetFreeVoiceInfo(pSnd, iFlags);
    if (!vp) {
#if DEBUG
        if (iFlags & xSnd_STREAMED)
            iprintf("Unable to play stream: %s\n", xSTAssetName(pSndInfo->uSoundNameHash));
#endif
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    U32 uniqueID = ++gSnd.SndCount;
    S32 voiceIndex = vp - gSnd.voice;
    S32 voiceRef = xSndMgrAllocRef(uniqueID, voiceIndex);
    if (voiceRef == -1) {
        gSnd.SndCount--;
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    vp->priority = ip ? xSndMgrGetPriority(hSoundToPlay, ip->isStreamed(), ip->isLooping()) : XSND_DEFAULT_PRIORITY;
    vp->assetVol = pSndInfo->fVolume;
    vp->assetID = pSndInfo->uSoundNameHash;
    vp->pSndFileInfo = ip;
    vp->dopplerFreqCoef = 1.0f;
    vp->category = pSnd->eSoundCategory;
    vp->sndID.SetID(uniqueID);
    vp->sndID.SetVoiceRef(voiceRef);

    vp->flags |= iFlags | xSnd_ACTIVE;
    vp->soundGroupHandle = xSndMgr_GetHandle((xSndGroup*)pSnd);

    if (!pfOverridePitch) {
        vp->xpitch = xrand_RandomFloatRange(pSndInfo->fMinPitchMult, pSndInfo->fMaxPitchMult);
    } else {
        vp->xpitch = *pfOverridePitch;
        if (iFlags & xSnd_RELATIVEPITCH) {
            vp->xpitch += xrand_RandomFloatRange(pSndInfo->fMinPitchMult, pSndInfo->fMaxPitchMult);
        }
    }

    if (!pfOverrideVolume) {
        vp->vol = pSndInfo->fVolume;
    } else {
        vp->vol = *pfOverrideVolume;
        if (iFlags & xSnd_RELATIVEVOLUME) {
            vp->vol *= pSndInfo->fVolume;
        }
    }

    if (vp->category == xSndCat_GAME_3D || vp->category == xSndCat_DIALOG_3D) {
        if (pParentEnt) {
            vp->flags |= xSnd_CALC3D;
            vp->pParent = pParentEnt;
            vp->pPosition = NULL;
            vp->actualPos = *xEntGetPos(pParentEnt);
            vp->pVelocity = NULL;
            if (vp->flags & xSnd_DOPPLER) {
                if (xglobals->update_dt > 0.01f) {
                    vp->actualVelocity.ScaleDiff(pParentEnt->frame->mat.pos, pParentEnt->frame->oldmat.pos, 1.0f / xglobals->update_dt);
                } else {
                    vp->actualVelocity.assign(0.0f);
                }
            } else {
                vp->actualVelocity.assign(0.0f);
            }

            if (vp->category == xSndCat_DIALOG_3D) {
                vp->innerRadius = 99999.0f;
                vp->outerRadius = 100000.0f;
            } else {
                vp->innerRadius = pSnd->fInnerRadius;
                vp->outerRadius = pSnd->fOuterRadius <= 0.0f ? 100000.0f : pSnd->fOuterRadius;
            }
        }
        else if (pPos_WS) {
            vp->flags |= xSnd_CALC3D;
            vp->pParent = NULL;
            if (iFlags & xSnd_STATICPOS) {
                vp->pPosition = NULL;
                vp->actualPos = *pPos_WS;
            } else {
                vp->pPosition = pPos_WS;
                vp->actualPos = *pPos_WS;
            }

            if (vp->category == xSndCat_DIALOG_3D) {
                vp->innerRadius = 99999.0f;
                vp->outerRadius = 100000.0f;
            } else {
                vp->innerRadius = pSnd->fInnerRadius;
                vp->outerRadius = pSnd->fOuterRadius <= 0.0f ? 100000.0f : pSnd->fOuterRadius;
            }

            if (pVel) {
                vp->pVelocity = pVel;
                vp->actualVelocity = *pVel;
            } else {
                vp->pVelocity = NULL;
                vp->actualVelocity.assign(0.0f);
            }
        } else {
#if DEBUG || RELEASE
            char buf[128];
            sprintf(buf, "Orphaned 3d sound '%s' needs a position!\n", pSnd->pszGroupName);
            if (gShowSoundMessages) iprintf(buf);
#endif

            vp->pParent = NULL;
            vp->pPosition = NULL;
            vp->actualPos = gSnd.pos;
            vp->actualVelocity.assign(0.0f);
            vp->pVelocity = NULL;
            vp->innerRadius = pSnd->fInnerRadius;
            vp->outerRadius = pSnd->fOuterRadius;
        }
    } else {
        xASSERT_L(1789, vp->category != xSndCat_GAME_3D);

        vp->pParent = NULL;
        vp->pPosition = NULL;
        vp->actualPos = gSnd.pos;
        vp->actualVelocity.assign(0.0f);
        vp->pVelocity = NULL;
        vp->innerRadius = pSnd->fInnerRadius;
        vp->outerRadius = pSnd->fOuterRadius;
    }

    xASSERT_FMT_L(1818, !(vp->flags & xSnd_DOPPLER) || (vp->flags & xSnd_FOLLOW), "If you are using the doppler flag, you must use the follow flag (so the sound follows your entity).");

    bool bOKtoPlay = true;
    if (vp->flags & xSnd_CALC3D) {
        bOKtoPlay = xSndMgr_UpdateVoicePosition(vp);
    }

    bool bPlaying = vp->pSndFileInfo && vp->pSndFileInfo->isLooping();

    if (bOKtoPlay) {
        bPlaying = iSndPlay(vp);
        if (xSndMgrIsPaused(vp->category))
            xSndMgrPause(vp->sndUniqueID, true);
    }

    if (bPlaying) {
        xASSERT_L(1842, vp->sndUniqueID);
    } else {
        vp->flags = 0;
        vp->ps.nChannel = -1;
        xSndMgrFreeRef(vp->sndUniqueID);
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    return vp->sndUniqueID;
}

U32 xSndMgrGetSoundId(xSndHandle& hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle queried isPlaying\n");
        return 0;
    }
    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice) {
        return voice->assetID;
    }
    return 0;
}

bool xSndMgrIsPlaying(xSndHandle& hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle queried isPlaying\n");
        return false;
    }
    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice) {
        return true;
    }
    return false;
}

bool xSndMgrIsLooping(xSndHandle& hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle queried isLooping\n");
        return false;
    }
    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice)
        return voice->pSndFileInfo->isLooping();
    return false;
}

bool xSndMgrIsStreamed(xSndHandle& hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle queried isStreamed\n");
        return false;
    }
    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice)
        return voice->pSndFileInfo->isStreamed();
    return false;
}

U32 xSndMgrIsReady(xSndHandle& hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle queried isReady\n");
        return FALSE;
    }
    return iSndIsReady(hSound);
}

void xSndMgrPause(xSndHandle& hSound, bool bPause)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle Paused\n");
        return;
    }
    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice)
        xSndMgr_Pause(voice, bPause);
}

void xSndMgrStop(xSndHandle& hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle Stop\n");
        return;
    }
    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice) {
        xSndMgr_Stop(voice, true);
    }
    hSound = XSNDMGR_INVALID_SND_HANDLE;
}

void xSndMgrSetVolume(xSndHandle& hSound, F32 fVolume, bool assetRelative)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle setVolume\n");
        return;
    }
    xASSERT_L(1955, fVolume>=0.f && fVolume<=1.f);

    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice) {
        F32 vol = fVolume;
        if (assetRelative) {
            vol *= voice->assetVol;
        }

        S32 voiceId = voice - gSnd.voice;
        xASSERT_L(1965, xSndMgrGetVoiceIndex( voice->sndUniqueID ) == voiceId);
        if (voiceId < XSND_MAX_VOICES) {
            iSndSetVol(voice, vol);
        }
        voice->vol = vol;
    }
}

// UNUSED: ported from RotU GC
void xSndMgrSetPitch(xSndHandle& hSound, F32 fPitch)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        if (gShowSoundMessages) iprintf("Invalid sound handle SetPitch\n");
        return;
    }

    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice) {
        S32 voiceId = voice - gSnd.voice;
        xASSERT(xSndMgrGetVoiceIndex( voice->sndUniqueID ) == voiceId);
        if (voiceId < XSND_MAX_VOICES) {
            iSndSetPitch(voice, fPitch);
        }
        voice->xpitch = fPitch;
    }
}

void xSndMgrStartStereo(xSndHandle hSnd00, xSndHandle hSnd01)
{
    iSndStartStereo(hSnd00, hSnd01);
}

void xSndMgrChangeAttachedPosition(xSndHandle& hSound, const xVec3* pVector)
{
    xSndVoiceInfo* voice = xSndMgrGetVoice(hSound);
    if (voice)
        voice->pPosition = pVector;
}

void xSndMgrStopChildren(xEnt* pParentEnt)
{
    for (U32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && gSnd.voice[i].pParent == pParentEnt) {
            xSndMgr_Stop(&gSnd.voice[i], true);
        }
    }
}

// UNUSED: ported from Incredibles GC
xSndHandle xSndMgrFadeOn(xSndGroupHandle hSoundToPlay, F32 fadeTime, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch)
{
    if (fadeTime <= 0.0f) {
        return xSndMgrPlay(hSoundToPlay, uFlags, pPos_WS, pVel, pParentEnt, pfOverrideVolume, pfOverridePitch);
    }

    F32 zero = 0.0f;
    xSndHandle sndUniqueID = xSndMgrPlay(hSoundToPlay, uFlags, pPos_WS, pVel, pParentEnt, &zero, pfOverridePitch);
    if (sndUniqueID == XSNDMGR_INVALID_SND_HANDLE) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    faders.push_back();
    sound_fader* fader = &faders.back();
    fader->handle = sndUniqueID;
    fader->volume = 0.0f;
    U8 sound_index = xSndMgr_GetGroup(hSoundToPlay)->header.uLastSelected;
    fader->end = pfOverrideVolume ? *pfOverrideVolume : xSndMgr_GetGroup(hSoundToPlay)->aSndGroupInfo[sound_index].fVolume;
    fader->rate = fader->end / fadeTime;
    return fader->handle;
}

void xSndMgrFade(xSndHandle& hSound, F32 fadeTime, F32 startVolume, F32 destVolume)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }

    if (fadeTime <= 0.0f) {
        xSndMgrSetVolume(hSound, destVolume, false);
        return;
    }

    sound_fader* fader = find_fader(hSound);
    if (!fader) {
        faders.push_back();
        fader = &faders.back();
    }

    fader->handle = hSound;
    fader->volume = startVolume;
    fader->end = destVolume;
    fader->rate = (fader->end - fader->volume) / fadeTime;
}

void xSndMgrFade(xSndHandle& hSound, F32 fadeTime, F32 destVolume)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }

    if (fadeTime <= 0.0f) {
        xSndMgrSetVolume(hSound, destVolume, false);
        return;
    }

    xSndVoiceInfo* vp = xSndMgrGetVoice(hSound);
    if (vp)
        xSndMgrFade(hSound, fadeTime, vp->vol, destVolume);
}

// UNUSED: ported from Incredibles GC
void xSndMgrFadeOff(xSndHandle& hSound, F32 fadeTime)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }

    sound_fader* fader = find_fader(hSound);
    xSndVoiceInfo* vp;
    F32 start;

    if (fadeTime <= 0.0f) {
        goto stop;
    }

    vp = xSndMgrGetVoice(hSound);
    if (!vp) {
        goto stop;
    }

    if (!fader) {
        faders.push_back();
        fader = &faders.back();
        start = 1.0f;
    } else {
        start = fader->end;
    }

    if (start <= 0.001f) {
        goto stop;
    }

    xASSERT(vp != 0);
    fader->handle = hSound;
    fader->volume = vp->vol;
    fader->end = 0.0f;
    fader->rate = -start / fadeTime;
    hSound = XSNDMGR_INVALID_SND_HANDLE;
    return;

stop:
    if (fader) {
        faders.erase(fader);
    }
    if (hSound != XSNDMGR_INVALID_SND_HANDLE) {
        xSndMgrStop(hSound);
    }
}

U32 gCurrSoundMemoryUsed;
U32 gHighwaterSoundMemoryUsed;
U32 gNumStreamsOpened;
U32 gNumStreamsClosed;

void xSndMgrDebugRender()
{
#if DEBUG || RELEASE
    if (xSndMgrGetEffect() == xSndEffect_CAVE)
        xprintf("Echo   ON\n");
    else
        xprintf("Echo   OFF\n");

    S32 size = OSCheckHeap(the_heap);

    xprintf("Mem Free %d\n", size);
    xprintf("Streams Opened %d Streams Closed %d\n", gNumStreamsOpened, gNumStreamsClosed);

    char channelStats[XSND_TOTAL_VOICES] = { 0 };
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if (gSnd.voice[i].flags & xSnd_ACTIVE) {
            iSndFileInfo* pSndFileInfo = iSndLookup(gSnd.voice[i].assetID);
            if (pSndFileInfo) {
                if (pSndFileInfo->isStreamed())
                    channelStats[i] = 's';
                else
                    channelStats[i] = 'm';
            } else
                channelStats[i] = 'c';
        }
        else if (gSnd.voice[i].flags == 0)
            channelStats[i] = '0';
        else
            channelStats[i] = '*';
    }

    for (S32 i = 0; i < 10; i++) {
        char buf[26] = { 0 };
        strncpy(buf, &channelStats[i * XSND_TOTAL_VOICES / 10], sizeof(buf) - 1);
        xprintf("%s", buf);
        if (i <= xSndCat_MUSIC) {
            char msg[80];
            U8 stars = xSndMixerGetOutputVolume(i) * 11.0f + 0.5f;
            switch (i) {
            case xSndCat_GAME_3D:
                sprintf(msg, " GAME3D:");
                break;
            case xSndCat_GAME_2D:
                sprintf(msg, " GAME2D:");
                break;
            case xSndCat_DIALOG:
                sprintf(msg, " DIALOG:");
                break;
            case xSndCat_DIALOG_3D:
                sprintf(msg, " DIALOG 3D:");
                break;
            case xSndCat_MUSIC:
                sprintf(msg, " MUSIC:");
                break;
            }
            while (stars--)
                strcat(msg, "*");
            strcat(msg, "\n");
            xprintf(msg);
        } else {
            xprintf("\n");
        }
    }

    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        const xSndVoiceInfo* vp = &gSnd.voice[i];
        if (vp->flags & xSnd_ACTIVE) {
            iSndFileInfo* pSndFileInfo = iSndLookup(vp->assetID);

            static char streamPrefix[] = "STR ";
            static char memoryPrefix[] = "MEM ";
            static char externalPrefix[] = "EXT ";
            char* prefix = memoryPrefix;
            if (pSndFileInfo) {
                if (pSndFileInfo->isStreamed()) {
                    prefix = streamPrefix;
                }
            } else {
                prefix = externalPrefix;
            }

            char pausedState = ' ';
            if (vp->flags & xSnd_PAUSED) {
                pausedState = 'P';
            }

            const char* name = xSTAssetName(vp->assetID);
            if (!name)
                xprintf("%s%2d %c:%08X\n", prefix, i, pausedState, vp->assetID);
            else if (vp->pParent)
                xprintf("%s%2d %c:%s  %s %3.1f\n", prefix, i, pausedState, name, zSceneGetName(vp->pParent), vp->distToListener);
            else if ((vp->flags & xSnd_CALC3D) && !(vp->flags & 0x4))
                xprintf("%s%2d %c:%s %3.1f:%3.0f\n", prefix, i, pausedState, name, vp->distToListener, vp->outerRadius);
            else
                xprintf("%s%2d %c:%s %d %1.2f\n", prefix, i, pausedState, name, vp->ps.nChannel, vp->vol);
        }
    }
    xprintf("\n");

    if (gDrawSoundDebug) {
        xSndMgrDebugRenderWorld();
    }
#endif
}

void xSndMgrDebugRenderWorld()
{
}

inline void xSndMgr_SetPaused(S16 whichCat, bool bPause)
{
    switch (whichCat) {
    case xSndCat_ALL:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x1FC) : (gSnd.m_uSndMgrFlags &= ~0x1FC);
        break;
    case xSndCat_ALL_BUT_MUSIC:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x1BC) : (gSnd.m_uSndMgrFlags &= ~0x1BC);
        break;
    case xSndCat_GAME_3D:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x4) : (gSnd.m_uSndMgrFlags &= ~0x4);
        break;
    case xSndCat_GAME_2D:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x8) : (gSnd.m_uSndMgrFlags &= ~0x8);
        break;
    case xSndCat_DIALOG:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x10) : (gSnd.m_uSndMgrFlags &= ~0x10);
        break;
    case xSndCat_DIALOG_3D:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x20) : (gSnd.m_uSndMgrFlags &= ~0x20);
        break;
    case xSndCat_MUSIC:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x40) : (gSnd.m_uSndMgrFlags &= ~0x40);
        break;
    case xSndCat_CUTSCENE:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x80) : (gSnd.m_uSndMgrFlags &= ~0x80);
        break;
    case xSndCat_UI:
        bPause ? (gSnd.m_uSndMgrFlags |= 0x100) : (gSnd.m_uSndMgrFlags &= ~0x100);
        break;
    }
}

bool xSndMgrIsPaused(S16 eSoundCategory)
{
    switch (eSoundCategory) {
    case xSndCat_ALL:
        return gSnd.m_uSndMgrFlags & 0x1FC;
    case xSndCat_ALL_BUT_MUSIC:
        return gSnd.m_uSndMgrFlags & 0x1BC;
    case xSndCat_GAME_3D:
        return gSnd.m_uSndMgrFlags & 0x4;
    case xSndCat_GAME_2D:
        return gSnd.m_uSndMgrFlags & 0x8;
    case xSndCat_DIALOG:
        return gSnd.m_uSndMgrFlags & 0x10;
    case xSndCat_DIALOG_3D:
        return gSnd.m_uSndMgrFlags & 0x20;
    case xSndCat_MUSIC:
        return gSnd.m_uSndMgrFlags & 0x40;
    case xSndCat_CUTSCENE:
        return gSnd.m_uSndMgrFlags & 0x80;
    case xSndCat_UI:
        return gSnd.m_uSndMgrFlags & 0x100;
    }
    return false;
}

S32 xSndMgrAllocRef(U32 uniqueID, S32 voiceIndex)
{
    S32 i = 0;
    S32 refIndex = -1;
    for (; i < XSND_TOTAL_REFS; i++) {
        if (gSnd.voiceRefs[i].GetID() == 0xFFFFFF) {
            if (refIndex == -1) {
                gSnd.voiceRefs[i].SetID(uniqueID);
                gSnd.voiceRefs[i].SetVoiceIndex(voiceIndex);
                refIndex = i;
            }
        }
        else if (gSnd.voiceRefs[i].GetVoiceIndex() == voiceIndex) {
            gSnd.voiceRefs[i].SetID(0xFFFFFF);
        }
    }
    return refIndex;
}

void xSndMgrFreeRef(xSndHandle& xHandle)
{
    iSndHandle* iHandle = (iSndHandle*)&xHandle;
    xASSERT_L(2474, iHandle->GetID() != XSNDMGR_INVALID_SND_HANDLE);
    S32 iVoiceRef = iHandle->GetVoiceRef();
    xASSERT_L(2476, (iVoiceRef >= 0) && (iVoiceRef < XSND_TOTAL_REFS));

    gSnd.voiceRefs[iVoiceRef].SetID(0xFFFFFF);
}

void xSndMgrUpdateRef(xSndHandle& xHandle, S32 voiceIndex)
{
    S32 i = 0;
    for (; i < XSND_TOTAL_REFS; i++) {
        if (gSnd.voiceRefs[i].GetVoiceIndex() == voiceIndex) {
            gSnd.voiceRefs[i].SetID(0xFFFFFF);
        }
    }

    iSndHandle* iHandle = (iSndHandle*)&xHandle;
    xASSERT_L(2505, iHandle->GetID() != XSNDMGR_INVALID_SND_HANDLE);
    S32 iVoiceRef = iHandle->GetVoiceRef();
    xASSERT_L(2507, (iVoiceRef >= 0) && (iVoiceRef < XSND_TOTAL_VOICES));

    gSnd.voiceRefs[iVoiceRef].SetVoiceIndex(voiceIndex);
}
