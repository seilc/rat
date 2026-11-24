#ifndef ZICOMPLOGIC_H
#define ZICOMPLOGIC_H

#include "zIComponent.h"
#include "zNPCBase.h"
#include "xAnim.h"

struct SphereCollisionResults;
struct zCombatDamageInfo;
struct xEnt;

class zICompLogic : public zIComponent<zNPCBase>
{
public:
    virtual bool InteractionCheck(const SphereCollisionResults& results) { return false; }
    virtual void NPCGetsDamage(const zCombatDamageInfo&) {}
    virtual void NPCGivesDamage(xEnt*, const zCombatDamageInfo&) {}

    static void CreateAnimTable(xAnimTable* table)
    {
        xAnimTableNewState(table, "ERROR", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
        xAnimTableNewTransition(table, "*", "ERROR", NULL, AnimationErrorStateCB, AnimTransition_Default, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    }

    static U32 AnimationErrorStateCB(xAnimTransition*, xAnimSingle*, void*)
    {
        xFAIL_MSG_L(67, "Error State!");
        return 0;
    }
};

#endif
