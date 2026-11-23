#ifndef ZNPCLEVELSPECIFIC_H
#define ZNPCLEVELSPECIFIC_H

#include "types.h"

enum ezGameLevel
{
    ezGAMELEVEL_UT01,
    ezGAMELEVEL_DD01,
    ezGAMELEVEL_MT01,
    ezGAMELEVEL_BB01,
    ezGAMELEVEL_B101,
    ezGAMELEVEL_MM01,
    ezGAMELEVEL_RF01,
    ezGAMELEVEL_B201,
    ezGAMELEVEL_UW01,
    ezGAMELEVEL_CC01,
    ezGAMELEVEL_B301,
    ezGAMELEVEL_A10x,
    ezGAMELEVEL_A20x,
    ezGAMELEVEL_A30x,
    ezGAMELEVEL_Misc,
    ezGAMELEVEL_Total
};

extern ezGameLevel gNPCGameLevel;

void zNPCLevelSpecificSetup();
F32 zNPCLevelSpecificGetBlockChance(); // UNUSED
F32 zNPCLevelSpecificGetRoundRobinTimeOut();
F32 zNPCLevelSpecificGetShuffleWaitTimer(); // UNUSED
F32 zNPCLevelSpecificGetRunawayWaitTimer(); // UNUSED
F32 zNPCLevelSpecificGetRangerFireTimer(); // UNUSED
F32 zNPCLevelSpecificGetShieldRechargeRate(); // UNUSED
F32 zNPCLevelSpecificGetRunawayChance(); // UNUSED
S32 zNPCLevelSpecificGetTankAttackPattern(); // UNUSED
F32 zNPCLevelSpecificGetUpdateDistance();
F32 zNPCLevelSpecificGetSurrenderDistance();
bool zNPCLevelSpecificIsIceLevel(); // UNUSED
bool zNPCLevelSpecificIsMetalLevel(); // UNUSED
bool zNPCLevelSpecificIsDirtLevel(); // UNUSED

#endif
