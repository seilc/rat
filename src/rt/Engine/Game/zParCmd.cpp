#include "xCommon.h"
#include "zParCmd.h"

#include "xBound.h"
#include "xCollide.h"
#include "xDebug.h"
#include "xEnt.h"
#include "xFont.h"
#include "xMath.h"
#include "xPar.h"
#include "xParCmd.h"
#include "xParEmitter.h"
#include "xParGroup.h"
#include "xQuickCull.h"
#include "xRand.h"
#include "xString.h"
#include "xVec3.h"
#include "xVolumeAsset.h"
#include "zCombat.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zVolume.h"

#include <stdio.h>
#include <string.h>

#define k_DAMAGE_PLAYER_SKIP_MAX 10

#define MAX_CLIP_VOLUMES 32

static zVolume* sClipVolume[MAX_CLIP_VOLUMES];
static S32 sClipVolumeTotal;

void zParCmdInit()
{
    xParCmdInit();

    xParCmdRegister(eParCmdJet, sizeof(xParCmdJet), xParCmdJet_Update);
    xParCmdRegister(eParCmdPlayerCollision, sizeof(xParCmdPlayerCollision), xParCmdPlayerCollision_Update);
    xParCmdRegister(eParCmdCustom, sizeof(xParCmdCustom), xParCmdCustom_Update);
    xParCmdRegister(eParCmdClipVolumes, sizeof(xParCmdClipVolumes), xParCmdClipVolumes_Update);
    xParCmdRegister(eParCmdAnimalMagentism, sizeof(xParCmdAnimalMagentism), xParCmdAnimalMagentism_Update);
    xParCmdRegister(eParCmdDamagePlayer, sizeof(xParCmdDamagePlayer), xParCmdDamagePlayer_Update);
    xParCmdRegister(eParCmdApplyCamMat, sizeof(xParCmdApplyCamMat), xParCmdApplyCamMat_Update);
    xParCmdRegister(eParCmdKillDistance, sizeof(xParCmdKillDistance), xParCmdKillDistance_Update);
}

void zParCmdFindClipVolumes()
{
    char findname[64];

    memset(sClipVolume, 0, sizeof(sClipVolume));

    for (sClipVolumeTotal = 0; sClipVolumeTotal < MAX_CLIP_VOLUMES; sClipVolumeTotal++) {
        sprintf(findname, "PARTICLE_CLIP_%d", sClipVolumeTotal + 1);

        zVolume* vol = (zVolume*)zSceneFindObject(xStrHash(findname));
        if (!vol) {
            break;
        }

        sClipVolume[sClipVolumeTotal] = vol;
    }
}

void xParCmdKillDistance_Update(xParCmd* c, xParGroup* ps, F32)
{
    xPar* p;
    xParCmdKillDistance* cmd = (xParCmdKillDistance*)c->tasset;

    xASSERT_L(99, cmd);
    xASSERT_L(100, ps);

    xVec3* camera_pos = &globals.cam->mat.pos;
    xVec3 d;

    p = ps->m_root;

    if (cmd->kill_greater_than) {
        while (p) {
            xVec3Sub(&d, &p->m_pos, camera_pos);

            if (xVec3Length2(&d) > cmd->dSqr) {
                p->m_lifetime = -1.0f;
            }

            p = p->m_next;
        }
    } else {
        while (p) {
            xVec3Sub(&d, &p->m_pos, camera_pos);

            if (xVec3Length2(&d) < cmd->dSqr) {
                p->m_lifetime = -1.0f;
            }

            p = p->m_next;
        }
    }
}

void xParCmdClipVolumes_Update(xParCmd* c, xParGroup* ps, F32)
{
    xPar* p;
#if DEBUG
    xParCmdAlpha3rdPolyReg* cmd = (xParCmdAlpha3rdPolyReg*)c->tasset;
    xASSERT_L(144, cmd);
#endif

    xASSERT_L(146, ps);

    p = ps->m_root;
    while (p) {
        for (S32 i = 0; i < sClipVolumeTotal; i++) {
            zVolume* vol = sClipVolume[i];
            xASSERT_L(157, vol);

            xCollis collis;
            xVecHitsBound(&p->m_pos, &vol->asset->bound, &collis);
            if (collis.flags & k_HIT_IT) {
                p->m_lifetime = -1.0f;
            }
        }

        p = p->m_next;
    }
}

