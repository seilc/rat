#ifndef ZUIIMAGE_H
#define ZUIIMAGE_H

#include "zUI.h"
#include "zUIImageAsset.h"
#include "xTextureManager.h"

class zUIImage : public zUI
{
public:
    struct ImageState
    {
        F32 rotation;
        F32 offsetU;
        F32 offsetV;
    };

    ImageState currentImage;
    ImageState startMovementImage;

    zUIImageAsset* GetAsset() const;
    xTextureHandle& GetTexture() const;
    void SetTexture(U32 id, bool clearPrev);
    void PlayMovie(U32 id, bool loop, bool stopFrame, U32 startFrame, U32 endFrame);
    void StopMovie();

    virtual U32 GetSortKey() const;
    virtual bool Blends() const;
    virtual void DoInit();
    virtual void DoReset();
    virtual void DoResetMotion();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void DoRender() const;
    virtual void DoInitMotion();
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);
    virtual void Visible();
    virtual void Invisible();
#if DEBUG || RELEASE
    virtual void DoAddTweaks(const char* baseName);
#endif

private:
    xTextureHandle texture;
    U32 currentID;
};

void zUIImage_Init(xBase& data, xDynAsset& asset, size_t);
void zUIImage_Init(zUIImage* image, zUIImageAsset* asset);

#endif
