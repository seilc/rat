#ifndef ZICOMPSTEERING_H
#define ZICOMPSTEERING_H

#include "zIComponent.h"
#include "zNPCBase.h"

struct xEnt;

class zICompSteering : public zIComponent<zNPCBase>
{
public:
    virtual void ApplySteering(zICompNPCEntity* npcEnt, F32 dt);
    virtual void SetAttractor(xEnt* ent) { xFAIL_MSG_L(38, "This function is not defined for this component"); }
    virtual void SetRepeller(xEnt* ent) { xFAIL_MSG_L(39, "This function is not defined for this component"); }
    virtual void SetArriver(xEnt* ent) { xFAIL_MSG_L(40, "This function is not defined for this component"); }
    virtual void SetAttractorV(xVec3* vec) { xFAIL_MSG_L(41, "This function is not defined for this component"); }
    virtual void SetRepellerV(xVec3* vec) { xFAIL_MSG_L(42, "This function is not defined for this component"); }
    virtual void SetArriverV(xVec3* vec) { xFAIL_MSG_L(43, "This function is not defined for this component"); }
    virtual void SetDestination(xVec3* vec) { xFAIL_MSG_L(45, "This function is not defined for this component"); }
    virtual void SetFacing(xVec3* vec) { xFAIL_MSG_L(46, "This function is not defined for this component"); }
};

#endif
