#include "xCommon.h"
#include "zCamPool.h"

#include "containers.h"
#include "xCam.h"
#include "zGlobals.h"

zCamPoolBase::zCamPoolBase() : mNumCameras(0), mNextCamera(0)
{
}

void zCamPoolBase::SceneExit()
{
    for (S32 i = 0; i < mNumCameras; i++)
        mCameras[i]->destroy();
    mNumCameras = 0;
}

void zCamPoolBase::AddCamera(xCam* newCam, const char* tweakPrefix)
{
    if (mNumCameras < MAX_POOL_CAMERAS) {
        xASSERT_L(24, newCam);
        if (newCam) {
            newCam->create();
            mCameras[mNumCameras++] = newCam;
#if DEBUG || RELEASE
            if (tweakPrefix)
                newCam->add_tweaks(tweakPrefix);
#endif
        }
    }
}

xCam* zCamPoolBase::GetNextCamera()
{
    static S32 depth = 0;

    for (S32 i = 0; i < mNumCameras; i++) {
        xCam* cam = mCameras[mNextCamera++];
        if (mNextCamera >= mNumCameras)
            mNextCamera = 0;
        if (!cam->started())
            return cam;
    }

    return NULL;
}

#if DEBUG || RELEASE
void zCamPoolBase::CheckCameraFlags()
{
    for (S32 i = 0; i < mNumCameras; i++) {
        if (mCameras[i]->group_flags & 0x10)
            mCameras[i]->group->PrintCamMsg("FOUND BLENDED", mCameras[i]->owner);
    }
}
#endif
