#include "xCommon.h"
#include "zConversation.h"

#include "xDebugTweak.h"
#include "xSndMgr.h"
#include "zBase.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zLightWeight.h"
#include "zPlayer.h"
#include "zScene.h"

#include "decomp.h"

namespace {

enum zConversationManagerState
{
    eConversationNone,
    eConversationFadeIn,
    eConversationPlaying,
    eConversationFadeOut,
    eConversationDone
};

xTWEAKABLE F32 dampValue = 0.1f;
xTWEAKABLE F32 dampFadeInVel = dampValue / 0.2f;
xTWEAKABLE F32 dampFadeOutVel = dampValue / 0.3f;

void EventHandlerWrapper(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                         U32 toParamWidgetID)
{
    ((zConversation*)to)->HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

class zConversationManager : public zLightweightSystemBase
{
protected:
    bool asserted;
    U32 badSoundID;
    const xBase* queuedOtherSpeaker;
    zConversation* queuedConversation;
    zConversation* currentConversation;
    xJawData* currentJawData;
    xSndHandle soundHandle;
    xVec3* currentVoicePosition;
    const xBase* otherSpeaker;
    F32 jawValue;
    F32 conversationTime;
    xJawSpeaker currentSpeaker;
    zConversationManagerState state;
    F32 curVolumeDamp;

public:
    F32 GetJaw(xJawSpeaker speaker);
    void Init();
    void Reset();
    void ClearConversation();
    bool IsConversing() { return soundHandle != XSNDMGR_INVALID_SND_HANDLE; }
    void Play(zConversation* conversation, const xBase* otherSpeakerBase);
    bool Play(xSndGroupHandle groupHandle, const xBase* otherSpeakerBase
#if DEBUG || RELEASE
              , zConversation* conversation = NULL
#endif
              );
    void UpdateConversation();
    void Update(F32 dt);
    void Stop(zConversation* conversation);
    void Stop(xJawSpeaker speaker); // UNUSED
    void Exit();
};

zConversationManager manager;

F32 zConversationManager::GetJaw(xJawSpeaker speaker)
{
    if (!currentJawData)
        return 0.0f;
    return currentSpeaker == speaker ? jawValue : 0.0f;
}

void zConversationManager::Init()
{
    zLightweightManager::RegisterLightweight(this);
    soundHandle = XSNDMGR_INVALID_SND_HANDLE;
    state = eConversationNone;
    Reset();
}

void zConversationManager::Reset()
{
    queuedConversation = NULL;
    queuedOtherSpeaker = NULL;
    ClearConversation();
    jawValue = 0.0f;
    currentSpeaker = exJawSpeaker_Count;
}

void zConversationManager::ClearConversation()
{
    if (IsConversing()) {
        xSndMgrStop(soundHandle);
    }
    soundHandle = XSNDMGR_INVALID_SND_HANDLE;
    state = eConversationNone;
    currentConversation = NULL;
    currentJawData = NULL;
    otherSpeaker = NULL;
    if (queuedConversation) {
        Play(queuedConversation, queuedOtherSpeaker);
        queuedConversation = NULL;
        queuedOtherSpeaker = NULL;
    }
}

void zConversationManager::Play(zConversation* conversation, const xBase* otherSpeakerBase)
{
    if (globals.stopCurrentConversation) {
        queuedConversation = NULL;
        queuedOtherSpeaker = NULL;
        ClearConversation();
    }

    if (IsConversing()) {
        queuedConversation = conversation;
        queuedOtherSpeaker = otherSpeakerBase;
    }
    else if (Play(conversation->GetSoundGroup(), otherSpeakerBase
#if DEBUG || RELEASE
                  , conversation
#endif
                  )) {
        currentConversation = conversation;
    }
}

bool zConversationManager::Play(xSndGroupHandle groupHandle, const xBase* otherSpeakerBase
#if DEBUG || RELEASE
                                , zConversation* conversation
#endif
                                )
{
    if (groupHandle == ISND_INVALID_SND_GROUP_HANDLE) {
        xASSERT_FMT_L(189, false, "Something is trying to play a conversation with an invalid sound group");
        return false;
    }

    xASSERT_FMT_L(193, xSndMgr_GetGroup( groupHandle )->header.eSoundCategory == xSndCat_DIALOG_3D,
                  "All conversations should be in the DIALOG_3D sound catagory. Sound group %s is not",
                  xSceneID2Name(xglobals->sceneCur, xSndMgr_GetGroup(groupHandle)->header.id));
    xASSERT_FMT_L(195, soundHandle == XSNDMGR_INVALID_SND_HANDLE,
                  "Conversations no longer interrupt each other, so you shouldn't queue one up when the previous one is playing");

    currentVoicePosition = &gSnd.pos;
    soundHandle = xSndMgrPlay(groupHandle, xSnd_FOLLOW, currentVoicePosition, NULL, NULL, NULL, NULL);
    if (soundHandle != XSNDMGR_INVALID_SND_HANDLE) {
        U32 soundID = xSndMgrGetSoundId(soundHandle);
        currentJawData = xJawData::Find(soundID);
        asserted = false;
        badSoundID = soundID;
        otherSpeaker = otherSpeakerBase;

        xASSERT_FMT_L(217, currentJawData,
                      "SOUND DEPARTMENT BUG: Conversation %s should have jaw data. Sound file %s does not. This error MUST be fixed.",
                      conversation ? xSceneID2Name(xglobals->sceneCur, conversation->id) : "",
                      xSceneID2Name(xglobals->sceneCur, soundID));

        conversationTime = 0.0f;
        curVolumeDamp = 0.0f;
        state = eConversationFadeIn;
        UpdateConversation();
        return true;
    } else {
        return false;
    }
}

void zConversationManager::UpdateConversation()
{
    if (currentJawData) {
        currentSpeaker = currentJawData->Eval(conversationTime, jawValue);
    }

    if (currentSpeaker == exJawSpeaker_Remy) {
        if (!otherSpeaker) {
            if (!asserted) {
                if (currentConversation) {
                    xASSERT_FMT_L(282, false,
                                  "SOUND DEPARTMENT BUG: Conversation \"%s\" has a third speaker and it was not provided in the play event to the converation",
                                  xSceneID2Name(xglobals->sceneCur, currentConversation->id));
                } else {
                    xASSERT_FMT_L(287, false,
                                  "SOUND DEPARTMENT BUG: Sound \"%s\" has a third speaker, but it was not provided in the function call",
                                  xSceneID2Name(xglobals->sceneCur, badSoundID));
                }
                asserted = true;
            }
        } else {
            xASSERT_FMT_L(296, otherSpeaker,
                          "There is a third speaker, but you did not supply the otherSpeaker to the conversation (usually done through the extra parameter widget when you send the play event). This WILL crash in master. I'll let it go for now.");
#if DEBUG || RELEASE
            if (otherSpeaker) {
#endif
                if (xBaseIsEntity(otherSpeaker)) {
                    currentVoicePosition = xEntGetPos((const xEnt*)otherSpeaker);
                    if (soundHandle != XSNDMGR_INVALID_SND_HANDLE) {
                        xSndMgrChangeAttachedPosition(soundHandle, currentVoicePosition);
                    }
                }
#if DEBUG || RELEASE
            }
#endif
        }
    }
    else if (currentSpeaker == exJawSpeaker_Count) {
        if (!asserted) {
            xASSERT_FMT_L(321, false, "Sound \"%s\" is using the old jaw format. Please re-generate its jaw data.",
                          xSceneID2Name(xglobals->sceneCur, badSoundID));
            asserted = true;
        }
    } else {
        currentVoicePosition = &CURPLAYERS[currentSpeaker]->frame->mat.pos;
        if (soundHandle != XSNDMGR_INVALID_SND_HANDLE)
            xSndMgrChangeAttachedPosition(soundHandle, currentVoicePosition);
    }
}

void zConversationManager::Update(F32 dt)
{
    if (soundHandle == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }

    switch (state) {
    case eConversationNone:
        break;
    case eConversationFadeIn:
        if (xSndMgrIsPlaying(soundHandle)) {
            conversationTime += dt;
            UpdateConversation();
        } else {
            state = eConversationFadeOut;
        }

        curVolumeDamp += dt * dampFadeInVel;
        if (curVolumeDamp > dampValue) {
            curVolumeDamp = dampValue;
            state = eConversationPlaying;
        }

        xSndMixerSetDampVolume(xSndMixerFader_GAME_3D, curVolumeDamp);
        break;
    case eConversationPlaying:
        if (xSndMgrIsPlaying(soundHandle)) {
            conversationTime += dt;
            UpdateConversation();
        } else {
            state = eConversationFadeOut;
        }
        break;
    case eConversationFadeOut:
        curVolumeDamp -= dt * dampFadeOutVel;
        if (curVolumeDamp < 0.0f) {
            curVolumeDamp = 0.0f;
            state = eConversationDone;
        }

        xSndMixerSetDampVolume(xSndMixerFader_GAME_3D, curVolumeDamp);
        break;
    case eConversationDone:
        ClearConversation();
        xSndMixerResetDampVolumes();
        break;
    }
}

void zConversationManager::Stop(zConversation* conversation)
{
    if (conversation == currentConversation) {
        ClearConversation();
    }
}

void zConversationManager::Exit()
{
}

} // namespace

void zConversationInit()
{
    manager.Init();
#if DEBUG || RELEASE
    xTWEAK("Sound|Dampen 3D", &dampValue, 0.0f, 1.0f, NULL, NULL, 0);
#endif
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("Invalid speaker brainType 0x%08x")
#endif

F32 zConversationGetJaw(xJawSpeaker speaker)
{
    return manager.GetJaw(speaker);
}

bool zConversationIsConversing()
{
    return manager.IsConversing();
}

void zConversationPlay(xSndGroupHandle soundGroup, const xBase* otherSpeakerBase)
{
    manager.Play(soundGroup, otherSpeakerBase);
}

zConversation::zConversation(const zConversationAsset& myAsset) : asset(myAsset)
{
    xBaseInit(this, &asset);
    eventFunc = EventHandlerWrapper;
    soundGroup = xSndMgrGetSoundGroup(myAsset.soundGroupID);

    if (linkCount) {
        const zConversationAsset* assetPtr = &myAsset;
        link = (const xLinkAsset*)(assetPtr + 1);
    } else {
        link = NULL;
    }
}

void zConversation::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(497, asset, asset_size, zConversationAsset, "audio:conversation");
    new (&data) zConversation((zConversationAsset&)asset);
}

void zConversation::HandleEvent(xBase*, U32 toEvent, const F32*, xBase* toParamWidget, U32)
{
    switch (toEvent) {
    case eEventPlay:
        xASSERT_FMT_L(506, !toParamWidget || xBaseIsEntity( toParamWidget ) || toParamWidget->baseType == eBaseTypeNPC,
                      "The last parameter to a Conversation Play event must be an entity, NPC or nothing at all");
        manager.Play(this, toParamWidget);
        break;
    case eEventStop:
        manager.Stop(this);
        break;
    default:
        break;
    }
}
