#ifndef ZCAMERACURVEASSET_H
#define ZCAMERACURVEASSET_H

#include "xBaseAsset.h"

enum eCamCurveCam
{
    eCamCurveCam_POI,
    eCamCurveCam_Rail,
    eCamCurveCam_NumCam
};

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

public:
    U32 GetCurveID(eCamCurveCam curveType) { return mCurveID[curveType]; }
};

#endif