void xParCmdPlayerCollision_Update(xParCmd*, xParGroup*, F32)
{
}

void xParCmdAnimalMagentism_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdAnimalMagentism* cmd = (xParCmdAnimalMagentism*)c->tasset;

    xASSERT_L(234, cmd);
    xASSERT_L(235, ps);

    xVec3 pos = *xEntGetPos(CURPLAYERS[0]);

    pos.y += 1.0f;

    F32 mul = dt * -cmd->magnetism;
    p = ps->m_root;
    while (p) {
        xVec3 vec;
        vec.x = p->m_pos.x - pos.x;
        vec.y = p->m_pos.y - pos.y;
        vec.z = p->m_pos.z - pos.z;

        F32 dist = xVec3Normalize(&vec, &vec);

        p->m_vel.x += vec.x * mul;
        p->m_vel.y += vec.y * mul;
        p->m_vel.z += vec.z * mul;

        if (dist < 0.25f) {
            p->m_lifetime = -1.0f;
        }
        else if (dist < 2.0f) {
            dist *= 0.49f;
            p->m_vel.x *= dist;
            p->m_vel.y *= dist;
            p->m_vel.z *= dist;
        }

        p = p->m_next;
    }
}

void xParCmdDamagePlayer_Update(xParCmd* c, xParGroup* ps, F32)
{
    xPar* p;
    xParCmdDamagePlayer* cmd = (xParCmdDamagePlayer*)c->tasset;

    xASSERT_L(288, cmd);
    xASSERT_L(289, ps);
    xASSERT_GT_L(290, k_DAMAGE_PLAYER_SKIP_MAX, 0);

    xBound* pbound = &CURPLAYERS[0]->bound;

    S32 last_idx = k_DAMAGE_PLAYER_SKIP_MAX - (xrand_RandomInt32() & 1);
    p = ps->m_root;
    while (p && last_idx-- > 0) p = p->m_next;

    static xCollis collis;
    memset(&collis, 0, sizeof(collis));

    static xBound bnd_fake = { k_XBOUNDTYPE_SPHERE };
    memset(&bnd_fake, 0, sizeof(bnd_fake));
    bnd_fake.type = k_XBOUNDTYPE_SPHERE;

    while (p) {
        if (p->m_lifetime > 0.1f) {
            S32 hithim = 0;
            collis.flags = 0;

            if (p->m_size > 1.0f) {
                bnd_fake.sph.r = 0.5f * p->m_size;
                xVec3Copy(&bnd_fake.sph.center, &p->m_pos);
                xQuickCullForBound(&bnd_fake.qcd, &bnd_fake);
                xBoundHitsBound(&bnd_fake, pbound, &collis);
            } else {
                xVecHitsBound(&p->m_pos, pbound, &collis);
            }

            if (collis.flags & k_HIT_IT) {
                CURPLAYERS[0]->Damage(zCombatDamageInfo(NULL, 1, zHS_PROJECTILE, zHT_GENERAL, xVec3::m_Null, xVec3::m_Null, xVec3::m_Null, 0));
            }
        }

        for (S32 i = 0; i < cmd->granular; i++) {
            if (!p) break;
            p = p->m_next;
        }
    }
}

void xParCmdJet_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdJet* cmd = (xParCmdJet*)c->tasset;

    xASSERT_L(377, cmd);
    xASSERT_L(378, ps);

    F32 mdt = cmd->gravity * dt;

    xVec3 center;
    if (cmd->mode == 0) {
        center.x = CURPLAYERS[0]->model->Mat->pos.x;
        center.y = CURPLAYERS[0]->model->Mat->pos.y;
        center.z = CURPLAYERS[0]->model->Mat->pos.z;
    } else {
        center = cmd->center;
    }

    p = ps->m_root;
    while (p) {
        xVec3 dir;
        xVec3Sub(&dir, &center, &p->m_pos);

        F32 rSqr = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;

        if (rSqr < cmd->radiusSqr) {
            F32 oorSqr = mdt / xsqrt(rSqr + cmd->epsilon);

            dir.x = dir.x < 0.0f ? 1.0f : -1.0f;
            dir.z = dir.z < 0.0f ? 1.0f : -1.0f;

            p->m_vel.x += cmd->acc.x * oorSqr * dir.x;
            p->m_vel.y += cmd->acc.y * oorSqr;
            p->m_vel.z += cmd->acc.z * oorSqr * dir.z;
        }

        p = p->m_next;
    }
}

