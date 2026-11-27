#include "xCommon.h"
#include "zHangable.h"

#include "xColor.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xGroup.h"
#include "zBase.h"
#include "zCommonPlayer.h"
#include "zEntSimpleObj.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"

void zHangable_Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(18, asset, asset_size, zHangableAsset, "game_object:Hangable");
    zHangable_Init((zHangable*)&data, (zHangableAsset*)&asset);
}

void zHangable_Init(zHangable* hangable, zHangableAsset* asset)
{
    xBaseInit(hangable, asset);

    hangable->hangableAsset = asset;
    hangable->eventFunc = zHangable_EventCB;

    if (hangable->linkCount)
        hangable->link = (xLinkAsset*)(asset + 1);
    else
        hangable->link = NULL;

    asset->maxAngVel *= PI / 180.0f;

    if (!(asset->hangFlags & 0x2) || !(asset->hangFlags & 0x4)) {
        asset->hangFlags &= ~0x8;
    }
}

static zHangableObject* sPlayerHangObj;
static xVec3 playerDPos;
static zHangableObject* sHangableObjects;
static S32 sNumHangableObjects;

S32 zHangable_Recurse(U32 id, zHangableObject* gpList, zHangable* hangable)
{
    U32 i;
    S32 result = 0;
    xBase* base = zSceneFindObject(id);
    if (base) {
        if (base->baseType == eBaseTypeGroup) {
            xGroup* group = (xGroup*)base;
            U32 count = xGroupGetCount(group);
            for (i = 0; i < count; i++) {
                U32 memberID = xGroupGetItem(group, i);
                S32 incr;
                if (gpList) {
                    incr = zHangable_Recurse(memberID, &gpList[result], hangable);
                } else {
                    incr = zHangable_Recurse(memberID, NULL, NULL);
                }
                result += incr;
            }
        } else {
            if (gpList) {
                zEntSimpleObj* simp = (zEntSimpleObj*)base;
                simp->sflags |= 0x8;
                gpList[result].object = simp;
                gpList[result].hangable = hangable;
            }
            result++;
        }
    }

    return result;
}

void zHangable_SceneInit(zScene* zsc)
{
    S32 i, count, numHangables = 0;
    zHangable* hangable;

    count = zsc->baseCount[eBaseTypeHangableObj];
    for (i = 0; i < count; i++) {
        hangable = &((zHangable*)zsc->baseList[eBaseTypeHangableObj])[i];
        numHangables += zHangable_Recurse(hangable->hangableAsset->object, NULL, NULL);
    }

    if (numHangables > 0)
        sHangableObjects = (zHangableObject*)xMEMALLOC_L(numHangables * sizeof(zHangableObject), 0, eMemMgrTag_EnvAbstract, 0, 117);
    else
        sHangableObjects = NULL;
    sNumHangableObjects = numHangables;

    numHangables = 0;
    for (i = 0; i < count; i++) {
        hangable = &((zHangable*)zsc->baseList[eBaseTypeHangableObj])[i];
        numHangables += zHangable_Recurse(hangable->hangableAsset->object, &sHangableObjects[numHangables], hangable);
    }

    xASSERT_L(131, numHangables == sNumHangableObjects);

    for (i = 0; i < sNumHangableObjects; i++) {
        zHangableObject* hangObj = &sHangableObjects[i];
        zHangableAsset* asset = hangObj->hangable->hangableAsset;

        hangObj->hangable->flags = asset->hangFlags;

        xMat3x3 rotMat;
        xVec3 pivot, disp, cross;
        F32 len, ang;

        xASSERT_FMT_L(144, (hangObj->object->model), "Hangable Object '%s' has no model.", xSceneID2Name(xglobals->sceneCur, asset->id));

        xMat3x3RMulVec(&pivot, (xMat3x3*)hangObj->object->model->Mat, &asset->pivot);
        xVec3AddTo(&pivot, (xVec3*)&hangObj->object->model->Mat->pos);
        xMat3x3RMulVec(&hangObj->handle, (xMat3x3*)hangObj->object->model->Mat, &asset->handle);
        xVec3AddTo(&hangObj->handle, (xVec3*)&hangObj->object->model->Mat->pos);
        xVec3Sub(&hangObj->lastDisp, &hangObj->handle, &pivot);
        hangObj->radius = xVec3Length(&hangObj->lastDisp);

        xVec3SMul(&disp, &hangObj->lastDisp, 1.0f / hangObj->radius);
        xVec3Init(&cross, disp.z, 0.0f, -disp.x);
        len = xVec3Normalize(&cross, &cross);
        ang = xasin(len);
        if (disp.y > 0.0f) {
            ang = PI - ang;
        }

        xMat3x3Rot(&rotMat, &cross, ang);

        xMat3x3Mul(&hangObj->centerMat, (xMat3x3*)hangObj->object->model->Mat, &rotMat);

        xVec3Init(&hangObj->accel, 0.0f, 0.0f, 0.0f);
        xVec3Init(&hangObj->vel, 0.0f, 0.0f, 0.0f);
        xVec3Add(&hangObj->rotAxis, &hangObj->centerMat.right, &hangObj->centerMat.at);
        xVec3Normalize(&hangObj->rotAxis, &hangObj->rotAxis);
    }

    zHangable_Reset();
}

