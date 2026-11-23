#include "xCommon.h"
#include "zNPCOneLiner.h"

#include "xCam.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "zConversation.h"
#include "zGlobals.h"
#include "zNPCCommon.h"

#define k_NPC_ONE_LINER_MAX 32

struct zNPCOneLiner
{
    zNPC::zNPCCommon* source;
    S32 priority;
    ezNPCOneLinerType type;
    xSndGroupHandle soundGroup;
};

struct zNPCOneLinerSystem
{
    zNPCOneLiner line[k_NPC_ONE_LINER_MAX];
    S32 total;
};

struct zNPCOneLinerExternalSource
{
    zNPC::zNPCCommon* source;
    xSndGroupHandle soundGroup;
    F32 timeLeft;
};

static F32 sTimeSinceLastOneLiner;
static zNPCOneLinerSystem sInternal;
static zNPCOneLinerSystem sExternal;
static zNPCOneLinerExternalSource sExternalSource;
static xSndGroupHandle sLastPlayed;

static void _ResetList(zNPCOneLinerSystem* ol)
{
    ol->total = 0;
}

static void _AddOneLiner(zNPCOneLinerSystem* ols, zNPC::zNPCCommon* inSource, ezNPCOneLinerType inType, S32 inPriority, xSndGroupHandle inSoundGroup)
{
    xASSERT_L(43, ols->total < k_NPC_ONE_LINER_MAX);
    zNPCOneLiner* ol = &ols->line[ols->total++];
    ol->source = inSource;
    ol->priority = inPriority;
    ol->type = inType;
    ol->soundGroup = inSoundGroup;
}

void zNPCOneLinerSystemPreUpdate(F32 dt)
{
    _ResetList(&sInternal);

    sExternalSource.timeLeft -= dt;
    if (sExternalSource.timeLeft <= 0.0f)
        sExternalSource.timeLeft = 0.0f;

    _ResetList(&sExternal);

    if (sExternalSource.timeLeft > 0.0f)
        _AddOneLiner(&sExternal, sExternalSource.source, ezOLT_External, ezOLP_Highest, sExternalSource.soundGroup);
}

void zNPCOneLinerSystemPostUpdate(F32 dt)
{
    if (zConversationIsConversing())
        return;

    sTimeSinceLastOneLiner -= dt;
    if (sTimeSinceLastOneLiner <= 0.0f) {
        if (sExternal.total > 0) {
            sTimeSinceLastOneLiner = xrand_RandomFloatRange(50.0f, 70.0f);
            sExternalSource.timeLeft = 0.0f;
            // BUG: reads sInternal, not sExternal
            sLastPlayed = sInternal.line[0].soundGroup;
            if (sExternal.line[0].source)
                xSndMgrPlay(sLastPlayed, 0, &sExternal.line[0].source->frame->mat.pos, NULL, sExternal.line[0].source, NULL, NULL);
            else
                xSndMgrPlay(sLastPlayed, 0, &globals.cam->mat.pos, NULL, sExternal.line[0].source, NULL, NULL);
        }
        else if (sInternal.total > 0) {
            if (sLastPlayed == ISND_INVALID_SND_GROUP_HANDLE || !xSndMgrIsPlaying(sLastPlayed)) {
                sTimeSinceLastOneLiner = xrand_RandomFloatRange(50.0f, 70.0f);
                sLastPlayed = sInternal.line[0].soundGroup;
                xSndMgrPlay(sLastPlayed, 0, &sInternal.line[0].source->frame->mat.pos, NULL, sInternal.line[0].source, NULL, NULL);
            }
        }
    }
}

void zNPCOneLinerPlayInternal(zNPC::zNPCCommon* inSource, ezNPCOneLinerType inType, ezNPCOneLinerPriority inPriority)
{
    xASSERT_L(135, inSource);
    xSndGroupHandle soundHandle = inSource->GetOneLinerSoundHandle(inType);
    if (soundHandle != ISND_INVALID_SND_GROUP_HANDLE)
        _AddOneLiner(&sInternal, inSource, inType, inPriority, soundHandle);
}

void zNPCOneLinerPlayExternal(zNPC::zNPCCommon* inSource, xSndGroupHandle inSoundGroup, F32 inTime)
{
    sExternalSource.source = inSource;
    sExternalSource.soundGroup = inSoundGroup;
    sExternalSource.timeLeft = inTime;
}

void zNPCOneLinerSceneReset()
{
    sTimeSinceLastOneLiner = xrand_RandomFloatRange(0.0f, 70.0f);
    sExternalSource.timeLeft = 0.0f;
    sLastPlayed = ISND_INVALID_SND_GROUP_HANDLE;
}
