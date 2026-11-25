#ifndef ZUICUSTOMMISSIONSELECT_H
#define ZUICUSTOMMISSIONSELECT_H

#include "zUICustom.h"
#include "xCounter.h"

class zUIImage;
class zUIText;

class zUICustomMissionSelect : public zUICustom
{
public:
    zUICustomMissionSelect(zUI* ui);

private:
    static const S32 MAX_BUBBLES = 14;

    xCounter* counter;
    zUIText* activeDesc;
    zUIText* inactiveDesc;
    zUIText* activeName;
    zUIText* inactiveName;
    zUIText* levelGoals;
    zUIImage* blinkyLights[MAX_BUBBLES];
    zUIImage* bubbles[MAX_BUBBLES];
    zUIImage* arrows[4];
    zUIImage* arrowGlows[4];
    zUIImage* bubblesExtra[3];
    zUIImage* bubblesExtraGlow[3];
    xBase* soundFXPing;
    xBase* soundFXDeny;
    xBase* soundFXConfirm;
    xBase* motionOutLeft;
    xBase* motionInLeft;
    xBase* motionInRight;
    xBase* motionOutRight;
    xBase* motionOutUp;
    xBase* motionInUp;
    xBase* motionInDown;
    xBase* motionOutDown;
    xBase* motionInvisible;
    xBase* motionBlink;
    S32 selectedStage;
    S32 selectedMission;
    S32 numStoryScenes;
    S32 numArenaScenes;
    char descBuff[1024];
};

#endif
