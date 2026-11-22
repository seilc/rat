#include "xCommon.h"
#include "zCombatSystem.h"

#include "containers.h"
#include "xCam.h"
#include "xPtankPool.h"
#include "zCombat.h"
#include "zGlobals.h"

enum
{
    MAX_COMBAT = 96
};

namespace {
xEnt* combatList[MAX_COMBAT];
U32 currentCombat;
} // namespace

void zCombatSystemInit()
{
    zCombatSystemReset();
}

void zCombatSystemReset()
{
    currentCombat = 0;
    globals.boundUpdateTime = 0;
}

void zCombatSystemFrameStart()
{
    currentCombat = 0;
}

void zCombatSystemUpdate(F32 dt)
{
    if (currentCombat == 0) {
        return;
    }

    globals.boundUpdateTime++;

    for (U32 i = 0; i < currentCombat; i++) {
        xEnt* ent = combatList[i];
        zCombat* combat = zCombatGetFrom(ent);

        combat->PostUpdate(ent, dt);
    }
}

void zCombatSystemRenderEffects()
{
    if (currentCombat == 0) {
        return;
    }

    for (U32 i = 0; i < currentCombat; i++) {
        xEnt* ent = combatList[i];
        zCombat* combat = zCombatGetFrom(ent);

        combat->RenderAfterEffect(ent);
    }
}

void zCombatSystemUpdateEntity(xEnt* ent, F32 dt)
{
    if (!ent->model->Anim || !ent->model->Anim->Single->State) {
        return;
    }

    zCombat* combat = zCombatGetFrom(ent);
    xASSERT_L(97, combat != NULL);

    combat->PreUpdate(ent, dt);

    xASSERT_L(103, currentCombat < MAX_COMBAT);

    if (currentCombat < MAX_COMBAT)
        combatList[currentCombat++] = ent;
}
