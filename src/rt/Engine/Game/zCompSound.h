#ifndef ZCOMPSOUND_H
#define ZCOMPSOUND_H

#include "xSnd.h"
#include "zCompParameters.h"
#include "zICompSound.h"

class zCompSoundParams : public zCompParameters
{
public:
    zCompSoundParams() { SetDefaults(); }

    xSndGroupHandle loopingSoundGroup;
    xSndGroupHandle oneShotSoundGroup;
    F32 oneShotMinTime;
    F32 oneShotMaxTime;
    const xVec3* loopingSoundPosition;
    const xVec3* oneShotSoundPosition;

    virtual void SetDefaults()
    {
        loopingSoundGroup = ISND_INVALID_SND_GROUP_HANDLE;
        oneShotSoundGroup = ISND_INVALID_SND_GROUP_HANDLE;
        oneShotMinTime = 0.0f;
        oneShotMaxTime = 1.0f;
        loopingSoundPosition = NULL;
        oneShotSoundPosition = NULL;
    }
};

class zCompSound : public zICompSound
{
public:
    zCompSound();

#if DEBUG
    static void AddStaticTweaks();
#endif

    virtual void Attached();
    virtual void AllAttached();
    virtual void Detached();
#if RELEASE
    virtual void Reset() {}
#endif
    virtual void Update(xScene*, F32) {}
    virtual void SetParameters(const zCompParameters* parameters) { this->parameters = (const zCompSoundParams*)parameters; }
    virtual void PreUpdate(xScene*, F32);

private:
    const zCompSoundParams* Params() { return parameters; }

    const zCompSoundParams* parameters;
    xSndGroupHandle loopingSoundGroupPlaying;
    xSndHandle loopingSoundPlaying;
};

#endif