void zHangable_Reset()
{
    for (S32 i = 0; i < sNumHangableObjects; i++) {
        zHangableObject* hangObj = &sHangableObjects[i];
        zHangable* hangable = hangObj->hangable;
        hangable->flags = hangable->hangableAsset->hangFlags;

        if (hangObj->object->driveInfo) {
            hangObj->object->driveInfo->flags |= 0x2;
        }
    }
}

void zHangable_Update(F32 dt)
{
    xVec3Init(&playerDPos, 0.0f, 0.0f, 0.0f);

    for (S32 i = 0; i < sNumHangableObjects; i++) {
        zHangableObject* hangObj = &sHangableObjects[i];
        zHangableAsset* asset = hangObj->hangable->hangableAsset;

        xVec3 oldHandle, hangDisp, playerCenter, localCenter;
        xVec3Copy(&oldHandle, &hangObj->handle);

        if (hangObj == sPlayerHangObj) {
            xVec3Copy(&playerCenter, (xVec3*)&CURPLAYERS[0]->model->Mat->pos);
            playerCenter.y += CENTER_OF_MASS_HEIGHT;

            xVec3 centerDisp;
            xVec3Sub(&centerDisp, &playerCenter, (xVec3*)&hangObj->object->model->Mat->pos);
            localCenter.x = xVec3Dot(&centerDisp, (xVec3*)&hangObj->object->model->Mat->right);
            localCenter.y = xVec3Dot(&centerDisp, (xVec3*)&hangObj->object->model->Mat->up);
            localCenter.z = xVec3Dot(&centerDisp, (xVec3*)&hangObj->object->model->Mat->at);
        }

        hangObj->handle.AddScale(hangObj->vel, dt);

        if (hangObj == sPlayerHangObj) {
            hangObj->accel.y -= asset->onGravity;
        } else {
            hangObj->accel.y -= asset->offGravity;
        }

        xVec3 disp;
        F32 dist;
        xVec3 pivot;
        xMat3x3RMulVec(&pivot, (xMat3x3*)hangObj->object->model->Mat, &asset->pivot);
        xVec3AddTo(&pivot, (xVec3*)&hangObj->object->model->Mat->pos);
        xVec3Sub(&disp, &hangObj->handle, &pivot);

        xVec3Sub(&hangObj->objVel, &pivot, &oldHandle);
        xVec3AddTo(&hangObj->objVel, &hangObj->lastDisp);
        xVec3SMulBy(&hangObj->objVel, 1.0f / dt);

        dist = xVec3Length(&disp);

        xMat3x3 newMat, rotMat;
        if (hangObj == sPlayerHangObj && (hangObj->hangable->hangableAsset->hangFlags & 0x8)) {
            xMat3x3Mul(&newMat, &hangObj->centerMat, (xMat3x3*)CURPLAYERS[0]->model->Mat);
        } else {
            xMat3x3Copy(&newMat, &hangObj->centerMat);
        }

        if (dist > EPSILON) {
            xVec3SMulBy(&disp, 1.0f / dist);
            xVec3SMul(&hangDisp, &disp, hangObj->radius);
            xVec3Add(&hangObj->handle, &hangDisp, &pivot);

            hangObj->handle.AddScale(hangObj->accel, xsqr(dt));

            xVec3Sub(&hangDisp, &hangObj->handle, &pivot);

            if (!(hangObj->hangable->flags & 0x2)) {
                F32 comp = xVec3Dot(&hangDisp, &newMat.right);
                hangDisp.AddScale(newMat.right, -comp);
            }

            if (!(hangObj->hangable->flags & 0x4)) {
                F32 comp = xVec3Dot(&hangDisp, &newMat.at);
                hangDisp.AddScale(newMat.at, -comp);
            }

            dist = xVec3Length(&hangDisp);
            xVec3SMulBy(&hangDisp, hangObj->radius / dist);

            F32 maxAng = asset->maxAngVel * dt;

            xVec3 cross;
            xVec3Cross(&cross, &hangObj->lastDisp, &hangDisp);
            F32 len = xVec3Normalize(&cross, &cross);
            len /= xsqr(hangObj->radius);
            F32 ang = xasin(len);
            if (ang > maxAng || ang < -maxAng) {
                if (ang > maxAng) {
                    ang = maxAng;
                } else {
                    ang = -maxAng;
                }

                xMat3x3 rotMat;
                xMat3x3Rot(&rotMat, &cross, ang);
                xMat3x3RMulVec(&hangDisp, &rotMat, &hangObj->lastDisp);
            }

            xVec3Add(&hangObj->handle, &hangDisp, &pivot);

            xVec3Sub(&hangObj->vel, &hangObj->handle, &oldHandle);
            xVec3SMulBy(&hangObj->vel, 1.0f / dt);
        } else {
            // BUG: hangDisp is uninitialized here
            xVec3Add(&hangObj->handle, &hangDisp, &pivot);
        }

        xVec3Init(&hangObj->accel, 0.0f, 0.0f, 0.0f);

        xVec3 actualDisp, actualPivot;
        xMat3x3RMulVec(&actualDisp, &newMat, &asset->handle);
        xMat3x3RMulVec(&actualPivot, &newMat, &asset->pivot);
        xVec3SubFrom(&actualDisp, &actualPivot);

        xVec3 cross;
        xVec3Cross(&cross, &actualDisp, &hangDisp);
        F32 len = xVec3Normalize(&cross, &cross);
        len /= xsqr(hangObj->radius);
        F32 ang = xasin(len);
        if (xVec3Dot(&actualDisp, &hangDisp) < 0.0f) {
            ang = PI - ang;
        }

        xMat3x3Rot(&rotMat, &cross, ang);

        F32 minArcRadians = (PI / 180.0f) * asset->minArcDegrees;
        if (ang > minArcRadians && ang > 0.005f) {
            xVec3Copy(&hangObj->rotAxis, &cross);
        }

        if (hangObj != sPlayerHangObj && ang < minArcRadians) {
            F32 height = hangObj->radius + hangDisp.y;

            xVec3 relVel;
            xVec3Sub(&relVel, &hangObj->vel, &hangObj->objVel);
            F32 velSq = xVec3Length2(&relVel);

            F32 kinetic = 0.5f * velSq, potential = height * asset->offGravity, total = kinetic + potential;

            F32 reqTotal = asset->offGravity * (1.0f - icos(minArcRadians));
            if (total < reqTotal) {
                xVec3 velAddDir;
                xVec3Cross(&velAddDir, &hangObj->rotAxis, &hangDisp);
                xVec3Normalize(&velAddDir, &velAddDir);
                F32 velDot = xVec3Dot(&velAddDir, &relVel);

                if (velDot < 0.0f) {
                    hangObj->vel.AddScale(velAddDir, -0.1f);

#if DEBUG || RELEASE
                    xDrawSetColor(g_BLUE);
                    xVec3 negAddDir;
                    xVec3Inv(&negAddDir, &velAddDir);
                    xDrawArrow(&hangObj->handle, &negAddDir, false, 1.0f);
#endif
                } else {
                    hangObj->vel.AddScale(velAddDir, 0.1f);

#if DEBUG || RELEASE
                    xDrawSetColor(g_BLUE);
                    xDrawArrow(&hangObj->handle, &velAddDir, false, 1.0f);
#endif
                }
            }
        }

        xMat3x3Mul(&newMat, &newMat, &rotMat);

        xMat3x3RMulVec(&pivot, (xMat3x3*)hangObj->object->model->Mat, &asset->pivot);
        xVec3AddTo((xVec3*)&hangObj->object->model->Mat->pos, &pivot);
        xMat3x3Copy((xMat3x3*)hangObj->object->model->Mat, &newMat);
        xMat3x3RMulVec(&pivot, &newMat, &asset->pivot);
        xVec3SubFrom((xVec3*)&hangObj->object->model->Mat->pos, &pivot);

        xModelInstance* atomic = hangObj->object->model->Next;
        while (atomic) {
            xMat4x3Copy((xMat4x3*)atomic->Mat, (xMat4x3*)hangObj->object->model->Mat);
            atomic = atomic->Next;
        }

        xVec3Copy(&hangObj->lastDisp, &hangDisp);

        if (hangObj == sPlayerHangObj) {
            xVec3 newPlayerCenter;
            xMat3x3RMulVec(&newPlayerCenter, (xMat3x3*)hangObj->object->model->Mat, &localCenter);
            xVec3AddTo(&newPlayerCenter, (xVec3*)&hangObj->object->model->Mat->pos);
            xVec3Sub(&playerDPos, &newPlayerCenter, &playerCenter);
        }
    }
}

