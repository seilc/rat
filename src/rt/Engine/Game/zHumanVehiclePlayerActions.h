#ifndef ZHUMANVEHICLEPLAYERACTIONS_H
#define ZHUMANVEHICLEPLAYERACTIONS_H

#include "zHumanVehiclePlayer.h"

class zHumanVehiclePlayerDrive : public zPlayerAction
{
public:
    static void CameraRestartHack();

protected:
    struct driveSurfaceDescriptor
    {
        F32 staticTraction;
        F32 slidingTraction;
        F32 slideThreshold;
        F32 topSpeedFactor;
    };

    F32 steeringPosition;
    U8 initCameraPending;
    U8 isSliding;
    F32 scalarVel;
    U8 started;
    U8 smHitReactPending;
    U8 lgHitReactPending;
    driveSurfaceDescriptor* pCurrentSurface;
    S32 numCollisions;
    xCollis collisions[18];
    U32 collisionUserFlags[18];
    xBound bound[5];
    S32 currBound;
    U8 inGroundContact;
    U8 collisionTypes[4];
    U8 controllerInput;

    U32 MoveStartCheck(xAnimTransition*, xAnimSingle*);
    U32 MoveStopCheck(xAnimTransition*, xAnimSingle*);
    U32 RunStartCheck(xAnimTransition*, xAnimSingle*);
    U32 RunStopCheck(xAnimTransition*, xAnimSingle*);
    U32 SmHitReactStartCheck(xAnimTransition*, xAnimSingle*);
    U32 LgHitReactStartCheck(xAnimTransition*, xAnimSingle*);
    U32 HitReactToWalkCheck(xAnimTransition*, xAnimSingle*);
    U32 HitReactToRunCheck(xAnimTransition*, xAnimSingle*);

    static U32 anMoveStartCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(122, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->MoveStartCheck(tran, anim);
    }

    static U32 anMoveStopCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(123, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->MoveStopCheck(tran, anim);
    }

    static U32 anRunStartCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(124, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->RunStartCheck(tran, anim);
    }

    static U32 anRunStopCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(125, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->RunStopCheck(tran, anim);
    }

    static U32 anSmHitReactStartCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(126, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->SmHitReactStartCheck(tran, anim);
    }

    static U32 anLgHitReactStartCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(127, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->LgHitReactStartCheck(tran, anim);
    }

    static U32 anHitReactToWalkCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(128, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->HitReactToWalkCheck(tran, anim);
    }

    static U32 anHitReactToRunCheck(xAnimTransition* tran, xAnimSingle* anim, void*)
    {
        xASSERTM(129, anim->State->CallbackData == tran->Dest->CallbackData,
            "Action transition that didn't use the action transition function. This will potentially cause an error.");
        return ((zHumanVehiclePlayerDrive*)anim->State->CallbackData)->HitReactToRunCheck(tran, anim);
    }
};

#endif
