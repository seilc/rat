#include "xCommon.h"
#include "zHitParameters.h"

#include "xString.h"

const zHitSourceMapEntry zHitSourceMap[zHS_COUNT + 1] = {
    { "EVENT", zHS_EVENT },
    { "GENERAL", zHS_GENERAL },
    { "PROJECTILE", zHS_PROJECTILE },
    { "EXPLOSION", zHS_EXPLOSION },
    { "LASER", zHS_LASER },
    { "ENERGY", zHS_ENERGY },
    { "FIRE", zHS_FIRE },
    { "SURFACE", zHS_SURFACE },
    { "MELEE_HIGH", zHS_MELEE_HIGH },
    { "MELEE_MID", zHS_MELEE_MID },
    { "MELEE_LOW", zHS_MELEE_LOW },
    { "MELEE_UP", zHS_MELEE_UP },
    { "MELEE_BACK", zHS_MELEE_BACK },
    { "MELEE_DIZZY", zHS_MELEE_DIZZY },
    { "THROW", zHS_THROW },
    { "WATER", zHS_WATER },
    { "DEATHPLANE", zHS_DEATHPLANE },
    { "KNOCKBACK", zHS_KNOCKBACK },
    { "LASERBEAM", zHS_LASERBEAM },
    { "INFINITE_FALL", zHS_INFINITE_FALL },
    { "ICE_GLIDE", zHS_ICE_GLIDE_LEVEL1 },
    { "INCREDISLAM", zHS_INCREDISLAM_LEVEL1 },
    { "PUNCH1", zHS_PUNCH_MRI_1_LEVEL1 },
    { "PUNCH2", zHS_PUNCH_MRI_2_LEVEL1 },
    { "PUNCH3", zHS_PUNCH_MRI_3_LEVEL1 },
    { "PUNCH4", zHS_PUNCH_FRO_1_LEVEL1 },
    { "PUNCH5", zHS_PUNCH_FRO_2_LEVEL1 },
    { "LEDGE_UP", zHS_LEDGE_UP },
    { "DODGE_ROLL", zHS_DODGE_ROLL_LEVEL1 },
    { NULL, zHS_GENERAL }
};

U32 zHitSourceCCs[zHS_COUNT] = {
    'EV\0\0',
    'GE\0\0',
    'PR\0\0',
    'EX\0\0',
    'LA\0\0',
    'EN\0\0',
    'FI\0\0',
    'SU\0\0',
    'MH\0\0',
    'MM\0\0',
    'ML\0\0',
    'MU\0\0',
    'MB\0\0',
    'MD\0\0',
    'TH\0\0',
    'WA\0\0',
    'DP\0\0',
    'KB\0\0',
    'LB\0\0',
    'IF\0\0',
    'IG\0\0',
    'IS\0\0',
    'P1\0\0',
    'P2\0\0',
    'P3\0\0',
    'P4\0\0',
    'P5\0\0',
    'LU\0\0',
    'DR\0\0'
};

S32 getSourceCCIdx(U32 value)
{
    for (S32 i = 0; i < zHS_COUNT; i++) {
        char tag[4];
        tag[0] = (zHitSourceCCs[i] >> 24) & 0xFF;
        tag[1] = (zHitSourceCCs[i] >> 16) & 0xFF;
        tag[2] = (zHitSourceCCs[i] >> 8) & 0xFF;
        tag[3] = zHitSourceCCs[i] & 0xFF;
        if (value == xStrHash(tag))
            return i;
    }
    return -1;
}

#if DEBUG || RELEASE
// UNUSED: needed to match (speculative code)
char* zHitSourceNames[zHS_COUNT] = {
    "zHS_EVENT",
    "zHS_GENERAL",
    "zHS_PROJECTILE",
    "zHS_EXPLOSION",
    "zHS_LASER",
    "zHS_ENERGY",
    "zHS_FIRE",
    "zHS_SURFACE",
    "zHS_MELEE_HIGH",
    "zHS_MELEE_MID",
    "zHS_MELEE_LOW",
    "zHS_MELEE_UP",
    "zHS_MELEE_BACK",
    "zHS_MELEE_DIZZY",
    "zHS_THROW",
    "zHS_WATER",
    "zHS_DEATHPLANE",
    "zHS_KNOCKBACK",
    "zHS_LASERBEAM",
    "zHS_INFINITE_FALL",
    "zHS_ICE_GLIDE",
    "zHS_INCREDISLAM",
    "zHS_PUNCH1",
    "zHS_PUNCH2",
    "zHS_PUNCH3",
    "zHS_PUNCH4",
    "zHS_LEDGE_UP",
    "zHS_DODGE_ROLL"
};

char* zHitTargetNames[zHT_COUNT] = {
    "zHT_GENERAL",
    "zHT_FRONT",
    "zHT_BACK",
    "zHT_LEFT",
    "zHT_RIGHT",
    "zHT_ENTITY",
    "zHT_ENV"
};
#endif
