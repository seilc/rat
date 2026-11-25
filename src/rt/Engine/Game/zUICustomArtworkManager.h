#ifndef ZUICUSTOMARTWORKMANAGER_H
#define ZUICUSTOMARTWORKMANAGER_H

#include "zUICustom.h"

class zUIImage;
class zUIText;

class zUICustomArtworkManager : public zUICustom
{
public:
    zUICustomArtworkManager(zUI* ui);

private:
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

#endif
