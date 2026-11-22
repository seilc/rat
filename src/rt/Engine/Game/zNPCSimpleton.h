#ifndef ZNPCSIMPLETON_H
#define ZNPCSIMPLETON_H

#include "zNPCBase.h"

class zNPCSimpleton : public zNPCBase
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
