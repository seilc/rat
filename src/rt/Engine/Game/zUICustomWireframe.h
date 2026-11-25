#ifndef ZUICUSTOMWIREFRAME_H
#define ZUICUSTOMWIREFRAME_H

#include "zUICustom.h"

#include <rwcore.h>

struct zWireframeAsset;

class zUICustomWireframe : public zUICustom
{
public:
    zUICustomWireframe(zUI* ui);

    virtual void Init();
    virtual bool PreRender();
    virtual void PreUpdate(F32 dt);
    virtual void Exit();

private:
    zWireframeAsset* wireframeAsset;
    RwIm2DVertex* vertices;
    F32 dX;
    F32 dY;
    F32 rotation;
    F32 scale;
};

extern U32 loadSceneID;

#endif
