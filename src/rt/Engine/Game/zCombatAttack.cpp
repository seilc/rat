#include "xCommon.h"
#include "zCombatAttack.h"

#include "xCam.h"
#include "xPtankPool.h"
#include "zPlayerContainer.h"

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

    for (S32 i = 0; i < MAX_EFFECT_BONES; i++) {
        effectBones[i].bone = -1;
    }
}
