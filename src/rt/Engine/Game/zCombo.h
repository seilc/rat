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

struct xIniFile;
struct tweak_info;

void zCombo_ParseINI(xIniFile* ini);
#if DEBUG || RELEASE
void AddPointCB(const tweak_info&);
#endif
void zCombo_Setup();
void zCombo_Add(F32 points, zComboType type);
S32 zCombo_GetPowerMult();
void zCombo_HideImmediately();
void zCombo_Paused();
void zCombo_Update(F32 dt);

#endif
