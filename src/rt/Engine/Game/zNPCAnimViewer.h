#ifndef ZNPCANIMVIEWER_H
#define ZNPCANIMVIEWER_H

#include "zNPCBase.h"

class zNPCAnimViewer : public zNPCBase
{
public:
    bool Activate();
    void Deactivate();
#if !RELEASE
    void SceneExit();
#endif
    void Update(xScene* xscn, F32 dt);
    void Render();

    static bool IsPresent() { return mIsPresent; }

    static bool mIsPresent;
};

#endif
