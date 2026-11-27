#ifndef ZENTBUTTONASSET_H
#define ZENTBUTTONASSET_H

#include "types.h"

struct zEntButtonAsset
{
    U32 modelPressedInfoID;
    U32 actMethod;
    S32 initButtonState;
    S32 isReset;
    F32 resetDelay;
    U32 buttonActFlags;
};

#endif
