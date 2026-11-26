#include "xCommon.h"
#include "zUICustomArtworkManager.h"

#include "xDebugTweak.h"
#include "xGroup.h"
#include "xIni.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xTextureManager.h"
#include "zBase.h"
#include "zEvent.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIImage.h"
#include "zUIText.h"

#include <stdio.h>
#include <string.h>

#define TOTAL_BONUS ((U32)80)

enum
{
    TOTAL_BONUS_GROUPS = 8
};

const char* STR_BONUS_PICKUP_GROUP_PREFIX = "GALLERY_ITEMS_FOR_";

zUICustomArtworkManager* pUICustomArtworkManager;

namespace {
xTWEAKABLE F32 ARTWORK_SCROLL_TIMER = 0.5f;
bool unlockedBonus[TOTAL_BONUS];
U32 unlockBonusGroups[TOTAL_BONUS_GROUPS];
}

// UNUSED: needed to match (code from RotU GC)
void zUIArtworkSetBonusGroups(xIniFile* ini)
{
    for (U32 i = 0; i < TOTAL_BONUS_GROUPS; i++) {
        char buf[256];
        sprintf(buf, "%s%d", STR_BONUS_PICKUP_GROUP_PREFIX, i + 1);
        unlockBonusGroups[i] = xIniGetInt(ini, buf, 0);
    }
}

void zUIIncrediblesSetUnlockedBonus(U32 i, bool value)
{
    unlockedBonus[i] = value;
}

bool zUIIncrediblesGetUnlockedBonus(U32 i)
{
    return unlockedBonus[i];
}

U32 zUIIncrediblesGet_TOTAL_BONUS()
{
    return TOTAL_BONUS;
}

zUICustomArtworkManager::zUICustomArtworkManager(zUI* ui) : zUICustom(ui)
{
}

// UNUSED: needed to match (code from RotU GC)
void zUIIncrediblesUnlockBonus(U32 groupIdx)
{
    xASSERT(groupIdx > 0 && groupIdx <= TOTAL_BONUS_GROUPS);
    for (U32 i = 0; i < unlockBonusGroups[groupIdx - 1]; i++)
        unlockedBonus[i] = true;
}

void zUICustomArtworkManager::Init()
{
    xGroup* group = (xGroup*)zSceneFindObject(ui->asset->customWidget);
    xASSERT_L(81, group != NULL && group->baseType == eBaseTypeGroup);

    items = NULL;
    control = NULL;
    bigControl = NULL;
    bigTextControl = NULL;
    nextDescription = NULL;

    numItems = 0;
    for (U32 i = 0; i < xGroupGetCount(group); i++) {
        xBase* b = xGroupGetItemPtr(group, i);
        if (b == NULL)
            continue;

        if (b->baseType == eBaseTypeUIText) {
            bigTextControl = (zUIText*)b;
            continue;
        }

        xASSERT_L(105, b->baseType == eBaseTypeUIImage);

        zUIImage* ui = (zUIImage*)b;
        xASSERT_L(109, ui->asset->type == xStrHash("ui:image"));

        if (ui->asset->customWidget == 1) {
            control = ui;
            continue;
        }
        if (ui->asset->customWidget == 2) {
            bigControl = ui;
            continue;
        }

        numItems++;
    }

    xASSERT_L(132, control != NULL);

    xASSERT_L(135, numItems != 0);
    items = (zUIImage**)zUIAllocStaticMemory(numItems * sizeof(zUIImage*));

    S32 itemIterator = 0;
    for (U32 i = 0; i < xGroupGetCount(group); i++) {
        xBase* b = xGroupGetItemPtr(group, i);
        if (b == NULL || b == control || b == bigControl || b->baseType == eBaseTypeUIText)
            continue;

        zUIImage* image = (zUIImage*)b;
        items[itemIterator++] = image;
    }
    xASSERT_L(150, itemIterator == numItems);

    selectedArtwork = 0;
    displayIndex = 0;
    activeItem = 0;
    timeBeforeScroll = 0.0f;
    delayScroll = false;

    for (U32 i = 0; i < TOTAL_BONUS; i++)
        unlockedBonus[i] = false;
}