void zHangable_Recenter(zHangableObject* hangObj)
{
    zHangableAsset* asset = hangObj->hangable->hangableAsset;

    xVec3Init(&hangObj->vel, 0.0f, 0.0f, 0.0f);
    xVec3Init(&hangObj->accel, 0.0f, 0.0f, 0.0f);

    xVec3 pivot, newPivot;
    xMat4x3* objMat = (xMat4x3*)hangObj->object->model->Mat;

    xMat3x3RMulVec(&pivot, objMat, &asset->pivot);
    xMat3x3Copy(objMat, &hangObj->centerMat);
    xMat3x3RMulVec(&newPivot, objMat, &asset->pivot);

    xVec3AddTo(&objMat->pos, &pivot);
    xVec3SubFrom(&objMat->pos, &newPivot);

    xMat3x3RMulVec(&hangObj->handle, objMat, &asset->handle);
    xVec3AddTo(&hangObj->handle, &objMat->pos);
    xVec3AddTo(&newPivot, &objMat->pos);
    xVec3Sub(&hangObj->lastDisp, &hangObj->handle, &newPivot);
}

void zHangable_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    xASSERT_L(501, to);

    zHangable* hang = (zHangable*)to;
    xASSERT_L(504, hang->baseType == eBaseTypeHangableObj);

    switch (toEvent) {
    case eEventOn:
        hang->flags |= 0x1;
        break;
    case eEventOff:
        hang->flags &= ~0x1;
        break;
    case eEventUntilt:
        for (S32 i = 0; i < sNumHangableObjects; i++) {
            zHangableObject* hangObj = &sHangableObjects[i];
            if (hangObj->hangable == hang)
                zHangable_Recenter(hangObj);
        }
        break;
    }
}

// UNUSED: needed to match (code from Incredibles GC)
zHangableObject* zHangable_GetHangableObject(S32 i)
{
    xASSERT(i < sNumHangableObjects);
    return &sHangableObjects[i];
}
