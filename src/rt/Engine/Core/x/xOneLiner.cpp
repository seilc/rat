#include "xOneLiner.h"

#include "xDebug.h"
#include "xDebugTweak.h"
#include "iTime.h"
#include "zUIIncredibles.h"
#include "zConversation.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "xString.h"

xOneLiner* xOneLiner::sDelayedSound;

#if DEBUG || RELEASE
bool xOneLiner::sDebugIgnoreTime;
bool xOneLiner::sDebugIgnoreProb;
#endif

#if DEBUG || RELEASE
struct xRequestedSound
{
    xOneLiner* mSound;
    F32 mRequestTime;
    S32 mRequestID;

    inline void Init(xOneLiner* sound, S32 requestID);

    void Display(bool wasPlayed)
    {
        xprintf("%s %.4d: %s\n", wasPlayed ? "PLAYED   " : "REQUESTED", mRequestID, mSound->mAssetName);
    }
};

struct xSoundHistory
{
    S32 mNumSounds;
    S32 mTotalRequests;
    xRequestedSound mHistory[5];

    void Init()
    {
        mNumSounds = 0;
        mTotalRequests = 0;
    }

    void ScrollSounds()
    {
        if (mNumSounds > 0) {
            for (S32 i = 1; i < mNumSounds; i++)
                mHistory[i-1] = mHistory[i];
            mNumSounds--;
        }
    }

    inline void AddSound(xOneLiner* sound);

    void Age()
    {
        F32 now = iTimeGetGame();
        while (mNumSounds > 0 && now - mHistory[0].mRequestTime > 10.0f)
            ScrollSounds();
    }

    void Display()
    {
        for (S32 i = 0; i < mNumSounds; i++)
            mHistory[i].Display(false);
    }
};
#endif

#if DEBUG || RELEASE
static xSoundHistory sSndHistory;

static void OneLinerDebugModeCB()
{
    sSndHistory.Display();
}

static void OneLinerDebugInit()
{
    sSndHistory.Init();
    xDebugModeAdd("DM_ONE_LINERS", OneLinerDebugModeCB);
}
#endif

void xOneLiner::SceneInit()
{
    SceneReset();
#if DEBUG || RELEASE
    OneLinerDebugInit();
    TweaksInit();
#endif
}

void xOneLiner::SceneReset()
{
    sDelayedSound = NULL;
}

void xOneLiner::UpdateSounds()
{
#if DEBUG || RELEASE
    sSndHistory.Age();
#endif

    if (sDelayedSound && iTimeGetGame() >= sDelayedSound->mSoonestTimeToPlay) {
        xOneLiner* sound = sDelayedSound;
        sDelayedSound = NULL;
        sound->PlaySoundCore();
    }
}

void xOneLiner::TweaksInit()
{
    xTWEAK_BOOL("Player|OneLiners|Ignore cycle time", &sDebugIgnoreTime, NULL, NULL, 0);
    xTWEAK_BOOL("Player|OneLiners|Ignore probability", &sDebugIgnoreProb, NULL, NULL, 0);
}

inline bool xOneLiner::NoInhibitions() const
{
    return (HasValidHandle()
         && !(mFlags & 0x4000)
         && !xSndMgrIsDialogPlaying()
         && !zConversationIsConversing());
}

inline bool xOneLiner::IsProbable() const
{
#if DEBUG || RELEASE
    if (sDebugIgnoreProb) return true;
#endif

    U32 probBits = mFlags & 0x3F80;
    if (probBits == 0) return true;

    F32 prob = 0.01f * (probBits >> 7);
    return (prob > xrand_RandomFloat());
}

bool xOneLiner::PlaySoundCore()
{
    if (NoInhibitions()) {
        F32 timeNow = iTimeGetGame();
        
        F32 timeToPlay;
        if (mFlags & 0x40) {
            timeToPlay = mSoonestTimeToPlay;
        } else {
            timeToPlay = mSoonestTimeToPlay + 45.0f * zUIIncrediblesGetGameProgress();
        }

#if DEBUG || RELEASE
        if (timeNow >= timeToPlay || sDebugIgnoreTime) {
#else
        if (timeNow >= timeToPlay) {
#endif
            if (!sDelayedSound && IsProbable()) {
                zConversationPlay(mSoundGroupHdl, NULL);
                mSoonestTimeToPlay = timeNow + CalcTimeToWait();
                if (mFlags & 0x10) {
                    mFlags |= 0x4000;
                }
                return true;
            }
        } else if (mFlags & 0x20) {
            mSoonestTimeToPlay = timeNow + CalcTimeToWait();
        }
    }
    return false;
}

F32 xOneLiner::CalcTimeToWait() const
{
    return mCycleTime;
}

// RESTORED: from RotU GC
void xOneLinerList::InitWithSounds(xOneLiner* soundList, U32 maxCheck)
{
    F32 timeNow = iTimeGetGame();
    mNumSounds = 0;
    mSounds = soundList;
    while (soundList->mAssetName) {
        soundList->Init();
        mNumSounds++;
        soundList++;
    }
    xASSERT(maxCheck == 0 || maxCheck == mNumSounds);
}

inline void xOneLiner::Init()
{
    mSoonestTimeToPlay = iTimeGetGame() + xrand_RandomFloatRange(0.2f, 0.5f) * CalcTimeToWait();
    mSoundGroupHdl = mAssetName[0] ? xSndMgrGetSoundGroup(xStrHash(mAssetName)) : ISND_INVALID_SND_GROUP_HANDLE;
}

void xOneLinerList::Reset()
{
    xOneLiner* sp = mSounds;
    for (U32 i = 0; i < mNumSounds; i++, sp++) {
        sp->Reset();
    }
}

inline void xOneLiner::Reset()
{
    mFlags &= ~0x4000;
}
// END RESTORED
