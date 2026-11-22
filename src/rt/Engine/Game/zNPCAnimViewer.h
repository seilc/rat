#ifndef ZNPCANIMVIEWER_H
#define ZNPCANIMVIEWER_H

#include "zNPCBase.h"

class zNPCAnimViewer : public zNPCBase
{
public:
    virtual bool Activate();
    virtual void Deactivate();
#if !RELEASE
    virtual void SceneExit();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    virtual void Render();

    static bool IsPresent() { return mIsPresent; }

    static bool mIsPresent;
};

#endif
