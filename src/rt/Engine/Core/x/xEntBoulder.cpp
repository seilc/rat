#include "xEntBoulder.h"

#include "zMovePoint.h"
#include "xCam.h"
#include "xCollideSweptSphere.h"
#include "xDebug.h"
#include "xDraw.h"
#include "xHud.h"
#include "xGroup.h"
#include "xMemMgr.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xShadow.h"
#include "xUpdateCull.h"
#include "zCollGeom.h"
#include "zEntButton.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zGrid.h"
#include "zNPCCommon.h"
#include "zPad.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zShrapnel.h"
#include "zSurface.h"

#include "decomp.h"

#include <string.h>

static xEntCollis sBoulderCollis;

void xEntBoulder_FitToModel(xEntBoulder* ent)
{
    xASSERT_FMT_L(82, ent->model, "Boulder 0x%X doesn't have a model. This will crash the engine!", ent->asset->id);
    if (ent->model) {
        xVec3Copy(&ent->bound.sph.center, (xVec3*)&ent->model->Data->boundingSphere.center);
        ent->bound.sph.r = ent->model->Data->boundingSphere.radius;
    } else {
        xVec3Copy(&ent->bound.sph.center, &xVec3::m_Null);
        ent->bound.sph.r = 0.5f;
    }

    ent->fOOSphRadius = 1.0f / ent->bound.sph.r;
    if (ent->basset->flags & 0x400) {
        if (ent->basset->fSphereRadius > 0.0f) {
            ent->fOOSphRadius = 1.0f / ent->basset->fSphereRadius;
            ent->bound.sph.r = ent->basset->fSphereRadius;
        }
    }

    xVec3Copy(&ent->localCenter, &ent->bound.sph.center);
    xVec3AddTo(&ent->bound.sph.center, &ent->frame->mat.pos);
}

void xEntBoulder_Render(xEnt* ent)
{
    xModelInstance* model = ent->model;
    if (!model || !xEntIsVisible(ent) || (model->Flags & 0x400)) {
        return;
    }

    if (ent->flags & 0x20) {
        xModelRender(model);
    } else {
        S32 shadowResult;
        xVec3 shadVec;
        shadVec.x = ent->frame->mat.pos.x;
        shadVec.y = ent->frame->mat.pos.y - 10.0f;
        shadVec.z = ent->frame->mat.pos.z;
        if (!xModelSphereCullPlusShadow(iModelWorldBoundSphere(model->Data, (RwMatrix*)ent->frame, NULL), &shadVec, &shadowResult)) {
            xModelRender(model);
        }

        if (!shadowResult) {
            if (ent->flags & 0x10) {
                xShadowManager_Add(ent);
            } else {
                F32 radius = ent->model->Data->boundingSphere.radius;
                if (radius > 0.75f)
                    radius = 0.75f;
                xShadowSimple_Add(ent->simpShadow, ent, 2.0f * radius, 1.0f);
            }
        }
    }
}

void xEntBoulder_Init(void* ent, void* asset)
{
    xEntBoulder_Init((xEntBoulder*)ent, (xEntAsset*)asset);
}

void xEntBoulder_Init(xEntBoulder* ent, xEntAsset* asset)
{
    xEntInit(ent, asset);
    xEntBoulder_InitNoEntInit(ent, asset);
}

void xEntBoulder_InitNoEntInit(xEntBoulder* ent, xEntAsset* asset)
{
    ent->collType = 0x4;
    ent->collLev = 0x4;
    ent->bound.type = k_XBOUNDTYPE_SPHERE;
    ent->moreFlags |= 0x10;
    zEntParseModelInfo(ent, asset->modelInfoID);

    ent->entShadow = &ent->entShadow_embedded;
    ent->simpShadow = &ent->simpShadow_embedded;
    xShadowSimple_CacheInit(ent->simpShadow, ent, 0x50);
    xVec3Copy(&ent->entShadow->pos, &asset->pos);
    xVec3Init(&ent->entShadow->vec, 0.0f, 1.0f, 0.0f);
    ent->entShadow->shadowModel = NULL;

    ent->frame = (xEntFrame*)xMEMALLOC_L(sizeof(xEntFrame), 0, 1, 0, 191);
    memset(ent->frame, 0, sizeof(xEntFrame));

    ent->collis = NULL;

    xASSERT_L(198, ent);

    xEntBoulderAsset* basset = (xEntBoulderAsset*)(asset + 1);
    ent->basset = basset;
    ent->eventFunc = xEntBoulderEventCB;
    ent->update = (xEntUpdateCallback)xEntBoulder_Update;
    ent->bupdate = xEntBoulder_BUpdate;
    ent->render = xEntBoulder_Render;

    if (ent->linkCount)
        ent->link = (xLinkAsset*)((U8*)ent->asset + sizeof(xEntAsset) + sizeof(xEntBoulderAsset));
    else
        ent->link = NULL;

    if (ent->model) {
        xEntBoulder_FitToModel(ent);
    }

    if (basset->mass <= 0.0f) basset->mass = 1.0f;

    xEntBoulder_Reset(ent, globals.sceneCur);

    ent->currSndFX = (xSndHandle)-1;
    ent->lastRolling = 0;
    ent->rollingID = 0;

#if DEBUG || RELEASE
    if (ent->model) {
        xEntDebugAdd(ent);
    }
#endif

    ent->baseFlags |= (U16)0x20;

    if (basset->uSoundGroupHash)
        basset->soundID = xSndMgrGetSoundGroup(basset->uSoundGroupHash);
}

void xEntBoulder_ApplyForces(zPlayer* player, xEntCollis* collis)
{
    S32 i;
    xVec3 f;
    for (i = collis->dyn_sidx; i < collis->dyn_eidx; i++) {
        if (collis->colls[i].optr)
            if (((xBase*)collis->colls[i].optr)->baseType == eBaseTypeBoulder) {
                xEntBoulder* boul = (xEntBoulder*)collis->colls[i].optr;
                xVec3SMul(&f, &collis->colls[i].norm, -5.0f);
                xEntDrive* drv = player->GetDrive();
                if ((boul == drv->oldDriver || boul == drv->driver) && collis->colls[i].norm.y > 0.5f) {
                    f.x = -f.x;
                    f.z = -f.z;
                }
                xEntBoulder_AddForce(boul, &f);
                if (boul->basset->flags & 0x2)
                    player->Damage(zCombatDamageInfo(boul, 1, zHS_PROJECTILE, zHT_GENERAL, f, xVec3::m_Null, xVec3::m_Null, 0));
            }
    }
}

