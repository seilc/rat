#include "xCommon.h"
#include "zCompCombat.h"

#include "xCam.h"
#include "xModel.h"
#include "xPtankPool.h"
#include "zAnimCache.h"
#include "zCombatSystem.h"
#include "zGlobals.h"
#include "zICompNPCEntity.h"

#define MAX_COMBAT_BOUNDS 16

S32 zCompCombat::Initialize()
{
    parameters = NULL;
    return combatData.Setup(NULL, MAX_COMBAT_BOUNDS);
}

void zCompCombat::Attached()
{
}

void zCompCombat::Detached()
{
    zICompNPCEntity* npcEnt = owner->GetEntityComponent();
    npcEnt->user_data = NULL;
}

void zCompCombat::AllAttached()
{
    if (parameters == NULL) {
        xFAIL_MSG_L(52, "Combat component requires zCompCombatParams to be set before use!");
        return;
    }

    zICompNPCEntity* npcEnt = owner->GetEntityComponent();
    xASSERT_FMT_L(58, npcEnt, "Combat component requires the owner to have a entity component.");
    if (npcEnt == NULL) {
        return;
    }

    xASSERT_L(63, npcEnt->user_data == NULL);

    npcEnt->user_data = &combatData;

    if (parameters->attackTableStates != NULL) {
        zAnimCacheCreateData(parameters->attackTableStates, parameters->stateCount, npcEnt->model->Anim);
    }

    combatData.Init(parameters->hierarchy, parameters->masterRadius, parameters->attackTableStates,
                    parameters->stateCount, parameters->hitPoints, npcEnt->GetModelScale(), NULL, NULL, -1);

    combatData.envCollisionCB = NULL;
}

void zCompCombat::PostUpdate(xScene*, F32 dt)
{
    zICompNPCEntity* npcEnt = owner->GetEntityComponent();
    xASSERT_L(87, zCombatGetFrom(npcEnt));

    zCombatSystemUpdateEntity(npcEnt, dt);
}
