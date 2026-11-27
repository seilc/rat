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

    virtual void Init();
    virtual void Exit();
    virtual void Setup();
    virtual void Reset();
    virtual void PreUpdate(F32 dt);
    xBase* GetColorMotion(bool completed, bool unlocked);
    xBase* GetColorMotionScene(U32 idx);
    void GetArenasLevelCompletion(S32 i, bool& scene1Comp, bool& scene2Comp, bool& scene3Comp, bool& scene1Unlock,
                                  bool& scene2Unlock, bool& scene3Unlock);
    void SetLevelBubblesExtraState(S32 i, bool& allComplete, bool& allUnlocked, bool setExtraBubbles);
    void SetLevelImageState();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    void SetLevelGoalsState(S32 i);
    S32 countFromSelectedStage(S32 stage);
    S32 selectedStageFromCount(S32 count); // UNUSED
    S32 selectedMissionFromCount(S32 count); // UNUSED
    void FormatLevelGoals(char* buffer); // UNUSED

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
