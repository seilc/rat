#ifndef ZICOMPLOGIC_H
#define ZICOMPLOGIC_H

#include "zIComponent.h"
#include "zNPCBase.h"

struct SphereCollisionResults;
struct zCombatDamageInfo;
struct xEnt;

class zICompLogic : public zIComponent<zNPCBase>
{
public:
    virtual bool InteractionCheck(const SphereCollisionResults& results);
    virtual void NPCGetsDamage(const zCombatDamageInfo&);
    virtual void NPCGivesDamage(xEnt*, const zCombatDamageInfo&);
};

#endif
