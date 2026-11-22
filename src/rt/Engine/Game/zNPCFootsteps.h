#ifndef ZNPCFOOTSTEPS_H
#define ZNPCFOOTSTEPS_H

#include "zFXLightDust.h"
#include "zNPCTypes.h"

struct zShrapnelAsset;

namespace zNPC {

class zNPCCommon;

struct zNPCFootstep
{
    U32 animID;
    F32 animTime;
    F32 animDuration;
    U32 boneID;
    F32 emit_remaining;
    zShrapnelAsset* frag;
    zFX::light_dust::config* config;
};

class zNPCFootstepsManager
{
public:
    static void Init();
    static void RegisterTable(U32 npcType, zNPCFootstep* newTable); // UNUSED
    static void UpdateFootsteps(zNPCCommon* npc, F32 dt);

    static zNPCFootstep* footstepTables[eNPCTypeCount];
};

} // namespace zNPC

#endif
