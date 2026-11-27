#include "xCommon.h"
#include "zEntButton.h"

#include "xFont.h"
#include "xFX.h"
#include "xstransvc.h"
#include "xString.h"
#include "xserializer.h"
#include "xSndMgr.h"
#include "xUpdateCull.h"
#include "zCollGeom.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zShrapnel.h"

static F32 sRedMultiplier = 1.0f;
static F32 sGreenMultiplier = 1.0f;
static F32 sBlueMultiplier = 1.0f;

static void zEntButton_Press(zEntButton* ent);

void zEntButton_Init(void* ent, void* asset)
{
    zEntButton_Init((zEntButton*)ent, (xEntAsset*)asset);
}

void zEntButton_Init(zEntButton* ent, xEntAsset* asset)
{
    zEntInit(ent, asset, 'BUTN');

    zEntButtonAsset* passet = (zEntButtonAsset*)(asset + 1);
    xEntMotionAsset* emasset = (xEntMotionAsset*)(passet + 1);

    xASSERT_L(78, ent);

    ent->basset = passet;

    U32 bufsize;
    void* info = xSTFindAsset(passet->modelPressedInfoID, &bufsize);
    if (info) {
        ent->modelPressed = xEntLoadModel(NULL, (RpAtomic*)info);
    } else {
        ent->modelPressed = NULL;
    }

    if (asset->modelInfoID == xStrHash("button"))
        ent->topHeight = 0.36f;
    else if (asset->modelInfoID == xStrHash("plate_pressure"))
        ent->topHeight = 0.32524f;
    else
        ent->topHeight = 0.0f;

    ent->pflags |= k_XENT_IS_MOVING;
    ent->penby |= k_XENT_COLLTYPE_PC;
    ent->chkby |= k_XENT_COLLTYPE_PC;

    ent->update = (xEntUpdateCallback)zEntButton_Update;
    ent->move = (xEntMoveCallback)zEntButton_Move;
    ent->eventFunc = zEntButtonEventCB;
    ent->render = (xEntRenderCallback)zEntButton_Render;

    if (ent->linkCount)
        ent->link = (xLinkAsset*)(emasset + 1);
    else
        ent->link = NULL;

    xASSERT_L(124, ! (ent->pflags & k_XENT_HAS_GRAVITY));
    xASSERT_L(125, ent->pflags & k_XENT_IS_MOVING);

    ent->eventFunc = zEntButtonEventCB;

    ent->state = 0;
    ent->oldState = ent->state;
    ent->holdTimer = 0.0f;
    ent->counter = ent->basset->resetDelay;

    xEntReset(ent);

    xEntMotionInit(&ent->motion, ent, emasset);
    xEntMotionRun(&ent->motion);

    ent->oldMotState = ent->motion.mech.state;
}

void zEntButton_Move(zEntButton* ent, xScene* s, F32 dt, xEntFrame* frame)
{
    if (ent->driver) {
        xASSERT_DESIGN_L(151, ent->driveInfo == NULL, "Buttons Support only driverMode 0 'translation only' (in param 1). Button Asset ID %x", ent->id);
        if (ent->driveInfo == NULL) {
            xEntFrame* dframe = ent->driver->frame;
            xVec3 dpos;

            xVec3Sub(&dpos, &dframe->mat.pos, &dframe->oldmat.pos);
            xVec3Add(&ent->motion.mech.original_position, &ent->motion.mech.original_position, &dpos);
            xVec3Add(&ent->motion.mech.destination_position, &ent->motion.mech.destination_position, &dpos);
        }
    }

    xEntMotionMove(&ent->motion, s, dt, frame);
}

void zEntButton_Setup(zEntButton* ent, xScene* sc)
{
    zEntSetup(ent);

    xEntMotionSetup(&ent->motion, sc);
}

void zEntButton_Save(zEntButton* ent, xSerial* s)
{
    zEntSave(ent, s);

    xASSERT_L(185, ent);
    xASSERT_L(186, s);

    if (ent->state == 3)
        s->Write_b1(1);
    else
        s->Write_b1(0);

    if (ent->state == 1)
        s->Write_b1(1);
    else
        s->Write_b1(0);
}

void zEntButton_Load(zEntButton* ent, xSerial* s)
{
    zEntLoad(ent, s);

    xASSERT_L(214, ent);
    xASSERT_L(215, s);

    S32 pressed = 0;
    S32 pushing = 0;
    s->Read_b1(&pressed);
    if (pressed)
        ent->state = 1;
    s->Read_b1(&pushing);
    if (pushing) {
        ent->state = 1;
    }
}

