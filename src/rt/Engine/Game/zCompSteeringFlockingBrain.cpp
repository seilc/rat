#include "zCompSteeringFlockingBrain.h"

#include "zICompNPCEntity.h"

void zCompSteeringFlockingBrain::AllAttached()
{
}

void zCompSteeringFlockingBrain::PreUpdate(xScene*, F32)
{
    zNPCBase* ownerNPC = owner;

    zNPCBase* neighbourNPCs;
    S32 numberNeighbourNPCs;
    ownerNPC->GetChildren(&neighbourNPCs, &numberNeighbourNPCs);

    flockingCenter = xVec3::m_Null;
    flockingAlignment = xVec3::m_Null;

    const xVec3* neighbourEntityPos;
    const xVec3* neighbourEntityHeading;
    for (S32 i = 0; i < numberNeighbourNPCs; i++) {
        zICompNPCEntity* curNeighbourEntity = neighbourNPCs[i].GetEntityComponent();
        neighbourEntityPos = curNeighbourEntity->GetPosition();
        neighbourEntityHeading = curNeighbourEntity->GetHeading();
        flockingCenter.Add(*neighbourEntityPos);
        flockingAlignment.Add(*neighbourEntityHeading);
    }
    
    F32 oneOverNumbers = 1.0f / numberNeighbourNPCs;
    flockingCenter.Scale(oneOverNumbers);
    flockingAlignment.Scale(oneOverNumbers);
}
