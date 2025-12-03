#include "zCompLogicSwarmMember.h"

#include "zICompSteering.h"

void zCompLogicSwarmMember::Update(xScene*, F32 dt)
{
    zICompSteering* compSteering = owner->GetSteeringComponent();
    zICompNPCEntity* compNPCEntity = owner->GetEntityComponent();
    if (compSteering && compNPCEntity) {
        compSteering->ApplySteering(compNPCEntity, dt);
    }
}
