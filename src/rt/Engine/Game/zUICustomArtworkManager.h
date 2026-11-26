#ifndef ZUICUSTOMARTWORKMANAGER_H
#define ZUICUSTOMARTWORKMANAGER_H

#include "zUICustom.h"

class zUIImage;
class zUIText;
struct xIniFile;

class zUICustomArtworkManager : public zUICustom
{
public:
    zUICustomArtworkManager(zUI* ui);

    void UnlockAll(); // UNUSED
    void LockAll(); // UNUSED

    virtual void Init();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void PreUpdate(F32 dt);
#if DEBUG || RELEASE
    virtual void AddTweaks(const char* baseName);
#endif

private:
    void UpdateImages();
    void AddArtworkIndex(char* buffer, S32 index);

    S32 numItems;
    zUIImage** items;
    zUIImage* control;
    zUIImage* bigControl;
    zUIText* bigTextControl;
    const char* nextDescription;
    S32 activeItem;
    S32 selectedArtwork;
    S32 displayIndex;
    F32 timeBeforeScroll;
    bool lastScrollUp;
    bool delayScroll;
    bool lastDescriptionValid;
};

extern zUICustomArtworkManager* pUICustomArtworkManager;

void zUIArtworkSetBonusGroups(xIniFile* ini); // UNUSED
void zUIIncrediblesSetUnlockedBonus(U32 i, bool value);
bool zUIIncrediblesGetUnlockedBonus(U32 i);
U32 zUIIncrediblesGet_TOTAL_BONUS();
void zUIIncrediblesUnlockBonus(U32 groupIdx); // UNUSED

#endif