void xEntBoulder_AddInstantForce(xEntBoulder* ent, xVec3* force)
{
    if (ent->update) {
        xVec3AddTo(&ent->instForce, force);
    }
}

void xEntBoulder_AddForce(xEntBoulder* ent, xVec3* force)
{
    if (ent->update) {
        xVec3AddTo(&ent->force, force);
    }
}

void xEntBoulder_BUpdate(xEnt*, xVec3*)
{
}

void xEntBoulder_RealBUpdate(xEnt* ent, xVec3* pos)
{
    xEntBoulder* boul = (xEntBoulder*)ent;
    xVec3 rotatedLC;
    xMat3x3RMulVec(&rotatedLC, &boul->frame->mat, &boul->localCenter);
    xVec3Add(&boul->bound.sph.center, pos, &rotatedLC);

    xBoundUpdate(&boul->bound);
    zGridUpdateEnt(boul);
}

void xEntBoulder_Update(xEntBoulder* ent, xScene* sc, F32 dt)
{
    S32 i;
    xVec3 rotatedLC, oldPos, sweptDisp;

    if (ent->timeToLive > 0.0f) {
        ent->timeToLive -= dt;
        if (ent->timeToLive <= 0.0f) {
            zEntEvent(ent, eEventKill);
            return;
        }
    }

    if (ent->initNonCollideTime > 0.0f) {
        ent->initNonCollideTime -= dt;
        if (ent->initNonCollideTime <= 0.0f) {
            ent->collis_chk = 0x4e;
        }
    }

    if (!ent->collis) {
        ent->collis = &sBoulderCollis;
    }

    ent->collis->chk = ent->collis_chk;
    ent->collis->pen = ent->collis_pen;
    ent->collis->post = NULL;
    ent->collis->depenq = NULL;

    xMat3x3RMulVec(&rotatedLC, &ent->frame->mat, &ent->localCenter);
    xVec3Add(&ent->bound.sph.center, &ent->frame->mat.pos, &rotatedLC);

    xVec3Copy(&oldPos, &ent->bound.sph.center);

    ent->vel.y -= ent->basset->gravity * dt;
    ent->vel.AddScale(ent->instForce, 1.0f / ent->basset->mass);
    xVec3Init(&ent->instForce, 0.0f, 0.0f, 0.0f);
    ent->vel.AddScale(ent->force, dt / ent->basset->mass);
    xVec3Init(&ent->force, 0.0f, 0.0f, 0.0f);

    xVec3SMul(&sweptDisp, &ent->vel, dt);
    xVec3AddTo(&ent->bound.sph.center, &sweptDisp);

    xQuickCullForBound(&ent->bound.qcd, &ent->bound);
    zGridUpdateEnt(ent);

    xEntBeginUpdate(ent, sc, dt);

    ent->collType = 0x10;

    bool definitelyBounce = false;
    F32 sweptDistSqr = xVec3Length2(&sweptDisp);
    F32 penAllowed = 0.5f * ent->bound.sph.r;
    xSweptSphere sws;
    xSweptSpherePrepare(&sws, &oldPos, &ent->bound.sph.center, ent->bound.sph.r, 0);
    if (sweptDistSqr > xsqr(penAllowed)) {
        definitelyBounce = true;
        if (xSweptSphereToScene(&sws, globals.sceneCur, ent, 0x10, 0)) {
            F32 pen = sws.dist - sws.curdist;
            if (pen > penAllowed) {
                F32 scaleBack = (penAllowed + sws.curdist) / sws.dist - 1.0f;
                ent->bound.sph.center.AddScale(sweptDisp, scaleBack);
                oldPos.AddScale(sweptDisp, scaleBack);
                xQuickCullForBound(&ent->bound.qcd, &ent->bound);
                zGridUpdateEnt(ent);
            }
        }
    }

    xEntCollide(ent, sc, dt);

    ent->collType = 0x4;

    if (xEntBoulder_KilledBySurface(ent, sc, dt)) {
        if (ent->collis == &sBoulderCollis) {
            ent->collis = NULL;
        }
        return;
    }

    xEntBoulder* boulder = ent;
    S32 numDepens = 0;
    xVec3 depen;
    xVec3Init(&depen, 0.0f, 0.0f, 0.0f);
    if (ent->collis->env_eidx > ent->collis->env_sidx || ent->collis->dyn_eidx > ent->collis->dyn_sidx || ent->collis->npc_eidx > ent->collis->npc_sidx || ent->collis->stat_eidx > ent->collis->stat_sidx) {
        for (i = ent->collis->env_sidx; i < ent->collis->env_eidx; i++) {
            if (ent->basset->flags & 0x1) {
                xVec3AddTo(&depen, &ent->collis->colls[i].depen);
            } else {
                F32 penComp = xVec3Dot(&ent->collis->colls[i].norm, &ent->collis->colls[i].depen);
                depen.AddScale(ent->collis->colls[i].norm, penComp);
            }

            numDepens++;
        }

        for (i = ent->collis->dyn_sidx; i < ent->collis->dyn_eidx; i++) {
            xEnt* optr = (xEnt*)ent->collis->colls[i].optr;
            if (optr && optr->model && optr->model->Surf && ((zSurfaceProps*)optr->model->Surf->moprops)->asset->swimmable_surface) {
                continue;
            } else if (ent->basset->flags & 0x1) {
                xVec3AddTo(&depen, &ent->collis->colls[i].depen);
            } else {
                F32 penComp = xVec3Dot(&ent->collis->colls[i].norm, &ent->collis->colls[i].depen);
                depen.AddScale(ent->collis->colls[i].norm, penComp);
            }

            xEnt* xb = (xEnt*)ent->collis->colls[i].optr;
            if (xb->baseType == eBaseTypeBoulder) {
                xEntBoulder* boul = (xEntBoulder*)xb;
                xVec3 force, toAdd;
                xVec3Normalize(&force, &ent->collis->colls[i].depen);
                F32 v1 = xVec3Dot(&ent->vel, &force);
                F32 v2 = xVec3Dot(&boul->vel, &force);
                F32 forceMag = 2.0f * ent->basset->mass * boul->basset->mass * (v2 - v1) / (ent->basset->mass + boul->basset->mass);
                xVec3SMulBy(&force, forceMag);

                xVec3SMul(&toAdd, &force, ent->basset->bounce);
                xEntBoulder_AddInstantForce(ent, &toAdd);

                xVec3SMul(&toAdd, &force, -boul->basset->bounce);
                xEntBoulder_AddInstantForce(boul, &toAdd);
            }

            if (xb->baseType == eBaseTypeButton) {
                zEntButton_Press((zEntButton*)xb, 0);
                zEntButton_Hold((zEntButton*)xb, 0);
            }

            numDepens++;
        }

        for (i = ent->collis->stat_sidx; i < ent->collis->stat_eidx; i++) {
            xEnt* optr = (xEnt*)ent->collis->colls[i].optr;
            if (optr && optr->model && optr->model->Surf && ((zSurfaceProps*)optr->model->Surf->moprops)->asset->swimmable_surface) {
                continue;
            } else if (ent->basset->flags & 0x1) {
                xVec3AddTo(&depen, &ent->collis->colls[i].depen);
            } else {
                F32 penComp = xVec3Dot(&ent->collis->colls[i].norm, &ent->collis->colls[i].depen);
                depen.AddScale(ent->collis->colls[i].norm, penComp);
            }

            xEnt* xb = (xEnt*)ent->collis->colls[i].optr;
            if (!(ent->basset->flags & 0x4) || xb->baseType != eBaseTypeDestructObj)
                if (xb->moreFlags & 0x10) {
                    if (xb->baseType != eBaseTypeBoulder || (((xEntBoulder*)xb)->basset->flags & 0x100)) {
                        F32 hitParams[6] = { 25.0f, 12.0f, 0.0f, ent->vel.x, ent->vel.y, ent->vel.z };
                        zEntEvent(ent, xb, eEventHit, hitParams);
                    }
                }

            numDepens++;
        }

        for (i = ent->collis->npc_sidx; i < ent->collis->npc_eidx; i++) {
            zNPC::zNPCBaseIN2* npc = (zNPC::zNPCBaseIN2*)ent->collis->colls[i].optr;
            if (ent->basset->flags & 0x1) {
                xVec3AddTo(&depen, &ent->collis->colls[i].depen);
            } else {
                F32 penComp = xVec3Dot(&ent->collis->colls[i].norm, &ent->collis->colls[i].depen);
                depen.AddScale(ent->collis->colls[i].norm, penComp);
            }

            if (ent->basset->flags & 0x8) {
                ((zNPC::zNPCCommon*)ent->collis->colls[i].optr)->damage(zCombatDamageInfo(boulder, boulder->basset->hitpoints, zHS_PROJECTILE, zHT_GENERAL, xVec3::m_Null, xVec3::m_Null, xVec3::m_Null, 0));
                F32 hitParams[6] = { 25.0f, 12.0f, 0.0f, ent->vel.x, ent->vel.y, ent->vel.z };
                zEntEvent(ent, npc, eEventHit, hitParams);
            }

            numDepens++;
        }
    }

    zPlayer* player;
    sws.radius *= 0.95f;
    if (xSweptSphereToPlayer(&sws, globals.sceneCur, ent, &player, 0x4, 0x4)) {
        if (boulder->basset->flags & 0x2) {
            player->Damage(zCombatDamageInfo(boulder, boulder->basset->hitpoints, zHS_PROJECTILE, zHT_GENERAL, xVec3::m_Null, xVec3::m_Null, xVec3::m_Null, 0));
        } else {
            xSweptSphereGetResults(&sws);

            xVec3 playerDepen;
            playerDepen.Sub(sws.worldPos, sws.end);

            if (ent->basset->flags & 0x1) {
                xVec3AddTo(&depen, &playerDepen);
            } else {
                F32 penComp = xVec3Dot(&sws.worldNormal, &playerDepen);
                depen.AddScale(sws.worldNormal, penComp);
                numDepens++;
            }
        }
    }

    if (numDepens) {
        ent->bound.sph.center.AddScale(depen, 1.0f / numDepens);
    }

    xVec3 newVel;
    xVec3Sub(&newVel, &ent->bound.sph.center, &oldPos);
    xVec3SMul(&newVel, &newVel, 1.0f / dt);

    F32 newMag, oldMag;
    oldMag = xVec3Length(&ent->vel);
    newMag = xVec3Length(&newVel);
    if (oldMag > ent->basset->maxVel) oldMag = ent->basset->maxVel;
    if (newMag > oldMag) {
        xVec3SMulBy(&newVel, oldMag / newMag);
        newMag = oldMag;
    }

    xVec3 depenNorm;
    F32 depenComp = 0.0f;
    if (numDepens) {
        xVec3Normalize(&depenNorm, &depen);
        depenComp = xVec3Dot(&ent->vel, &depenNorm);
    }

    if (!(ent->basset->flags & 0x1)) {
        if (numDepens) {
            if (ent->basset->bounce && (depenComp / oldMag < -1.0f + ent->basset->bounce || definitelyBounce)) {
                F32 afterBounce = xVec3Dot(&newVel, &depenNorm);
                if (-depenComp > ent->basset->bounceDamp) {
                    newVel.AddScale(depenNorm, -ent->basset->bounce * depenComp - afterBounce);
                }
            }

            if (ent->basset->friction) {
                xVec3 fricComp;
                xVec3Copy(&fricComp, &ent->vel);
                fricComp.AddScale(depenNorm, -depenComp);
                newVel.AddScale(fricComp, -ent->basset->friction * dt);
            }
        }
    }

    xVec3Copy(&ent->vel, &newVel);

    if (numDepens) {
        if (ent->basset->flags & 0x400) {
            xMat4x3 baseMatrix;
            if (ent->basset->uBoneIndex)
                xMat4x3Mul(&baseMatrix, (xMat4x3*)&ent->model->Mat[ent->basset->uBoneIndex], (xMat4x3*)ent->model->Mat);
            else
                baseMatrix = *(xMat4x3*)ent->model->Mat;

            xVec3 vForcedRotationAxis;
            if (ent->basset->flags & 0x800) {
                vForcedRotationAxis = baseMatrix.right;
            }
            else if (ent->basset->flags & 0x1000) {
                vForcedRotationAxis = baseMatrix.up;
            } else {
                vForcedRotationAxis = baseMatrix.at;
            }

            F32 fMagnitudeAgainstAxis = xVec3Dot(&ent->vel, &vForcedRotationAxis);
            ent->vel.AddScale(vForcedRotationAxis, -fMagnitudeAgainstAxis);
        }

        xVec3 newRotVec;
        if (xVec3Length2(&depen) < 0.001f) {
            xVec3Init(&newRotVec, ent->vel.z, 0.0f, -ent->vel.x);
        } else {
            xVec3Cross(&newRotVec, &depen, &ent->vel);
        }

        xVec3Normalize(&newRotVec, &newRotVec);
        xVec3SMulBy(&ent->rotVec, 1.0f - ent->basset->stickiness);
        ent->rotVec.AddScale(newRotVec, ent->basset->stickiness);
        if (xVec3Normalize(&ent->rotVec, &ent->rotVec) < 0.00001f) {
            ent->angVel = 0.0f;
        } else {
            F32 newAngVel = ent->fOOSphRadius * xVec3Length(&ent->vel);
            ent->angVel = (1.0f - ent->basset->stickiness) * ent->angVel + ent->basset->stickiness * newAngVel;
        }
    }

    if (ent->angVel > ent->basset->maxAngVel) {
        ent->angVel = ent->basset->maxAngVel;
    }

    if (ent->angVel < -ent->basset->maxAngVel) {
        ent->angVel = -ent->basset->maxAngVel;
    }

    if (ent->angVel > 0.075f || ent->angVel < -0.075f) {
        xMat3x3 rotM;
        xMat3x3Rot(&rotM, &ent->rotVec, ent->angVel * dt);
        xMat3x3Mul(&ent->frame->mat, &ent->frame->mat, &rotM);
    }

    xMat3x3RMulVec(&rotatedLC, &ent->frame->mat, &ent->localCenter);
    xVec3Sub(&ent->frame->mat.pos, &ent->bound.sph.center, &rotatedLC);

    xEntEndUpdate(ent, sc, dt);

    if (ent->basset->soundID) {
        if (numDepens && ent->lastRolling > 15) {
            if (-depenComp > ent->basset->minSoundVel) {
                F32 vol = (-depenComp - ent->basset->minSoundVel) / (ent->basset->maxSoundVel - ent->basset->minSoundVel);
                if (vol > 1.0f) vol = 1.0f;

                ent->currSndFX = xSndMgrPlay(ent->basset->soundID, 0, NULL, NULL, ent, &vol, NULL);
            }
        }
    }

    if (numDepens) ent->lastRolling = 0;
    else ent->lastRolling++;

    if (ent->currSndFX) {
        if (ent->lastRolling > 15) {
            xSndMgrStop(ent->currSndFX);
        }
    }
    else if (numDepens) {
        if (ent->rollingID) {
            ent->currSndFX = xSndMgrPlay(xSndMgrGetSoundGroup(ent->rollingID), 0x800, NULL, NULL, ent, NULL, NULL);
        }
    }

    if (ent->collis == &sBoulderCollis) {
        ent->collis = NULL;
    }
}

