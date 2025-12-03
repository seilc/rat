#ifndef ZCOMBO_H
#define ZCOMBO_H

#include "types.h"

enum zComboType
{
    eComboType_NonDecreasing,
    eComboType_StandHit,
    eComboType_removeMe1,
    eComboType_JumpHit,
    eComboType_StandFierceHit,
    eComboType_RunFierceHit,
    eComboType_removeMe2,
    eComboType_JumpFierceHit,
    eComboType_Throw,
    eComboType_CrouchRoll,
    eComboType_StretchHit,
    eComboType_Count,
    eComboType_ForceSizeInt = FORCEENUMSIZEINT
};

void zCombo_Add(F32 points, zComboType type);
void zCombo_Paused();

#endif
