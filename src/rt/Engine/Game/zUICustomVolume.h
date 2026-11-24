#ifndef ZUICUSTOMVOLUME_H
#define ZUICUSTOMVOLUME_H

#include "xSndMgr.h"
#include "zUICustom.h"

class zUIImage;

class zUICustomVolume : public zUICustom
{
public:
    zUICustomVolume(zUI* ui, xSndMixerFaderName sndMixerFader);

    virtual void Setup();
    virtual bool PreRender();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif

    void RefreshBars();
    void HideBars();
    F32 GetVolume() const;
    void SetVolume(F32 volume);

private:
    xSndMixerFaderName sndMixerFader;
    S32 numItems;
    zUIImage** bars;
};

extern zUICustomVolume* pUICustomMusicVolumeManager;
extern zUICustomVolume* pUICustomSFXVolumeManager;

#endif
