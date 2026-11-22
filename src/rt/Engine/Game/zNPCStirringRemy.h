#ifndef ZNPCSTIRRINGREMY_H
#define ZNPCSTIRRINGREMY_H

#include "zNPCBase.h"

class zNPCStirringRemy : public zNPCBase
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
