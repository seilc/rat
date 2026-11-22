#ifndef ZNPCCONTEXTSENSITIVE_H
#define ZNPCCONTEXTSENSITIVE_H

#include "zNPCBase.h"

class zNPCContextSensitive : public zNPCBase
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