void zEntButton_Reset(zEntButton* ent, xScene* sc)
{
    xEntReset(ent);

    xEntMotionInit(&ent->motion, ent, (xEntMotionAsset*)(ent->basset + 1));

    xEntMotionReset(&ent->motion, sc);

    ent->pflags |= k_XENT_IS_MOVING;
    ent->penby |= k_XENT_COLLTYPE_PC;

    if (ent->asset->flags & k_XENT_IS_VISIBLE) {
        xEntShow(ent);
    }

    ent->chkby |= k_XENT_COLLTYPE_PC;
    ent->bupdate(ent, &((xMat4x3*)ent->model->Mat)->pos);

    ent->state = 0;
    xEntMotionStop(&ent->motion);

    ent->oldState = 5;
    ent->oldMotState = k_XENTMECHSTATE_DONE;
}

void zEntButton_Update(zEntButton* ent, xScene* sc, F32 dt)
{
    if (ent->modelPressed) {
        ent->modelPressed->Mat = ent->model->Mat;
    }

    xEntUpdate(ent, sc, dt);

    if (ent->motion.mech.state != ent->oldMotState) {
        switch (ent->motion.mech.state) {
        case k_XENTMECHSTATE_SLIDE_FORTH:
            if (ent->state == 4)
                ent->state = 0;
            break;

        case k_XENTMECHSTATE_WAIT_RETURN:
            if (!xEntMotionIsStopped(&ent->motion))
                ent->state = 3;
            break;

        case k_XENTMECHSTATE_SLIDE_BACK:
            ent->state = 4;
            break;

        case k_XENTMECHSTATE_ROT_BACK:
        case k_XENTMECHSTATE_WAIT_AGAIN:
        case k_XENTMECHSTATE_DONE:
            break;
        }
    }

    ent->oldMotState = ent->motion.mech.state;

    if (ent->state != ent->oldState) {
        switch (ent->state) {
        case 1:
            xEntMotionRun(&ent->motion);
            break;

        case 2:
            break;

        case 3:
            xEntMotionStop(&ent->motion);
            break;

        case 4:
            xEntMotionRun(&ent->motion);
            break;

        case 0:
            if (ent->motion.mech.state == k_XENTMECHSTATE_SLIDE_FORTH)
                xEntMotionStop(&ent->motion);
            if (ent->basset->actMethod == 1) {
                zEntEvent(ent, ent, eEventUnpress);
                xSndMgrPlay(xSndMgrGetSoundGroup("Button_up"), 0, &((xMat4x3*)ent->model->Mat)->pos, NULL, NULL, NULL, NULL);
            }
            break;
        }

        ent->oldState = ent->state;
    }

    if (ent->basset->actMethod == 0) {
        if (ent->basset->isReset && ent->state) {
            ent->counter += dt;
        }

        if (ent->counter > ent->basset->resetDelay && ent->state == 3) {
            zEntEvent(ent, ent, eEventUnpress);
            xSndMgrPlay(xSndMgrGetSoundGroup("Button_up"), 0, &((xMat4x3*)ent->model->Mat)->pos, NULL, NULL, NULL, NULL);
        }
    }
    else if (ent->basset->actMethod == 1) {
        if (!ent->hold) {
            zEntButton_SetReady(ent);
        }
    } else {
        xFAIL_L(355);
    }

    ent->holdTimer -= dt;
    if (ent->holdTimer < 0.0f) {
        ent->holdTimer = 0.5f;
        ent->hold = FALSE;
    }
}

void zEntButton_Render(zEntButton* ent)
{
    xModelInstance* model = ent->model;

    if (ent->state == 3 && ent->modelPressed) {
        ent->modelPressed->LightKit = model->LightKit;
        ent->model = ent->modelPressed;
    }
    else if (ent->basset->actMethod == 0) {
        model->RedMultiplier = sRedMultiplier;
        model->GreenMultiplier = sGreenMultiplier;
        model->BlueMultiplier = sBlueMultiplier;
    }

    if (ent->collType != k_XENT_COLLTYPE_TRIG) {
        xEntRender(ent);
    }

    ent->model = model;
}

void zEntButton_SetReady(zEntButton* ent)
{
    if (ent->state) {
        ent->state = 4;
    }
}

static void zEntButton_Press(zEntButton* ent)
{
    if (ent->state == 0) {
        ent->state = 1;
        ent->counter = 0.0f;
    }
}

static void _PressButtonSound(zEntButton* ent, U32)
{
    xSndMgrPlay(xSndMgrGetSoundGroup("Button_press"), 0, &((xMat4x3*)ent->model->Mat)->pos, NULL, NULL, NULL, NULL);
}

