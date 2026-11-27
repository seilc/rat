#include "xCommon.h"
#include "zEGenerator.h"

#include "containers.h"
#include "xDraw.h"
#include "xEntMotion.h"
#include "xEvent.h"
#include "xFont.h"
#include "xMovePoint.h"
#include "xPad.h"
#include "xRand.h"
#include "xString.h"
#include "xstransvc.h"
#include "zCollGeom.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zLightning.h"
#include "zScene.h"
#include "zShrapnel.h"

#include <string.h>

struct xEntBoulder;

void zEGenerator_Init(void* egen, void* asset)
{
    zEGenerator_Init((zEGenerator*)egen, (xEntAsset*)asset);
}

void zEGenerator_Init(zEGenerator* egen, xEntAsset* asset)
{
    xASSERT_L(47, egen);
    xASSERT_L(48, asset);

    zEntInit(egen, asset, 'EGEN');

    zEGenAsset* zasset = (zEGenAsset*)asset;
    egen->zasset = zasset;

    if (egen->linkCount)
        egen->link = (xLinkAsset*)(zasset + 1);
    else
        egen->link = NULL;

    egen->update = (xEntUpdateCallback)zEGenerator_Update;
    egen->move = (xEntMoveCallback)zEGenerator_Move;
    egen->eventFunc = zEGeneratorEventCB;
    egen->render = (xEntRenderCallback)zEGenerator_Render;

    egen->afile = NULL;

    if (zasset->onAnimID) {
        void* buf;
        U32 size;
        buf = xSTFindAsset(zasset->onAnimID, &size);

        if (buf) {
            egen->afile = xAnimFileNew(buf, "", 0, NULL);
            xASSERT_L(78, egen->afile);

            egen->atbl = xAnimTableNew("", 0);
            xAnimTableNewState(egen->atbl, "On", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
            xAnimTableAddFile(egen->atbl, egen->afile, "On");
            xAnimPoolAlloc(&globals.sceneCur->mempool, egen, egen->atbl, egen->model);

            xASSERT_L(89, egen->model->Anim);
        }
    }
}

void zEGenerator_Setup(zEGenerator* egen, xScene*)
{
    zEntSetup(egen);

    egen->num_dsts = 0;
    for (S32 i = 0; i < egen->linkCount; i++) {
        const xLinkAsset* la = &egen->link[i];
        xBase* b = zSceneFindObject(la->dstAssetID);
        if (b && la->dstEvent == eEventArcto) {
            if (b->baseType == eBaseTypeMovePoint)
                egen->num_dsts++;
            else if (b->baseFlags & k_XBASE_IS_ENTITY) {
                egen->num_dsts++;
            } else {
                xVALIDATE_MSG_L(110, 0, "zEGenerator_Setup: can only arcto entities");
            }
        }
    }
}

void zEGenerator_Save(zEGenerator* ent, xSerial* s)
{
    zEntSave(ent, s);

    xASSERT_L(130, ent);
    xASSERT_L(131, s);
}

void zEGenerator_Load(zEGenerator* ent, xSerial* s)
{
    zEntLoad(ent, s);

    xASSERT_L(149, ent);
    xASSERT_L(150, s);
}

void zEGenerator_Reset(zEGenerator* egen, xScene*)
{
    zEntReset(egen);

    zEGenAsset* zasset = (zEGenAsset*)egen->asset;

    egen->flags = zasset->flags;

    if (egen->flags & 0x1) {
        zEGenerator_TurnOn(egen);
    } else {
        zEGenerator_TurnOff(egen);
    }

    if (zasset->moreFlags & 0x4) {
        egen->chkby = k_XENT_COLLTYPE_NONE;
    } else {
        egen->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
    }

    egen->bupdate(egen, &((xMat4x3*)egen->model->Mat)->pos);
}

void zEGenerator_Move(zEGenerator*, xScene*, F32)
{
}

void zEGenerator_Update(zEGenerator* egen, xScene* sc, F32 dt)
{
    xEntUpdate(egen, sc, dt);

    xMat4x3ToWorld(&egen->src_pos, (xMat4x3*)egen->model->Mat, &egen->zasset->src_dpos);
    xBase* b = egen->dst;
    if (b) {
        if (b->baseFlags & k_XBASE_IS_ENTITY) {
            zEnt* ent = (zEnt*)egen->dst;
            if (b->baseType == eBaseTypeBoulder) {
                xVec3Copy(&egen->dst_pos, xEntGetCenter(ent));
            } else {
                xMat4x3ToWorld(&egen->dst_pos, (xMat4x3*)ent->model->Mat, &egen->dst_off);
            }
        }
    }

    if (egen->lfx[0]) {
        zLightningModifyEndpoints(egen->lfx[0], &egen->src_pos, &egen->dst_pos);
    }
    if (egen->lfx[1]) {
        zLightningModifyEndpoints(egen->lfx[1], &egen->src_pos, &egen->dst_pos);
    }

    if (egen->tmr > 0.0f) {
        egen->tmr -= dt;
        if (egen->tmr <= 0.0f) {
            zEGenerator_TurnOff(egen);
            zEntEvent(egen, eEventOff);
        }
    }
}

void zEGenerator_Render(zEGenerator* egen)
{
    xEntRender(egen);
}

void zEGenerator_TurnOn(zEGenerator* egen)
{
    egen->flags |= 0x1;

    zEGenAsset* zasset = (zEGenAsset*)egen->asset;

    if (egen->afile) {
        xAnimPlay* aplay = egen->model->Anim;
        xASSERT_L(253, aplay);
        xASSERT_L(254, aplay->NumSingle == 1);
        xAnimSingle* asing = aplay->Single;

        asing->CurrentSpeed = 1.0f;
    }

    egen->tmr = zasset->ontime;

    xMat4x3ToWorld(&egen->src_pos, (xMat4x3*)egen->model->Mat, &zasset->src_dpos);
    if (egen->num_dsts) {
        U16 itgt = xrand_RandomInt32() % egen->num_dsts;
        U16 imp = 0;
        for (S32 i = 0; i < egen->linkCount; i++) {
            const xLinkAsset* link = &egen->link[i];
            U32 dstAssetID = link->dstAssetID;
            xBase* b = zSceneFindObject(dstAssetID);

            xVec3 destOffset;
            destOffset.x = link->param[0];
            destOffset.y = link->param[1];
            destOffset.z = link->param[2];
            xVec3Copy(&egen->dst_off, &destOffset);

            if (b) {
                if (link->dstEvent == eEventArcto) {
                    if (imp == itgt) {
                        if (b->baseType == eBaseTypeMovePoint) {
                            xMovePoint* mp = (xMovePoint*)b;
                            xVec3Copy(&egen->dst_pos, xMovePointGetPos(mp));
                            xVec3AddTo(&egen->dst_pos, &egen->dst_off);
                            egen->dst = b;
                            goto found;
                        } else if (b->baseType == eBaseTypeBoulder) {
                            xEntBoulder* boulder = (xEntBoulder*)b;
                            xVec3Copy(&egen->dst_pos, xEntGetCenter((xEnt*)boulder));
                            egen->dst = b;
                            goto found;
                        } else if (b->baseFlags & k_XBASE_IS_ENTITY) {
                            xEnt* ent = (xEnt*)b;
                            xMat4x3ToWorld(&egen->dst_pos, (xMat4x3*)ent->model->Mat, &egen->dst_off);
                            egen->dst = b;
                            goto found;
                        } else {
                            xVALIDATE_MSG_L(298, 0, "zEGenerator_TurnOn: can only arcto entities");
                        }
                    } else {
                        imp++;
                    }
                }
            }
        }
    }

    DBprintf(DBML_DBG, "zEGenerator_TurnOn: no dest links available!\n");

    xDrawSphere(&egen->dst_pos, 2.0f, 0xC0006);

found:
    for (S32 i = 0; i < k_MAX_LFX; i++) {
        if (egen->lfx[i]) {
            zLightningKill(egen->lfx[i]);
            egen->lfx[i] = NULL;
        }
    }

    xASSERT_EQ_L(323, k_MAX_LFX, 2);

    LightningAdd add;
    memset(&add, 0, sizeof(add));

    add.mainTexture = xStrHash("lightning");
    add.branchTexture = xStrHash("lightning");
    add.branchSpeed = 0.3f;

    add.type = eLT_Func;
    add.total_points = (xrand_RandomInt32() & 3) * 2;
    add.total_points += 8;
    add.start = &egen->src_pos;
    add.end = &egen->dst_pos;
    add.thickness = 0.3f;
    add.segmentsPerMeter = 3.0f;
    xColor addColor1 = { 0xC8, 0xC8, 0xFF, 0xC8 };
    add.color = addColor1;
    add.arc_height = -0.2f;
    add.zeus_normal_offset = 0.2f;
    add.zeus_back_offset = 0.1f;
    add.zeus_side_offset = 0.1f;
    add.rand_radius = 15.0f;
    add.flags = 0x10;

    egen->lfx[0] = zLightningAdd(&add);

    add.total_points = 16;
    add.type = eLT_Func;

    add.move_degrees = 360.0f * xurand() + 180.0f;
    if (xrand_RandomInt32() & 1)
        add.move_degrees = -add.move_degrees;
    add.setup_degrees = 90.0f * xurand() + 20.0f;

    xColor addColor2 = { 0x50, 0x64, 0xFF, 0xC8 };
    add.color = addColor2;
    add.rot_radius = 0.25f;
    add.thickness = 0.3f;
    add.segmentsPerMeter = 3.0f;
    add.rand_radius = 10.0f;
    add.flags = 0x10;

    egen->lfx[1] = zLightningAdd(&add);
}

void zEGenerator_TurnOff(zEGenerator* egen)
{
    egen->flags &= (U16)~0x1;

    if (egen->afile) {
        xAnimPlay* aplay = egen->model->Anim;
        xASSERT_L(395, aplay);
        xASSERT_L(396, aplay->NumSingle == 1);
        xAnimSingle* asing = aplay->Single;

        asing->CurrentSpeed = 0.0f;
    }

    for (S32 i = 0; i < k_MAX_LFX; i++) {
        if (egen->lfx[i]) {
            zLightningKill(egen->lfx[i]);
            egen->lfx[i] = NULL;
        }
    }
}

void zEGenerator_ToggleOn(zEGenerator* egen)
{
    if (egen->flags & 0x1) {
        zEGenerator_TurnOff(egen);
    } else {
        zEGenerator_TurnOn(egen);
    }
}

void zEGeneratorEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32)
{
    xASSERT_L(443, to);

    zEGenerator* egen = (zEGenerator*)to;

    switch (toEvent) {
    case eEventOn:
        zEGenerator_TurnOn(egen);
        break;

    case eEventOff:
        zEGenerator_TurnOff(egen);
        break;

    case eEventToggle:
        zEGenerator_ToggleOn(egen);
        break;

    case eEventReset:
    case eEventDebugReset:
        zEGenerator_Reset(egen, globals.sceneCur);
        break;

    case eEventVisible:
    case eEventFastVisible:
        xEntShow(egen);
        break;

    case eEventInvisible:
    case eEventFastInvisible:
        xEntHide(egen);
        break;

    case eEventCollision_Visible_On:
        xEntShow(egen);
    case eEventCollisionOn:
        egen->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        egen->bupdate(egen, &((xMat4x3*)egen->model->Mat)->pos);
        break;

    case eEventCollision_Visible_Off:
        xEntHide(egen);
    case eEventCollisionOff:
        egen->chkby = k_XENT_COLLTYPE_NONE;
        break;

    case eEventCollisionReset:
        if (egen->asset->moreFlags & 0x4) {
            egen->chkby = k_XENT_COLLTYPE_NONE;
        } else {
            egen->chkby = k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC;
        }
        egen->bupdate(egen, &((xMat4x3*)egen->model->Mat)->pos);
        break;

    case eEventCameraCollideOn:
        zCollGeom_CamEnable(egen);
        break;

    case eEventCameraCollideOff:
        zCollGeom_CamDisable(egen);
        break;

    case eEventLaunchShrapnel:
    {
        zShrapnelAsset* shrap = (zShrapnelAsset*)toParamWidget;
        if (shrap && shrap->initCB) {
            shrap->initCB(shrap, egen->model, NULL, NULL, NULL);
        }
        break;
    }

    case eEventSetLightKit:
        xModelSetLightKit(egen->model, (xLightKit*)toParamWidget, toParam[0]);
        break;
    }
}
