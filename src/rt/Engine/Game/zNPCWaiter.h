#ifndef ZNPCWAITER_H
#define ZNPCWAITER_H

#include "zNPCBase.h"

class zNPCWaiter : public zNPCBase
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
