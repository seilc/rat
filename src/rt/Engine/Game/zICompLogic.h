#ifndef ZICOMPLOGIC_H
#define ZICOMPLOGIC_H

#include "zIComponent.h"
#include "zNPCBase.h"

struct SphereCollisionResults;
struct zCombatDamageInfo;
struct xEnt;
struct xAnimTransition;
struct xAnimSingle;

class zICompLogic : public zIComponent<zNPCBase>
{
public:
    virtual bool InteractionCheck(const SphereCollisionResults& results) { return false; }
    virtual void NPCGetsDamage(const zCombatDamageInfo&) {}
    virtual void NPCGivesDamage(xEnt*, const zCombatDamageInfo&) {}

    static U32 AnimationErrorStateCB(xAnimTransition*, xAnimSingle*, void*)
    {
        xFAIL_MSG_L(67, "Error State!");
        return 0;
    }
};

#endif