void zEntButton_Press(zEntButton* ent, U32 mask)
{
    if (ent->basset->buttonActFlags & mask) {
        if (ent->state == 0) {
            zEntEvent(ent, eEventPress);
            _PressButtonSound(ent, mask);
        }
    }
}

void zEntButton_Hold(zEntButton* ent, U32 mask)
{
    if (ent->basset->buttonActFlags & mask) {
        if (mask & 0x400) {
            switch (ent->bound.type) {
            case k_XBOUNDTYPE_SPHERE:
                iDebugBreak();
                break;

            case k_XBOUNDTYPE_BOX:
                iDebugBreak();
                break;

            case k_XBOUNDTYPE_CYLINDER:
                iDebugBreak();
                break;

            case k_XBOUNDTYPE_OBB:
                xVec3* player = &((xMat4x3*)CURPLAYERS[0]->model->Mat)->pos;
                xBound* bound = &ent->bound;
                xVec3 lv;

                xMat4x3ToLocal(&lv, bound->mat, player);
                lv.y = 0.0f;

                if (!xPointInBox(&bound->box.box, &lv)) {
                    return;
                }
                break;
            }
        }

        if (ent->state == 0) {
            zEntEvent(ent, eEventPress);
            _PressButtonSound(ent, 0);
        } else {
            ent->hold = TRUE;
        }
    }
}

void zEntButtonEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32)
{
    xASSERT_L(554, to);

    zEntButton* s = (zEntButton*)to;

    xASSERT_L(557, s->baseType == eBaseTypeButton);

    switch (toEvent) {
    case eEventVisible:
    case eEventFastVisible:
    case eEventCollision_Visible_On:
        s->chkby |= k_XENT_COLLTYPE_PC;
        xEntShow(s);
        s->bupdate(s, &((xMat4x3*)s->model->Mat)->pos);
        break;

    case eEventInvisible:
    case eEventFastInvisible:
    case eEventCollision_Visible_Off:
        s->chkby &= (U8)~k_XENT_COLLTYPE_PC;
        xEntHide(s);
        break;

    case eEventCameraCollideOn:
        zCollGeom_CamEnable(s);
        break;

    case eEventCameraCollideOff:
        zCollGeom_CamDisable(s);
        break;

    case eEventReset:
    case eEventDebugReset:
        zEntButton_Reset(s, globals.sceneCur);
        break;

    case eEventPress:
        zEntButton_Press(s);
        break;

    case eEventUnpress:
        if (s->state)
            zEntButton_SetReady(s);
        break;

    case eEventToggle:
        if (s->state) {
            zEntEvent(s, s, eEventUnpress);
            xSndMgrPlay(xSndMgrGetSoundGroup("Button_up"), 0, &((xMat4x3*)s->model->Mat)->pos, NULL, NULL, NULL, NULL);
        } else {
            zEntEvent(s, s, eEventPress);
            xSndMgrPlay(xSndMgrGetSoundGroup("Button_press"), 0, &((xMat4x3*)s->model->Mat)->pos, NULL, NULL, NULL, NULL);
        }
        break;

    case eEventAnimPlay:
    case eEventAnimPlayLoop:
    case eEventAnimStop:
    case eEventAnimPause:
    case eEventAnimResume:
    case eEventAnimTogglePause:
    case eEventAnimPlayRandom:
    case eEventAnimPlayMaybe:
        zEntAnimEvent(s, toEvent, toParam);
        break;

    case eEventSetUpdateDistance:
        if (globals.updateMgr) {
            if (toParam[0] <= 0.0f) {
                xUpdateCull_SetCB(globals.updateMgr, s, xUpdateCull_AlwaysTrueCB, NULL);
            } else {
                FloatAndVoid dist;
                dist.f = xsqr(toParam[0]);
                xUpdateCull_SetCB(globals.updateMgr, s, xUpdateCull_DistanceSquaredCB, dist.v);
            }
        }
        break;

    case eEventLaunchShrapnel:
    {
        zShrapnelAsset* shrap = (zShrapnelAsset*)toParamWidget;
        if (shrap && shrap->initCB) {
            shrap->initCB(shrap, s->model, NULL, NULL, NULL);
        }
        break;
    }

    case eEventSetLightKit:
        xModelSetLightKit(s->model, (xLightKit*)toParamWidget, toParam[0]);

        if (s->modelPressed) {
            xModelSetLightKit(s->modelPressed, (xLightKit*)toParamWidget, toParam[0]);
        }
        break;

    default:
        DBprintf(DBML_VDBG, "zEntButton: Unknown event %d\n", toEvent);
        break;
    }
}
