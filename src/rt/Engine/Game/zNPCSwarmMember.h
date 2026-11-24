#ifndef ZNPCSWARMMEMBER_H
#define ZNPCSWARMMEMBER_H

#include "zNPCBase.h"

class zNPCSwarmMember : public zNPCBase
{
public:
    virtual bool Activate();
#if !RELEASE
    virtual void Deactivate();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    virtual void Render();
    virtual void Reset();
};

#endif
