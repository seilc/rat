#ifndef ZNPCPOURNSWIRL_H
#define ZNPCPOURNSWIRL_H

#include "zNPCBase.h"

class zNPCPourNSwirl : public zNPCBase
{
public:
    virtual bool Activate();
#if !RELEASE
    virtual void Deactivate();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    virtual void Render();
};

#endif
