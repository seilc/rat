#include "xCommon.h"
#include "zUICustomVolume.h"

#include "xDebugTweak.h"
#include "xGroup.h"
#include "xSndMgr.h"
#include "zBase.h"
#include "zEvent.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIImage.h"

#include <stdio.h>

xTWEAKABLE xColor VOLUME_ACTIVE_COLOR = { 0x62, 0xD8, 0xFF, 0xFF };
xTWEAKABLE F32 VOLUME_WIDTH_PERCENTAGE = 0.6f;
xTWEAKABLE F32 VOLUME_HEIGHT_PERCENTAGE = 0.3f;

zUICustomVolume* pUICustomMusicVolumeManager;
zUICustomVolume* pUICustomSFXVolumeManager;

zUICustomVolume::zUICustomVolume(zUI* ui, xSndMixerFaderName sndMixerFader) : zUICustom(ui), sndMixerFader(sndMixerFader)
{
}

void zUICustomVolume::Setup()
{
    xGroup* group = (xGroup*)zSceneFindObject(ui->asset->customWidget);
    xASSERT_L(30, group != NULL && group->baseType == eBaseTypeGroup);
    if (group == NULL) {
        return;
    }

    numItems = 0;
    for (U32 i = 0; i < xGroupGetCount(group); i++) {
        xBase* b = xGroupGetItemPtr(group, i);
        if (b)
            numItems++;
    }

    xASSERT_FMT_L(47, numItems != 0, "Couldn't find volume bars. Try repacking latest MNUI.SDF");
    if (numItems == 0) {
        return;
    }

    bars = (zUIImage**)zUIAllocStaticMemory(numItems * sizeof(zUIImage*));

    S32 itemIterator = 0;
    for (U32 i = 0; i < xGroupGetCount(group); i++) {
        xBase* b = xGroupGetItemPtr(group, i);
        if (b)
            bars[itemIterator++] = (zUIImage*)b;
    }
    xASSERT_L(63, itemIterator == numItems);
}

bool zUICustomVolume::PreRender()
{
    return false;
}

void zUICustomVolume::RefreshBars()
{
    if (bars == NULL) {
        return;
    }

    S32 volume = GetVolume() * 11.0f + 0.25f;

    for (S32 i = 0; i < numItems; i++) {
        bars[i]->HandleEvent(NULL, i < volume ? eEventVisible : eEventInvisible, NULL, NULL, 0);
    }
}

void zUICustomVolume::HideBars()
{
    if (bars == NULL) {
        return;
    }

    for (S32 i = 0; i < numItems; i++) {
        bars[i]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
    }
}

bool zUICustomVolume::HandleEvent(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventVisible:
        RefreshBars();
        break;

    case eEventInvisible:
        HideBars();
        break;

    case eEventIncrement: {
        F32 oldVol = GetVolume(), vol = oldVol;
        vol += 1.0f / 11.0f;
        if (vol > 1.0f)
            vol = 1.0f;
        SetVolume(vol);
        ui->Signal(oldVol != vol ? eEventIncrementSuccess : eEventIncrementFailed);

        RefreshBars();
        break;
    }
    case eEventDecrement: {
        F32 oldVol = GetVolume(), vol = oldVol;
        vol -= 1.0f / 11.0f;
        if (vol < 0.0f)
            vol = 0.0f;
        SetVolume(vol);
        ui->Signal(oldVol != vol ? eEventDecrementSuccess : eEventDecrementFailed);

        RefreshBars();
        break;
    }
    }

    return true;
}

#if DEBUG || RELEASE
void zUICustomVolume::AddTweaks(const char* baseName)
{
    char buffer[128];

    sprintf(buffer, "%s|Active Color", baseName);
    xTWEAK_COLOR(buffer, &VOLUME_ACTIVE_COLOR, NULL, NULL, 0x2);
    sprintf(buffer, "%s|Height Percentage", baseName);
    xTWEAK(buffer, &VOLUME_HEIGHT_PERCENTAGE, 0.0f, 1.0f, NULL, NULL, 0x2);
    // BUG: width tweak registered under the height's name
    sprintf(buffer, "%s|Height Percentage", baseName);
    xTWEAK(buffer, &VOLUME_WIDTH_PERCENTAGE, 0.0f, 1.0f, NULL, NULL, 0x2);
}
#endif

F32 zUICustomVolume::GetVolume() const
{
    return xSndMixerGetVolume(sndMixerFader);
}

void zUICustomVolume::SetVolume(F32 volume)
{
    xSndMixerSetVolume(sndMixerFader, volume);
}
