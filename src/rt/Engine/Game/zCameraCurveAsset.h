#ifndef ZCAMERACURVEASSET_H
#define ZCAMERACURVEASSET_H

#include "xBaseAsset.h"

struct zCameraCurveAsset : xBaseAsset
{
private:
    U8 mVersion;
    U8 pad[3];
    S32 mCameraType;
    U32 mFlags;
    S32 mTransitionType;
    F32 mTransitionTime;
    U32 mCurveID[2];
    S32 mNumBeads;
};

#endif
