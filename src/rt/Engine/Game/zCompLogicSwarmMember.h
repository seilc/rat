#ifndef ZCOMPLOGICSWARMMEMBER_H
#define ZCOMPLOGICSWARMMEMBER_H

#include "zICompLogic.h"

class zCompLogicSwarmMember : public zICompLogic
{
public:
    virtual void Attached() {}
    virtual void AllAttached() {}
    virtual void Detached() {}
#ifdef RELEASE
    virtual void Reset() {}
#endif
    virtual void Update(xScene*, F32 dt);
};

#endif
