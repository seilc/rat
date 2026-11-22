#ifndef ZTIMEHANDLER_H
#define ZTIMEHANDLER_H

#include "xVec3.h"
#include "zQueue.h"

struct zTimeHandlerStruct
{
    F32 elapsed;
    F32 period;
    void (*cb)(F32, zTimeHandlerStruct*);
    xVec3 pos;
    xVec3 normal;
    F32 num_particles;

    zTimeHandlerStruct& operator=(const zTimeHandlerStruct& data)
    {
        cb = data.cb;
        elapsed = data.elapsed;
        period = data.period;
        pos = data.pos;
        normal = data.normal;
        num_particles = data.num_particles;
        return *this;
    }
};

class zTimeHandlerMgr : public zQueue<zTimeHandlerStruct, 64>
{
public:
    void update(F32 dt);
};

extern zTimeHandlerMgr timehandler_mgr;

#endif
