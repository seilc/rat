#ifndef XCAMTRANSITION_H
#define XCAMTRANSITION_H

#include "xBase.h"
#include "xCam.h"
#include "xSpringy.h"

struct xCamTransitionAsset;
struct xDynAsset;
struct zEntTrigger;
class xNurbs;

class xCamTransition : public xBase
{
public:
    const xCamTransitionAsset* mAsset;
    zEntTrigger* mTransitionVolume;
    S32 mType;

    void Init(const xCamTransitionAsset& asset);
    void Init(zEntTrigger& theVolume);

#if DEBUG || RELEASE
    void Reset();
#endif

    static void Init(xBase& data, xDynAsset& asset, size_t);
    static void AddTriggerTransition(zEntTrigger& theVolume);
    F32 BlendTime() const;
    bool IsCut();
    void GetParams(xCamTransitionParams& params);
    void Start();
    void Completed(bool sendCompletionEvents);

private:
    bool mActive;
    xCamBias* mCamBlendBias;
};

class xCamPathBias : public xCamBias
{
public:
    xCamPathBias(zEntTrigger* triggerEnt);
    F32 RawBias();
    void ResetCurve();
    virtual void Start();
    virtual void Update(F32 dt);
    virtual void Reverse();
    F32 GetInitialRawBias();
    void SetDirection(bool forward);
    virtual F32 GetBias();
    virtual bool IsActive();
    virtual bool DidComplete();
    F32 CurveIntersectPlane(const xVec3& planePoint, F32& u, F32 guess);

#if DEBUG || RELEASE
    void DebugRender();
#endif

protected:
    xSpringyF32 mBias;
    F32 mLastU;
    const xNurbs* mCurve;
    zEntTrigger* mTrigger;
    xVec3 mMappingPlaneNormal;
    F32 mGuessSpanU;
    xVec3 mCurveEnd0;
    xVec3 mCurveEnd1;
    S32 mReversed;
    S32 mDirection;
    bool mIsActive;
#if DEBUG || RELEASE
    F32 mRenderIncU;
#endif
};

void zCamTransition_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif
