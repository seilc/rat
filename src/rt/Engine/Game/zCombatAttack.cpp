#include "zCombatAttack.h"

#include <string.h>

zCombatAttack::zCombatAttack()
{
    memset(this, 0, sizeof(*this));

    source = 1;
    effect = 0;
    hitEffect = 0;

    for (S32 i = 0; i < 4; i++) {
        hitBones[i].bone = -1;
    }

    for (S32 i = 0; i < 2; i++) {
        effectBones[i].bone = -1;
    }
}
