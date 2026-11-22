#include "xCommon.h"
#include "zNPCFootsteps.h"

#include "containers.h"
#include "xCam.h"
#include "xEntMotion.h"
#include "xHud.h"
#include "xPtankPool.h"
#include "zNPCCommon.h"
#include "zPlayerContainer.h"
#include "zShrapnel.h"

#include <string.h>

using namespace zNPC;

zNPCFootstep* zNPCFootstepsManager::footstepTables[eNPCTypeCount];

void zNPCFootstepsManager::Init()
{
    memset(footstepTables, 0, sizeof(footstepTables));
}

// UNUSED: needed to match (code from RotU GC)
void zNPCFootstepsManager::RegisterTable(U32 npcType, zNPCFootstep* newTable)
{
    xASSERT(npcType < eNPCTypeCount);
    if (footstepTables[npcType] == NULL) {
        footstepTables[npcType] = newTable;
    } else {
        xASSERT_FMT(footstepTables[ npcType ] == newTable, "footsteps for npc type %d registered multiple times", npcType);
    }
}

void zNPCFootstepsManager::UpdateFootsteps(zNPCCommon* npc, F32 dt)
{
    zNPCFootstep* table = footstepTables[npc->get_type()];

    if (table == NULL) {
        return;
    }

    for (S32 i = 0; table[i].animID != 0; i++) {
        if (npc->model->Anim->Single->State->ID == table[i].animID) {
            F32 tym_animCurr = npc->manager.getCurrentBehavior()->getAnimTime();
            F32 tym_animDur = npc->manager.getCurrentBehavior()->getAnimDuration();
            F32 tym_animPlusTime = tym_animCurr + dt;
            xVec3 vel = xVec3::m_Null;

            if (tym_animPlusTime > table[i].animTime && tym_animCurr <= table[i].animTime) {
                if (table[i].frag) {
                    table[i].frag->initCB(table[i].frag, npc->model, NULL, &vel, NULL);
                }
            }

            if (tym_animCurr + table[i].animDuration > table[i].animTime && tym_animCurr <= table[i].animTime) {
                xMat4x3* mat = (xMat4x3*)&npc->model->Mat[table[i].boneID];
                xMat4x3 boneInWorld;
                xMat4x3Mul(&boneInWorld, mat, (xMat4x3*)npc->model->Mat);

                zFX::light_dust::emit(boneInWorld.pos, *xEntGetUpDirection(npc), table[i].emit_remaining, g_O3, dt,
                                      table[i].config, 1.0f, 1.0f);
            }
        }
    }
}