S32 xEntBoulder_KilledBySurface(xEntBoulder* ent, xScene*, F32)
{
    if (!(ent->basset->flags & 0x70)) {
        return FALSE;
    }

    xCollis* coll = ent->collis->colls;
    xCollis* cend = coll + ent->collis->idx;

    for (; coll < cend; coll++) {
        if (coll->flags & 0x1) {
            xSurface* surf = zSurfaceGetSurface(coll);
            if (surf && surf->state == 0) {
                zSurfaceProps* prop = (zSurfaceProps*)surf->moprops;
                if (prop && prop->asset) {
                    if (ent->basset->flags & 0x10) {
                        switch (prop->asset->game_damage_type) {
                        case 0:
                            break;
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 6:
                            ent->hitpoints--;
                            if (ent->hitpoints <= 0) {
                                zEntEvent(ent, eEventKill);
                                return TRUE;
                            }
                            break;
                        case 5:
                            zEntEvent(ent, eEventKill);
                            return TRUE;
                        }
                    }

                    if (ent->basset->flags & 0x20) {
                        if (prop->asset->phys_flags & 0x10) {
                            zEntEvent(ent, eEventKill);
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    return FALSE;
}

void xEntBoulder_Kill(xEntBoulder* ent)
{
    if (ent) {
        ent->chkby = 0;
        ent->penby = 0;
        ent->collis_chk = 0;
        ent->collis_pen = 0;
        ent->update = NULL;
        xEntHide(ent);

        if (ent->currSndFX) {
            xSndMgrStop(ent->currSndFX);
        }
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("s");
#endif

void xEntBoulder_Setup(xEntBoulder* ent)
{
    xASSERT_L(820, ent);
    ent->asset->redMult = 1.0f;
    ent->asset->greenMult = 1.0f;
    ent->asset->blueMult = 1.0f;
    if (ent->model) {
        ent->asset->seeThru = ent->model->Alpha;
    } else {
        ent->asset->seeThru = 1.0f;
    }

    xEntSetup(ent);
}

void xEntBoulder_Reset(xEntBoulder* ent, xScene*)
{
    ent->initNonCollideTime = ent->basset->fInitNonCollideTime;
    ent->chkby = 0x1c;
    ent->penby = 0x10;
    ent->collis_pen = 0;
    if (0.0f == ent->initNonCollideTime) {
        ent->collis_chk = 0x4e;
    } else {
        ent->collis_chk = 0x48;
    }

    xEntReset(ent);
    xVec3Init(&ent->force, 0.0f, 0.0f, 0.0f);
    xVec3Init(&ent->instForce, 0.0f, 0.0f, 0.0f);
    xVec3Init(&ent->vel, 0.0f, 0.0f, 0.0f);
    xVec3Init(&ent->rotVec, 1.0f, 0.0f, 0.0f);
    ent->angVel = 0.0f;
    if (ent->basset->flags & 0x200)
        ent->timeToLive = ent->basset->killtimer;
    else
        ent->timeToLive = -1.0f;
    ent->hitpoints = ent->basset->hitpoints;
}

void xEntBoulderEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32)
{
    xASSERT_L(884, to);

    xEntBoulder* s = (xEntBoulder*)to;
    xASSERT_L(887, s->baseType == eBaseTypeBoulder || s->baseType == eBaseTypeFrozenProjectile);

    switch (toEvent) {
    case eEventVisible:
    case eEventFastVisible:
        xEntShow(s);
        break;
    case eEventInvisible:
    case eEventFastInvisible:
        xEntHide(s);
        break;
    case eEventCollisionOn:
        s->chkby |= (U8)0x10;
        break;
    case eEventCollisionOff:
        s->chkby &= (U8)~0x10;
        break;
    case eEventCollisionReset:
        s->chkby = 0x1c;
        break;
    case eEventCollision_Visible_On:
        s->chkby |= (U8)0x10;
        xEntShow(s);
        break;
    case eEventCollision_Visible_Off:
        s->chkby &= (U8)~0x10;
        xEntHide(s);
        break;
    case eEventCameraCollideOn:
        zCollGeom_CamEnable(s);
        break;
    case eEventCameraCollideOff:
        zCollGeom_CamDisable(s);
        break;
    case eEventDebugReset:
    case eEventReset:
        xEntBoulder_Reset(s, globals.sceneCur);
        if (xEntIsVisible(s)) {
            s->update = (xEntUpdateCallback)xEntBoulder_Update;
        }
        break;
    case eEventHit:
        if (s->update) {
            if (from->baseType == eBaseTypePlayer && !(s->basset->flags & 0x100)) {
                break;
            } else if (from && from->baseType == eBaseTypeNPC && !(s->basset->flags & 0x80)) {
                break;
            } else {
                s->hitpoints--;
                if (s->hitpoints <= 0)
                    zEntEvent(s, eEventKill);
            }
        }
        break;
    case eEventKill:
        xEntBoulder_Kill(s);
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
        zShrapnelAsset* shrap = (zShrapnelAsset*)toParamWidget;
        if (shrap && shrap->initCB) {
            shrap->initCB(shrap, s->model, NULL, &s->vel, NULL);
        }
        break;
    case eEventSetLightKit:
        xModelSetLightKit(s->model, (xLightKit*)toParamWidget, toParam[0]);
        break;
    default:
        DBprintf(DBML_VDBG, "xEntBoulder: Unknown event %d\n", toEvent);
        break;
    }
}

static void RecurseChild(xBase* child, xEntBoulder** boulList, S32& currBoul);

static S32 RecurseLinks(const xLinkAsset* link, S32 count, xEntBoulder** boulList)
{
    S32 i, numInList = 0;

    for (i = 0; i < count; i++) {
        const xLinkAsset* currLink = &link[i];
        xASSERT_L(998, currLink);
        if (currLink->dstEvent != eEventConnectToChild) continue;

        xBase* mychild = zSceneFindObject(currLink->dstAssetID);
        xASSERT_L(1002, mychild);
        if (mychild)
            RecurseChild(mychild, boulList, numInList);
    }

    return numInList;
}

static void RecurseChild(xBase* child, xEntBoulder** boulList, S32& currBoul)
{
    U32 i, cnt;

    switch (child->baseType) {
    case eBaseTypeBoulder:
        if (boulList) boulList[currBoul] = (xEntBoulder*)child;
        currBoul++;
        break;
    case eBaseTypeGroup:
        xGroup* grp = (xGroup*)child;
        cnt = xGroupGetCount(grp);
        for (i = 0; i < cnt; i++) {
            xBase* grpitem = xGroupGetItemPtr(grp, i);
            xASSERT_MSG_L(1029, grpitem, "Group missing content item");
            if (grpitem) {
                if (grpitem->baseType == eBaseTypeBoulder) {
                    if (boulList) boulList[currBoul] = (xEntBoulder*)grpitem;
                    currBoul++;
                }
                else if (grpitem->baseType == eBaseTypeGroup)
                    RecurseChild(grpitem, boulList, currBoul);
            }
        }
        break;
    default: break;
    }
}

void xBoulderGenerator_Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(1046, asset, asset_size, xBoulderGeneratorAsset, "game_object:BoulderGenerator");
    xBoulderGenerator_Init((xBoulderGenerator*)&data, (xBoulderGeneratorAsset*)&asset);
}

void xBoulderGenerator_Init(xBoulderGenerator* bg, xBoulderGeneratorAsset* asset)
{
    xBaseInit(bg, asset);

    bg->eventFunc = xBoulderGenerator_EventCB;
    bg->bgasset = asset;

    if (bg->linkCount)
        bg->link = (xLinkAsset*)(bg->bgasset + 1);
    else
        bg->link = NULL;

    U32 size = 0;
    bg->objectPtr = xSTFindAsset(asset->object, &size);
    if (size == sizeof(xVec3)) {
        bg->isMarker = TRUE;
    } else {
        bg->isMarker = FALSE;
        bg->objectPtr = zSceneFindObject(asset->object);
    }

    bg->numBoulders = RecurseLinks(bg->link, bg->linkCount, NULL);
    bg->nextBoulder = bg->numBoulders;

    bg->boulderList = (xEntBoulder**)xMEMALLOC_L(bg->numBoulders * sizeof(xEntBoulder*) + bg->numBoulders * sizeof(S32), 0, 0x26, 0, 1079);
    bg->boulderAges = (S32*)&bg->boulderList[bg->numBoulders];

    S32 boulCount = RecurseLinks(bg->link, bg->linkCount, bg->boulderList);
    xASSERT_L(1089, boulCount == bg->numBoulders);

    bg->initaxis.y = bg->bgasset->rotation.x;
    bg->initaxis.x = bg->bgasset->rotation.y;
    bg->initaxis.z = bg->bgasset->rotation.z;
    bg->angvel = (PI/180.0f) * xVec3Normalize(&bg->initaxis, &bg->initaxis);
    bg->lengthOfInitVel = xVec3Length(&bg->bgasset->initvel);
    if (bg->lengthOfInitVel > 0.00001f) {
        bg->perp1.x = bg->bgasset->initvel.z - bg->bgasset->initvel.y;
        bg->perp1.y = bg->bgasset->initvel.x - bg->bgasset->initvel.z;
        bg->perp1.z = bg->bgasset->initvel.y - bg->bgasset->initvel.x;
        xVec3Normalize(&bg->perp1, &bg->perp1);
        xVec3Cross(&bg->perp2, &bg->bgasset->initvel, &bg->perp1);
        xVec3SMulBy(&bg->perp1, bg->lengthOfInitVel);
    }

    xBoulderGenerator_Reset(bg);
}

void xBoulderGenerator_Reset(xBoulderGenerator* bg)
{
    for (S32 i = 0; i < bg->numBoulders; i++) {
        xEntBoulder_Kill(bg->boulderList[i]);
        bg->boulderAges[i] = 0;
    }
}

static void BoulderGen_GiveBirth(xBoulderGenerator* bg, S32 indx)
{
    for (S32 i = 0; i < bg->numBoulders; i++) {
        bg->boulderAges[i]++;
    }
    bg->boulderAges[indx] = 1;
}

static S32 GetBoulderForGenerating(xBoulderGenerator* bg)
{
    S32 i = 0;
    S32 j, oldestCulled = -1;
    S32 minAge = bg->numBoulders >> 1, numList = bg->numBoulders;

    for (j = 0; j < numList; j++) {
        i = j + bg->nextBoulder;
        if (i >= numList) i -= numList;
        if (!xEntIsVisible(bg->boulderList[i])) break;

        if (bg->boulderAges[i] >= minAge) {
            if (oldestCulled < 0) {
                oldestCulled = i;
            }
            else if (bg->boulderList[oldestCulled]->isCulled) {
                if (bg->boulderList[i]->isCulled && bg->boulderAges[oldestCulled] < bg->boulderAges[i])
                    oldestCulled = i;
            }
            else if (bg->boulderList[i]->isCulled || bg->boulderAges[oldestCulled] < bg->boulderAges[i]) {
                oldestCulled = i;
            }
        }
    }

    if (j == numList) {
        if (oldestCulled < 0) {
            i = (numList - 1) * xurand();
            if (i >= numList) i = numList - 1;
        } else {
            i = oldestCulled;
        }
    }

    bg->nextBoulder = i + 1;
    if (bg->nextBoulder >= numList) bg->nextBoulder = 0;
    return i;
}

void xBoulderGenerator_Launch(xBoulderGenerator* bg, xVec3* pnt, F32 t)
{
    S32 i = GetBoulderForGenerating(bg);
    xEntBoulder* b = bg->boulderList[i];
    if (b != bg->objectPtr || !xEntIsVisible(b)) {
        xEntBoulder_Reset(b, globals.sceneCur);
        b->update = (xEntUpdateCallback)xEntBoulder_Update;

        xEnt* genEnt = (xEnt*)bg->objectPtr;
        if (bg->isMarker) {
            xVec3Copy(&b->frame->mat.pos, (xVec3*)bg->objectPtr);
        } else {
            xVec3Copy(&b->frame->mat.pos, (xVec3*)&genEnt->model->Mat->pos);
            if (b->asset->modelInfoID == genEnt->asset->modelInfoID) {
                xMat3x3Copy(&b->frame->mat, (xMat3x3*)genEnt->model->Mat);
                xMat3x3Copy((xMat3x3*)b->model->Mat, (xMat3x3*)genEnt->model->Mat);
            }
        }

        if ((bg->bgasset->flags & 0x1) && !bg->isMarker) {
            xVec3 worldOffset;
            xMat3x3RMulVec(&worldOffset, (xMat3x3*)genEnt->model->Mat, &bg->bgasset->offset);
            xVec3AddTo(&b->frame->mat.pos, &worldOffset);
        } else {
            xVec3AddTo(&b->frame->mat.pos, &bg->bgasset->offset);
        }

        xVec3Copy(&b->rotVec, &bg->initaxis);
        if ((bg->bgasset->flags & 0x1) && !bg->isMarker) {
            xMat3x3RMulVec(&b->rotVec, (xMat3x3*)genEnt->model->Mat, &b->rotVec);
            xVec3Normalize(&b->rotVec, &b->rotVec);
        }

        b->angVel = bg->angvel;
    }

    xASSERT_FMT_L(1207, t != 0.0f, "BoulderGenerator '%s' [0x%08X] is givng boulder zero velocity!", xSceneID2Name(xglobals->sceneCur, bg->id), bg->id);
    if (0.0f == t)
        t = 2.0f;

    F32 invTime = 1.0f / t;
    b->vel.x = invTime * (pnt->x - b->frame->mat.pos.x);
    b->vel.z = invTime * (pnt->z - b->frame->mat.pos.z);
    b->vel.y = invTime * (pnt->y - b->frame->mat.pos.y) + 0.5f * (b->basset->gravity * t);

    BoulderGen_GiveBirth(bg, i);
}

void xBoulderGenerator_EventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32)
{
    xASSERT_L(1223, to);

    xBoulderGenerator* bg = (xBoulderGenerator*)to;
    xASSERT_L(1226, bg->baseType == eBaseTypeBoulderGenerator);

    xVec3 pnt;
    switch (toEvent) {
    case eEventGenerateBoulder:
        xBoulderGenerator_GenBoulder(bg);
        break;
    case eEventLaunchBoulderAtWidget:
        switch (toParamWidget->baseType) {
        case eBaseTypeVillain:
        case eBaseTypePlayer:
        case eBaseTypePickup:
        case eBaseTypePlatform:
        case eBaseTypeDoor:
        case eBaseTypeStatic:
        case eBaseTypeDynamic:
        case eBaseTypePendulum:
        case eBaseTypeHangable:
        case eBaseTypeButton:
        case eBaseTypeDestructObj:
        case eBaseTypeNPC:
        case eBaseTypeBoulder:
            xVec3Copy(&pnt, xEntGetPos((xEnt*)toParamWidget));

            if (toParam[1]) {
                pnt.x += toParam[1] * (2.0f * (xurand() - 0.5f));
                pnt.y += toParam[1] * (2.0f * (xurand() - 0.5f));
                pnt.z += toParam[1] * (2.0f * (xurand() - 0.5f));
            }

            xBoulderGenerator_Launch(bg, &pnt, toParam[0]);
            break;
        case eBaseTypeMovePoint:
            xVec3Copy(&pnt, zMovePointGetPos((zMovePoint*)toParamWidget));

            if (toParam[1]) {
                pnt.x += toParam[1] * (2.0f * (xurand() - 0.5f));
                pnt.y += toParam[1] * (2.0f * (xurand() - 0.5f));
                pnt.z += toParam[1] * (2.0f * (xurand() - 0.5f));
            }

            xBoulderGenerator_Launch(bg, &pnt, toParam[0]);
            break;
        }
        break;
    case eEventLaunchBoulderAtPoint:
        pnt.x = toParam[0];
        pnt.y = toParam[1];
        pnt.z = toParam[2];
        xBoulderGenerator_Launch(bg, &pnt, toParam[3]);
        break;
    case eEventLaunchBoulderAtPlayer:
        xVec3Copy(&pnt, (xVec3*)&((xEnt*)toParamWidget)->model->Mat->pos);
        pnt.AddScale(((xEnt*)toParamWidget)->frame->getDpos(), toParam[0] * toParam[1] / globals.update_dt);
        if (toParam[2]) {
            pnt.x += toParam[2] * (2.0f * (xurand() - 0.5f));
            pnt.y += toParam[2] * (2.0f * (xurand() - 0.5f));
            pnt.z += toParam[2] * (2.0f * (xurand() - 0.5f));
        }

        xBoulderGenerator_Launch(bg, &pnt, toParam[0]);
        break;
    case eEventDebugReset:
    case eEventReset:
        xBoulderGenerator_Reset(bg);
        break;
    default:
        DBprintf(DBML_VDBG, "xBoulderGenerator: Unknown event %d\n", toEvent);
        break;
    }
}

void xBoulderGenerator_GenBoulder(xBoulderGenerator* bg)
{
    S32 i = GetBoulderForGenerating(bg);
    xEntBoulder* b = bg->boulderList[i];
    xEnt* genEnt = (xEnt*)bg->objectPtr;

    if (b != bg->objectPtr || !xEntIsVisible(b)) {
        xEntBoulder_Reset(b, globals.sceneCur);

        b->update = (xEntUpdateCallback)xEntBoulder_Update;

        if (bg->isMarker) {
            xVec3Copy(&b->frame->mat.pos, (xVec3*)bg->objectPtr);
        } else {
            xVec3Copy(&b->frame->mat.pos, (xVec3*)&genEnt->model->Mat->pos);

            if (b->asset->modelInfoID == genEnt->asset->modelInfoID) {
                xMat3x3Copy(&b->frame->mat, (xMat3x3*)genEnt->model->Mat);
                xMat3x3Copy((xMat3x3*)b->model->Mat, (xMat3x3*)genEnt->model->Mat);
            }
        }

        if ((bg->bgasset->flags & 0x1) && !bg->isMarker) {
            xVec3 worldOffset;
            xMat3x3RMulVec(&worldOffset, (xMat3x3*)genEnt->model->Mat, &bg->bgasset->offset);
            xVec3AddTo(&b->frame->mat.pos, &worldOffset);
        } else {
            xVec3AddTo(&b->frame->mat.pos, &bg->bgasset->offset);
        }

        if (bg->bgasset->offsetRand) {
            b->frame->mat.pos.x += bg->bgasset->offsetRand * (2.0f * (xurand() - 0.5f));
            b->frame->mat.pos.y += bg->bgasset->offsetRand * (2.0f * (xurand() - 0.5f));
            b->frame->mat.pos.z += bg->bgasset->offsetRand * (2.0f * (xurand() - 0.5f));
        }

        xVec3Copy(&b->rotVec, &bg->initaxis);
        if ((bg->bgasset->flags & 0x1) && !bg->isMarker) {
            xMat3x3RMulVec(&b->rotVec, (xMat3x3*)genEnt->model->Mat, &b->rotVec);
            xVec3Normalize(&b->rotVec, &b->rotVec);
        }

        b->angVel = bg->angvel;
    }

    if (bg->lengthOfInitVel > 0.00001f && bg->bgasset->velAngleRand > 0.00001f) {
        F32 p1c = xurand() - 0.5f;
        F32 p2c = xurand() - 0.5f;
        F32 nf = 1.0f / xsqrt(xsqr(p1c) + xsqr(p2c));
        if (nf < 0.00001f) {
            p1c = 1.0f;
            p2c = 0.0f;
        } else {
            p1c *= nf;
            p2c *= nf;
        }

        xVec3 perpRand;
        xVec3SMul(&perpRand, &bg->perp1, p1c);
        perpRand.AddScale(bg->perp2, p2c);
        F32 randAng = bg->bgasset->velAngleRand * (xurand() - 0.5f);
        randAng = (PI/180.0f) * randAng;
        p1c = icos(randAng);
        p2c = isin(randAng);
        xVec3SMul(&b->vel, &bg->bgasset->initvel, p1c);
        b->vel.AddScale(perpRand, p2c);
    } else {
        xVec3Copy(&b->vel, &bg->bgasset->initvel);
    }

    if (bg->bgasset->velMagRand > 0.00001f) {
        if (bg->lengthOfInitVel > 0.00001f) {
            F32 sclMag = bg->bgasset->velMagRand * xurand() + 1.0f;
            xVec3SMulBy(&b->vel, sclMag);
        } else {
            b->vel.x = bg->bgasset->velMagRand * (2.0f * (xurand() - 0.5f));
            b->vel.y = bg->bgasset->velMagRand * (2.0f * (xurand() - 0.5f));
            b->vel.z = bg->bgasset->velMagRand * (2.0f * (xurand() - 0.5f));
        }
    }

    if ((bg->bgasset->flags & 0x1) && !bg->isMarker) {
        xMat3x3RMulVec(&b->vel, (xMat3x3*)genEnt->model->Mat, &b->vel);
    }

    BoulderGen_GiveBirth(bg, i);
}

#if DEBUG
static S32 myDebugID;
static U8 showUsage;
static U8 drawWire = TRUE;
static U8 cameraSnap;
static xEntBoulder* lastBoulder;
static xEntBoulder* selectedBoulder;
static xVec3 snapPos;
static xVec3 snapRot;

void xEntBoulderDebugModeCB()
{
    S32 i;
    bool selectNext, selectPrev;
    xEntBoulder* boul, *prevBoulder;

    if (globals.sceneCur) {
        zScene* zsc = globals.sceneCur;
        xprintf("\n  L2    Toggle usage\n");

        if (zPad::GetDebugPad()->Pressed(0x200)) {
            showUsage = !showUsage;
        }
        if (zPad::GetDebugPad()->Pressed(0x2000)) {
            drawWire = !drawWire;
        }

        if (showUsage) {
            xprintf("  O     Reset boulders\n");
            xprintf("  X     Double speed\n");
            xprintf("  SQ    Half speed\n");
            xprintf("  TRI   Reverse direction\n");
            xprintf("  R2    Toggle wireframe\n");
            xprintf("  LEFT  Select previous boulder\n");
            xprintf("  RIGHT Select next boulder\n");
            xprintf("  R3    Toggle camera snap\n");
            if (cameraSnap) {
                xprintf("\n");
                xprintf("  Left stick pushes\n");
                xprintf("  Right stick moves camera\n");
            }
        }

        if (selectedBoulder) {
            xprintf("\n< %2.2f , %2.2f , %2.2f >\n", selectedBoulder->bound.sph.center.x, selectedBoulder->bound.sph.center.y, selectedBoulder->bound.sph.center.z);
        }

        if (zPad::GetDebugPad()->Pressed(0x80)) {
            selectPrev = true;
            selectNext = false;
        } else if (zPad::GetDebugPad()->Pressed(0x20)) {
            selectNext = true;
            selectPrev = false;
        } else {
            selectNext = false;
            selectPrev = false;
        }

        if (zPad::GetDebugPad()->Pressed(0x4000)) {
            if (cameraSnap) cameraSnap = FALSE;
            else if (selectedBoulder) {
                cameraSnap = TRUE;
                xVec3Init(&snapRot, 0.0f, 0.0f, 0.0f);
            }
        }

        if (selectPrev && !selectedBoulder) {
            if (selectedBoulder) xEntShow(selectedBoulder);
            selectedBoulder = lastBoulder;
            selectPrev = false;
        }

        if (selectNext && selectedBoulder == lastBoulder) {
            if (selectedBoulder) xEntShow(selectedBoulder);
            selectedBoulder = NULL;
            selectNext = false;
        }

        prevBoulder = NULL;

        for (i = 0; i < zsc->num_base; i++) {
            if (zsc->base[i]->baseType != eBaseTypeBoulder) continue;

            boul = (xEntBoulder*)zsc->base[i];

            if (selectPrev && boul == selectedBoulder) {
                if (selectedBoulder) xEntShow(selectedBoulder);
                selectedBoulder = prevBoulder;
                selectPrev = false;
            } else if (selectNext && prevBoulder == selectedBoulder) {
                if (selectedBoulder) xEntShow(selectedBoulder);
                selectedBoulder = boul;
                selectNext = false;
            }

            if (zPad::GetDebugPad()->Pressed(0x20000) || (selectedBoulder && selectedBoulder->bound.sph.center.y < -300.0f)) {
                xEntBoulder_Reset(boul, zsc);
            }

            if (cameraSnap && boul == selectedBoulder) {
                xEntHide(boul);
            }

            if (!selectedBoulder || boul == selectedBoulder) {
                if (zPad::GetDebugPad()->Pressed(0x10000)) {
                    xVec3SMulBy(&boul->vel, 2.0f);
                }
                if (zPad::GetDebugPad()->Pressed(0x40000)) {
                    xVec3SMulBy(&boul->vel, -1.0f);
                }
                if (zPad::GetDebugPad()->Pressed(0x80000)) {
                    xVec3SMulBy(&boul->vel, 0.5f);
                }

                xDrawSetColor(0, 0, 0xff, 0xff);
                if (drawWire) {
                    if (!(boul->basset->flags & 0x400)) {
                        xBoundDraw(&boul->bound);

                        xVec3 tmp;
                        xVec3Copy(&tmp, &boul->bound.sph.center);
                        tmp.AddScale(boul->rotVec, boul->angVel);
                        xDrawSetColor(0xff, 0, 0, 0xff);
                        xDrawLine(&boul->bound.sph.center, &tmp);

                        xVec3Add(&tmp, &boul->bound.sph.center, &boul->vel);
                        xDrawSetColor(0, 0xff, 0, 0xff);
                        xDrawLine(&boul->bound.sph.center, &tmp);

                        xDrawSetColor(0, 0, 0xff, 0xff);
                        xDrawLine(&boul->frame->mat.pos, &boul->bound.sph.center);
                    } else {
                        xMat4x3 baseMatrix;
                        if (boul->basset->uBoneIndex)
                            xMat4x3Mul(&baseMatrix, (xMat4x3*)&boul->model->Mat[boul->basset->uBoneIndex], (xMat4x3*)&boul->model->Mat);
                        else
                            baseMatrix = boul->frame->mat;

                        xVec3 vForcedRotationAxis;
                        if (boul->basset->flags & 0x800) {
                            vForcedRotationAxis = baseMatrix.right;
                        }
                        else if (boul->basset->flags & 0x1000) {
                            vForcedRotationAxis = baseMatrix.up;
                        } else {
                            vForcedRotationAxis = baseMatrix.at;
                        }

                        xDrawSetColor(g_RED);
                        xDrawArrow(&baseMatrix.pos, &vForcedRotationAxis, false, 99.0f);
                        xDrawSetColor(g_WHITE);
                        if (boul->basset->fSphereRadius > 0.0f)
                            xDrawSphere(&boul->bound.sph.center, boul->basset->fSphereRadius, 0xc0006);
                        else
                            xBoundDraw(&boul->bound);
                    }
                }
            }

            prevBoulder = boul;
        }

        if (!lastBoulder) {
            lastBoulder = prevBoulder;
        }

        if (!selectedBoulder) cameraSnap = FALSE;

        if (cameraSnap) {
            xVec3Copy(&snapPos, &selectedBoulder->bound.sph.center);
            snapPos.y += 0.5f * selectedBoulder->bound.sph.r;

            xVec2 s1, s2;
            zPad::GetDebugPad()->GetAnalog1()->Get(s1.x, s1.y);
            zPad::GetDebugPad()->GetAnalog2()->Get(s2.x, s2.y);

            snapRot.x -= 0.05f * s2.x;
            if (snapRot.x > 2.0f * PI) snapRot.x -= 2.0f * PI;
            if (snapRot.x < 0.0f) snapRot.x += 2.0f * PI;
            snapRot.y -= 0.05f * s2.y;
            if (snapRot.y < -PI / 2) snapRot.y = -PI / 2;
            if (snapRot.y > PI / 2) snapRot.y = PI / 2;

            selectedBoulder->vel.AddScale((xVec3&)globals.screen->icam->viewMatrix.right, 0.15f * s1.x);
            selectedBoulder->vel.AddScale((xVec3&)globals.screen->icam->viewMatrix.at, -0.15f * s1.y);
        }
    }
}

void xEntBoulder_DebugID(S32 id)
{
    myDebugID = id;
}

static U8 was_tracking;
static U8 first_time = TRUE;

void xEntBoulder_SetSnapCamera()
{
    if (xDebugModeGet() != myDebugID || !selectedBoulder) {
        cameraSnap = FALSE;
    }

    if (cameraSnap && first_time) {
        first_time = FALSE;
    }

    if (!cameraSnap) {
        if (!first_time) {
            first_time = TRUE;
            was_tracking = FALSE;
        }
    }
}
#endif
