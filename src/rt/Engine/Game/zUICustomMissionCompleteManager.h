#ifndef ZUICUSTOMMISSIONCOMPLETEMANAGER_H
#define ZUICUSTOMMISSIONCOMPLETEMANAGER_H

#include "zUICustom.h"

class zUICustomMissionCompleteManager : public zUICustom
{
public:
    zUICustomMissionCompleteManager(zUI* ui);

    virtual void Init();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    void start_autosave();
    void end_autosave();
    virtual void PreUpdate(F32 dt);

protected:
    F32 preupdatetimer;
    bool saved;
};

void zUICustomMissionComplete_SetAllowPaused(bool value); // UNUSED
bool zUICustomMissionComplete_IsAllowedPaused();

#endif
