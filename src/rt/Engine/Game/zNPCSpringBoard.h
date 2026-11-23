#ifndef ZNPCSPRINGBOARD_H
#define ZNPCSPRINGBOARD_H

#include "zNPCBase.h"

class zNPCSpringBoard : public zNPCBase
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
