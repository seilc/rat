#ifndef ZPLAYERONELINERDATA_H
#define ZPLAYERONELINERDATA_H

#include "types.h"

struct zPlayer;

class zPlayerOneLinerData
{
public:
#if DEBUG || RELEASE
    void AddTweaks(const char* prefix);
#endif
    void AddKillCount();
    bool Damage(S16 amount);
    void Reset();
    void Update(F32);

    static const S32 DAMAGE_RECORD_MAX = 14;

protected:
    S32 PrevDamageIndex(S32 currentDamageIndex) { return (currentDamageIndex + DAMAGE_RECORD_MAX - 1) % DAMAGE_RECORD_MAX; }

public:
    void ResetKillCount() { killCount = 0; }

protected:
    S32 currentDamageIndex;
    S32 killCount;
    struct
    {
        F32 time;
        S16 damageValue;
    } damageRecord[DAMAGE_RECORD_MAX];
    zPlayer* player;
};

#endif