void xParCmdCustom_Grass_Update(xParCmd*, xParGroup* ps, F32 dt)
{
    xPar* p = ps->m_root;
    F32 radSqr = 2.0f;

    xVec3 center;
    center.x = CURPLAYERS[0]->model->Mat->pos.x;
    center.y = CURPLAYERS[0]->model->Mat->pos.y;
    center.z = CURPLAYERS[0]->model->Mat->pos.z;

    while (p) {
        xVec3 dir;
        xVec3Sub(&dir, &center, &p->m_pos);

        F32 rSqr = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;

        if (rSqr < radSqr) {
            p->m_size -= 4.0f * dt;
            if (p->m_size < 0.1f)
                p->m_size = 0.1f;
        } else {
            p->m_size += 0.01f * dt;
            if (p->m_size > 1.0f) {
                p->m_size = 1.0f;
            }
        }

        p = p->m_next;
    }
}

void xParCmdApplyCamMat_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdApplyCamMat* cmd = (xParCmdApplyCamMat*)c->tasset;

    xASSERT_L(494, cmd);
    xASSERT_L(495, ps);

    xMat4x3* mat = &globals.cam->mat;
    F32 mul;

    if (cmd->apply.x != 0.0f) {
        mul = dt * cmd->apply.x;
        p = ps->m_root;
        while (p) {
            p->m_vel.x += mat->right.x * mul;
            p->m_vel.y += mat->right.y * mul;
            p->m_vel.z += mat->right.z * mul;

            p = p->m_next;
        }
    }

    if (cmd->apply.y != 0.0f) {
        mul = dt * cmd->apply.y;
        p = ps->m_root;
        while (p) {
            p->m_vel.x += mat->up.x * mul;
            p->m_vel.y += mat->up.y * mul;
            p->m_vel.z += mat->up.z * mul;

            p = p->m_next;
        }
    }

    if (cmd->apply.z != 0.0f) {
        mul = dt * cmd->apply.z;
        p = ps->m_root;
        while (p) {
            p->m_vel.x += mat->at.x * mul;
            p->m_vel.y += mat->at.y * mul;
            p->m_vel.z += mat->at.z * mul;

            p = p->m_next;
        }
    }
}

xTWEAKABLE F32 STANK_SLOW_RATE = 0.98f;

void xParCmdCustom_Update(xParCmd* c, xParGroup* ps, F32 dt)
{
    xPar* p;
    xParCmdCustom* cmd = (xParCmdCustom*)c->tasset;

    xASSERT_L(560, cmd);
    xASSERT_L(561, ps);

    p = ps->m_root;

    switch (cmd->user_id) {
    case 0:
    {
        F32 mdt = 4.0f * dt, radSqr = 16.0f, epsilon = 0.001f;

        xVec3 center;
        center.x = CURPLAYERS[0]->model->Mat->pos.x;
        center.y = CURPLAYERS[0]->model->Mat->pos.y;
        center.z = CURPLAYERS[0]->model->Mat->pos.z;

        while (p) {
            xVec3 dir;
            xVec3Sub(&dir, &center, &p->m_pos);

            F32 rSqr = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;

            if (rSqr < radSqr) {
                F32 oorSqr = mdt / xsqrt(rSqr + epsilon);

                dir.x = dir.x < 0.0f ? 1.0f : -1.0f;
                dir.z = dir.z < 0.0f ? 1.0f : -1.0f;

                p->m_size = 1.0f / (10.0f * oorSqr * (dir.x + dir.z));
            }

            p = p->m_next;
        }
        break;
    }
    case 2:
        while (p) {
            p->m_vel.x = 0.0f;
            p->m_vel.y = 0.0f;
            p->m_vel.z = 0.0f;
            p = p->m_next;
        }
        break;
    case 10:
        xParCmdCustom_Grass_Update(c, ps, dt);
        break;
    case 11:
        while (p) {
            xVec3SMulBy(&p->m_vel, STANK_SLOW_RATE);
            p = p->m_next;
        }
        break;
    default:
        iprintf("xParCmdCustom_Update unknown user_id %d\n", cmd->user_id);
        break;
    }
}