bool zUICustomArtworkManager::HandleEvent(xBase* from, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventInvisible:
    case eEventUISignalDeactivateScreen:
    case eEventUISignalDeactivatedScreen:
        timeBeforeScroll = 0.0f;
        delayScroll = false;
        nextDescription = NULL;
        xTextureManager::Get().FreeMemory();
        break;

    case eEventUISignalActivateScreen:
        selectedArtwork = 0;
        activeItem = 0;
        displayIndex = 0;
        timeBeforeScroll = 0.0f;
        delayScroll = false;
        break;

    case eEventUISelect:
        for (S32 i = 0; i < numItems; i++) {
            if (from != items[i])
                continue;

            selectedArtwork = displayIndex + i;

            char buffer[32], textBuffer[48];
            strcpy(buffer, "UI BONUS ");
            AddArtworkIndex(buffer, selectedArtwork);
            strcpy(textBuffer, "UI TXT BONUS DESCRIPTION ");
            AddArtworkIndex(textBuffer, selectedArtwork);
            activeItem = i;

            control->SetTexture(xStrHash(buffer), false);
            if (bigControl)
                bigControl->SetTexture(xStrHash(buffer), false);
            if (bigTextControl) {
                nextDescription = xTextFindString(textBuffer, NULL);
                if (!nextDescription)
                    nextDescription = "";
                lastDescriptionValid = nextDescription[0] != '\0';
            }
            return false;
        }

        UpdateImages();

        items[activeItem]->HandleEvent(ui, eEventUIFocusOn_Select, NULL, NULL, 0);
        items[activeItem]->Signal(eEventUIFocusOn_Select);
        break;

    case eEventUISignalUp:
        if (timeBeforeScroll > 0.0f) {
            lastScrollUp = true;
            delayScroll = true;
            break;
        }

        timeBeforeScroll = ARTWORK_SCROLL_TIMER;

        if (displayIndex > 0) {
            displayIndex--;
            UpdateImages();

            items[activeItem]->HandleEvent(ui, eEventUIFocusOn_Select, NULL, NULL, 0);
            items[activeItem]->Signal(eEventUIFocusOn_Select);

            items[activeItem]->Signal(eEventSuccess);
        } else
            items[activeItem]->Signal(eEventFailure);
        break;

    case eEventUISignalDown:
        if (timeBeforeScroll > 0.0f) {
            lastScrollUp = false;
            delayScroll = true;
            break;
        }

        timeBeforeScroll = ARTWORK_SCROLL_TIMER;

        if (displayIndex + numItems < TOTAL_BONUS) {
            displayIndex++;
            UpdateImages();

            items[activeItem]->HandleEvent(ui, eEventUIFocusOn_Select, NULL, NULL, 0);
            items[activeItem]->Signal(eEventUIFocusOn_Select);

            items[activeItem]->Signal(eEventSuccess);
        } else
            items[activeItem]->Signal(eEventFailure);
        break;

    case eEventUISignalEnlargeWindow:
        if (bigTextControl)
            bigTextControl->Signal(lastDescriptionValid ? eEventSuccess : eEventFailure);
    }

    return true;
}

void zUICustomArtworkManager::PreUpdate(F32 dt)
{
    if (timeBeforeScroll > 0.0f) {
        timeBeforeScroll -= dt;
        if (timeBeforeScroll <= 0.0f) {
            timeBeforeScroll = 0.0f;
            if (delayScroll && items[0]->visible) {
                HandleEvent(NULL, lastScrollUp ? eEventUISignalUp : eEventUISignalDown, NULL, NULL, 0);
                delayScroll = false;
            }
        }
    }

    if (nextDescription && control->GetTexture().IsReady()) {
        bigTextControl->SetText(nextDescription);
        if (bigControl && bigControl->visible)
            bigTextControl->Signal(lastDescriptionValid ? eEventSuccess : eEventFailure);
        nextDescription = NULL;
    }
}

#if DEBUG || RELEASE
void zUICustomArtworkManager::AddTweaks(const char* baseName)
{
    char buffer[128];

    sprintf(buffer, "%s|ARTWORK_SCROLL_TIMER", baseName);
    xTWEAK(buffer, &ARTWORK_SCROLL_TIMER, 0.0f, 10000.0f, NULL, NULL, 0x2);
}
#endif

void zUICustomArtworkManager::UpdateImages()
{
    for (S32 i = 0; i < numItems; i++) {
        S32 realItem = i + displayIndex;
        char buffer[32];

        strcpy(buffer, "UI BONUS THUMB ");
        AddArtworkIndex(buffer, realItem);

        items[i]->SetTexture(xStrHash(buffer), false);
    }

    ui->Signal(displayIndex > 0 ? eEventUISignalLess : eEventUISignalNoLess);
    ui->Signal(displayIndex + numItems < TOTAL_BONUS ? eEventUISignalMore : eEventUISignalNoMore);
}

void zUICustomArtworkManager::AddArtworkIndex(char* buffer, S32 index)
{
    static S32 translationTable[TOTAL_BONUS] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
        71, 72, 73, 74, 75, 76, 77, 78, 79, 80
    };

    buffer += strlen(buffer);

    S32 realIndex = translationTable[index], unlockIndex = index;
    if (unlockedBonus[unlockIndex])
        sprintf(buffer, "%d", realIndex);
    else
        sprintf(buffer, "NO %d", index % 2 + 1);
}
