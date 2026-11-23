#ifndef ZNPCSTIRRINGPLAYER_H
#define ZNPCSTIRRINGPLAYER_H

#include "zNPCBase.h"

class zNPCStirringPlayer : public zNPCBase
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
