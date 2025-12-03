#ifndef ZICOMPSTEERING_H
#define ZICOMPSTEERING_H

#include "zIComponent.h"
#include "zNPCBase.h"

struct xEnt;

class zICompSteering : public zIComponent<zNPCBase>
{
public:
    virtual void ApplySteering(zICompNPCEntity* npcEnt, F32 dt);
    virtual void SetAttractor(xEnt* ent);
    virtual void SetRepeller(xEnt* ent);
    virtual void SetArriver(xEnt* ent);
    virtual void SetAttractorV(xVec3* vec);
    virtual void SetRepellerV(xVec3* vec);
    virtual void SetArriverV(xVec3* vec);
    virtual void SetDestination(xVec3* vec);
    virtual void SetFacing(xVec3* vec);
};

#endif
