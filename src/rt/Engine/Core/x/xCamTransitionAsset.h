#ifndef XCAMTRANSITIONASSET_H
#define XCAMTRANSITIONASSET_H

#include "xDynAsset.h"

struct xLinkAsset;

struct xCamTransitionAsset : xDynAsset
{
    static const U16 VERSION = 1;
    
    U32 mVersion;
    S32 mType;
    U32 mFlags;
    U32 mDestCameraID;
    union
    {
        struct
        {
            F32 mTime;
            F32 mAccel;
            F32 mDecel;
        };
        struct
        {
            U32 mSrcCameraID;
            U32 mCurveID;
            U32 mTriggerID;
        };
    };

    void Validate() const
    {
        xASSERTM(58, mVersion == VERSION, "Camera Transition Asset version is %d, wanting %d. Repack.", mVersion, VERSION);
    }

    xLinkAsset* Links() const
    {
        return linkCount ? (xLinkAsset*)(this + 1) : NULL;
    }
};

#endif
