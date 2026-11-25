#include "xCommon.h"
#include "zPendulum.h"

#include "containers.h"
#include "xFont.h"
#include "xPad.h"
#include "xUpdateCull.h"
#include "zCollGeom.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zShrapnel.h"

static void zPendulumTranslate(xEnt* xent, xVec3* dpos, xMat4x3* dmat);

void zPendulum_Init(void* pend, void* asset)
{
    zPendulum_Init((zPendulum*)pend, (xEntAsset*)asset);
}

void zPendulum_Init(zPendulum* pend, xEntAsset* asset)
{
    zEntInit(pend, asset, 'PEND');

    xASSERT_L(47, pend);

    xEntMotionAsset* motionAsset = (xEntMotionAsset*)(asset + 1);

    if (pend->linkCount)
        pend->link = (xLinkAsset*)(motionAsset + 1);
    else
        pend->link = NULL;

    xASSERT_L(61, ! (pend->pflags & k_XENT_HAS_GRAVITY));
    xASSERT_L(64, ! (pend->pflags & k_XENT_HAS_VELOCITY));
    xASSERT_L(67, pend->pflags & k_XENT_IS_MOVING);

    pend->update = (xEntUpdateCallback)zPendulum_Update;
    pend->move = (xEntMoveCallback)zPendulum_Move;
    pend->eventFunc = zPendulumEventCB;
    pend->transl = zPendulumTranslate;

    zEntReset(pend);

    xEntMotionInit(&pend->motion, pend, motionAsset);
}

void zPendulum_Save(zPendulum* pend, xSerial* s)
{
    zEntSave(pend, s);

    xASSERT_L(95, pend);
    xASSERT_L(96, s);
}

void zPendulum_Load(zPendulum* pend, xSerial* s)
{
    zEntLoad(pend, s);

    xASSERT_L(114, pend);
    xASSERT_L(115, s);
}

void zPendulum_Setup(zPendulum* pend, xScene* sc)
{
    zEntSetup(pend);

    xEntMotionSetup(&pend->motion, sc);
}

void zPendulum_Reset(zPendulum* pend, xScene* sc)
{
    zEntReset(pend);

    xEntMotionReset(&pend->motion, sc);

    xEntMotionPenData* aspen = &pend->motion.asset->pen;

    pend->lt = 0.0f;
    pend->q1t = 1.0f - aspen->phase / (2.0f * PI);
    pend->q3t = 0.5f - aspen->phase / (2.0f * PI);

    if (pend->q1t <= 0.0f) {
        pend->q1t += 1.0f;
    }

    if (pend->q1t > 1.0f) {
        pend->q1t -= 1.0f;
    }

    if (pend->q3t <= 0.0f) {
        pend->q3t += 1.0f;
    }

    if (pend->q3t > 1.0f) {
        pend->q3t -= 1.0f;
    }

    pend->q1t *= aspen->period;
    pend->q3t *= aspen->period;
}

void zPendulum_Update(zPendulum* pend, xScene* sc, F32 dt)
{
    xEntUpdate(pend, sc, dt);

    xEntMotionPenData* aspen = &pend->motion.asset->pen;
    F32 t = pend->motion.t;
    if (t < pend->lt) {
        t += aspen->period;
    }

    if (pend->lt <= pend->q1t && t > pend->q1t) {
        zEntEvent(pend, eEventSwoosh);
    } else if (pend->lt <= pend->q3t && t > pend->q3t) {
        zEntEvent(pend, eEventSwoosh);
    }

    pend->lt = pend->motion.t;
}

void zPendulum_Move(zPendulum* pend, xScene* s, F32 dt, xEntFrame* frame)
{
    xEntMotionMove(&pend->motion, s, dt, frame);
}

static void zPendulumTranslate(xEnt* xent, xVec3* dpos, xMat4x3* dmat)
{
    zPendulum* pend = (zPendulum*)xent;

    xEntDefaultTranslate(pend, dpos, dmat);
    xEntMotionTranslate(&pend->motion, dpos, dmat);
}

void zPendulumEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32)
{
    xASSERT_L(207, to);

    zPendulum* pend = (zPendulum*)to;

    switch (toEvent) {
    case eEventRun:
        xEntMotionRun(&pend->motion);
        break;

    case eEventStop:
        xEntMotionStop(&pend->motion);
        break;

    case eEventDebugReset:
    case eEventReset:
        zPendulum_Reset(pend, globals.sceneCur);
        break;

    case eEventVisible:
    case eEventFastVisible:
        xEntShow(pend);
        break;

    case eEventInvisible:
    case eEventFastInvisible:
        xEntHide(pend);
        break;

    case eEventCollision_Visible_On:
        xEntShow(pend);
    case eEventCollisionOn:
        pend->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        pend->bupdate(pend, &((xMat4x3*)pend->model->Mat)->pos);
        break;

    case eEventCollision_Visible_Off:
        xEntHide(pend);
    case eEventCollisionOff:
        pend->chkby = k_XENT_COLLTYPE_NONE;
        break;

    case eEventCameraCollideOn:
        zCollGeom_CamEnable(pend);
        break;

    case eEventCameraCollideOff:
        zCollGeom_CamDisable(pend);
        break;

    case eEventAnimPlay:
    case eEventAnimPlayLoop:
    case eEventAnimStop:
    case eEventAnimPause:
    case eEventAnimResume:
    case eEventAnimTogglePause:
    case eEventAnimPlayRandom:
    case eEventAnimPlayMaybe:
        zEntAnimEvent(pend, toEvent, toParam);
        break;

    case eEventSetUpdateDistance:
        if (globals.updateMgr) {
            if (toParam[0] <= 0.0f) {
                xUpdateCull_SetCB(globals.updateMgr, pend, xUpdateCull_AlwaysTrueCB, NULL);
            } else {
                FloatAndVoid dist;
                dist.f = xsqr(toParam[0]);
                xUpdateCull_SetCB(globals.updateMgr, pend, xUpdateCull_DistanceSquaredCB, dist.v);
            }
        }
        break;

    case eEventLaunchShrapnel:
    {
        zShrapnelAsset* shrap = (zShrapnelAsset*)toParamWidget;
        if (shrap && shrap->initCB) {
            shrap->initCB(shrap, pend->model, NULL, NULL, NULL);
        }
        break;
    }

    case eEventSetLightKit:
        xModelSetLightKit(pend->model, (xLightKit*)toParamWidget, toParam[0]);
        break;
    }
}
