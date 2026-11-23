#ifndef ZUICUSTOMIMAGESLIDESHOW_H
#define ZUICUSTOMIMAGESLIDESHOW_H

#include "zUICustom.h"

struct xGroup;

class zUICustomImageSlideshow : public zUICustom
{
public:
    zUICustomImageSlideshow(zUI* ui);

    virtual void Reset();
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

    void SetTextureList(U32 groupID);
    void SetTextureList(xGroup* textureList);
    void UpdateTexture();
    void SetCurrentImageIndex(U32 index); // UNUSED

private:
    U32 m_CurrentImage;
    U32 m_NumImages;
    xGroup* m_TextureGroup;
};

#endif
