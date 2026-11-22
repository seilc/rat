#ifndef ZCOMPLOGICANIMVIEWER_H
#define ZCOMPLOGICANIMVIEWER_H

#include "zICompLogic.h"

class zCompLogicAnimViewer : public zICompLogic
{
    F32 mCurAnimCycleDuration;
    F32 mAutoActionTimer;
    S32 mAutoActionCount;
    F32 mTimer;

    static U32 mNeedTweaks;
    static bool mActionPressed;
    static bool mIsActive;
    static U32 mAutoAction;
};

#endif
