#ifndef ZCOMPCOMBAT_H
#define ZCOMPCOMBAT_H

#include "zCombat.h"
#include "zCompParameters.h"
#include "zICompCombat.h"

class zCompCombatParams : public zCompParameters
{
public:
    zCompCombatParams();

    S32 hitPoints;
    xHierarchyBoundInitData* hierarchy;
    F32 masterRadius;
    zCombatAttack* attackTableStates;
    U16 stateCount;

    virtual void SetDefaults();
};

class zCompCombat : public zICompCombat
{
public:
#if DEBUG
    static void AddStaticTweaks();
#endif

    virtual S32 Initialize();
    virtual void Attached();
    virtual void Detached();
#if RELEASE
    virtual void Reset() {}
#endif
    virtual void AllAttached();
    virtual void SetParameters(const zCompParameters* parameters) { this->parameters = (const zCompCombatParams*)parameters; }
    virtual void Update(xScene*, F32) {}
    virtual void PreUpdate(xScene*, F32) {}
    virtual void PostUpdate(xScene*, F32 dt);
    virtual void DebugRender() {}
    virtual bool SystemEvent(xBase*, xBase*, U32, const F32*, xBase*, U32) { return false; }

    bool InAttackRange(const xVec3&, U16); // UNUSED

private:
    inline const zCompCombatParams* Params(); // UNUSED

    const zCompCombatParams* parameters;
    zCombat combatData;
};

#endif
