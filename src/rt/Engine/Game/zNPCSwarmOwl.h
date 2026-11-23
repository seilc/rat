#ifndef ZNPCSWARMOWL_H
#define ZNPCSWARMOWL_H

#include "zNPCBase.h"

class zNPCSwarmOwl : public zNPCBase
{
public:
    static S32 CountChildrenInAsset(const xDynAsset* dynAsset, U32, const char*);

    virtual bool Activate();
#if !RELEASE
    virtual void Deactivate();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    virtual void UpdateNPCBaseData();
    virtual void Render();
};

#endif
