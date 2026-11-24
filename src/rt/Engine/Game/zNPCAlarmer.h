#ifndef ZNPCALARMER_H
#define ZNPCALARMER_H

#include "zNPCBase.h"
#if !RELEASE
#include "zICompLogic.h"
#endif

struct xAnimTable;
#if RELEASE
class zCompSteeringGenericParams;
class zCompPerceptionParams;
class zICompNPCEntityParams;
#endif

class zNPCAlarmer : public zNPCBase
{
public:
#if RELEASE
    static void CompParametersInit();
    static void AddTweaks(const char*); // UNUSED
#endif
    static void CreateAnimTable(xAnimTable* table);

    virtual bool Activate();
#if !RELEASE
    virtual void Deactivate();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    virtual void Render();

#if RELEASE
    static zICompNPCEntityParams sEntParams;
    static zCompPerceptionParams sPerceptionParams;
    static zCompSteeringGenericParams sSteeringPatrolParams;
#endif
};

#if !RELEASE
class zCompLogicAlarmer : public zICompLogic
{
public:
    enum eAlarmerStates
    {
        eAlarmerStates_None = -1,
        eAlarmerStates_SearchPlayer,
        eAlarmerStates_ActivatingAlarm,
        eAlarmerStates_Alarm,
        eAlarmerStates_DeactivatingAlarm
    };

#if DEBUG
    static void AddStaticTweaks();
#endif

    virtual void Attached();
    virtual void AllAttached() {}
    virtual void Detached() {}
    virtual void Update(xScene*, F32);
    virtual bool SystemEvent(xBase*, xBase*, U32, const F32*, xBase*, U32) { return false; }
    virtual F32 GetMaxMoveSpeed() { return 5.0f; }

    void ChangeState(eAlarmerStates nextState);

private:
    eAlarmerStates curState;
};
#endif

#endif
