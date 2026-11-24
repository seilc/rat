#include "xCommon.h"
#include "zConfiguration.h"

#include "xRand.h"
#include "xString.h"
#include "xutil.h"

#include <string.h>

#define COUNT_MAX_SCENES 200

namespace {

char* player_model_info[PLAYER_COUNT][4] = {
    { "remy_bind.MINF", "remy_bind.MINF", "remy_bind.MINF", "remy_bind.MINF" },
    { "linguini_bind.MINF", "linguini_bind.MINF", "linguini_bind.MINF", "linguini_bind.MINF" },
    { "ratball_bind.MINF", "ratball_bind.MINF", "ratball_bind.MINF", "ratball_bind.MINF" },
    { "MG1_player_bind", "MG1_player_bind", "MG1_player_bind", "MG1_player_bind" },
    { "ratball_p0_bind.MINF", "ratball_p0_bind.MINF", "ratball_p0_bind.MINF", "ratball_p0_bind.MINF" },
    { "puff_player_bind.MINF", "puff_player_bind.MINF", "puff_player_bind.MINF", "puff_player_bind.MINF" },
    { "stomper_bind.MINF", "stomper_bind1.MINF", "stomper_bind.MINF", "stomper_bind.MINF" },
    { "ratball_p3_bind.MINF", "ratball_p3_bind.MINF", "ratball_p3_bind.MINF", "ratball_p3_bind.MINF" },
    { "avoidthestuff_p0_bind.MINF", "avoidthestuff_p0_bind.MINF", "avoidthestuff_p0_bind.MINF", "avoidthestuff_p0_bind.MINF" }
};

char* player_lorez_model[PLAYER_COUNT] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

const U32 player_tag[PLAYER_COUNT] = {
    IDTAG('P','L','Y','R'),
    IDTAG('P','L','Y','H'),
    IDTAG('P','L','Y','B'),
    IDTAG('P','L','Y','1'),
    IDTAG('P','L','Y','2'),
    IDTAG('P','L','Y','3'),
    IDTAG('P','L','Y','4'),
    IDTAG('P','L','Y','5'),
    0
};

struct zConfig_ScenePlayer_MapEntry
{
    U32 scene_tag;
    U32 scene_mask;
    PLAYER_ENUM player_enum[4];
    U32 pointerToPlayer[10];
    S32 playerCount;
    S32 pointerCount;
};

zConfig_ScenePlayer_MapEntry scene_player_map[COUNT_MAX_SCENES] = { 0 };

struct zConfigSceneMemReserve
{
    U32 sceneTag;
    S32 memReserve;
    bool enabled;
};

#if RELEASE
zConfigSceneMemReserve sceneMemReserveMap[COUNT_MAX_SCENES];
#endif

S32 scene_player_map_max;

} // namespace

U32 zConfig_getPlayerEnumIdx(U32 _player_tag)
{
    for (U32 i = 0; i < PLAYER_COUNT; i++) {
        if (_player_tag == player_tag[i])
            return i;
    }

    return 0;
}

// UNUSED: needed to match (code from Incredibles GC, Incredibles PS2)
void zConfig_setScenePlayer_MapEntry_Fixed(U32 sceneID, U32 _player_tag, U32 idx)
{
    for (S32 i = 0; i < scene_player_map_max; i++) {
        if (sceneID == scene_player_map[i].scene_tag) {
            scene_player_map[i].player_enum[idx] = (PLAYER_ENUM)zConfig_getPlayerEnumIdx(_player_tag);
            return;
        }
    }

    xFAIL_MSG(" Scene entry not found ");
}

void zConfig_setScenePlayer_MapEntry(U32 sceneID, U32 _player_tag)
{
    for (S32 i = 0; i < scene_player_map_max; i++) {
        if (sceneID == scene_player_map[i].scene_tag) {
            scene_player_map[i].player_enum[scene_player_map[i].playerCount] = (PLAYER_ENUM)zConfig_getPlayerEnumIdx(_player_tag);
            scene_player_map[i].playerCount++;
            return;
        }
    }

    U32 sceneMask = 0xFFFFFFFF;
    for (S32 i = 0; i < 4; i++)
        if (((sceneID >> (i * 8)) & 0xFF) == '?')
            sceneMask &= ~(0xFF << (i * 8));

    xASSERT_L(137, scene_player_map_max < COUNT_MAX_SCENES);
    scene_player_map[scene_player_map_max].scene_tag = sceneID;
    scene_player_map[scene_player_map_max].scene_mask = sceneMask;
    scene_player_map[scene_player_map_max].player_enum[scene_player_map[scene_player_map_max].playerCount] = (PLAYER_ENUM)zConfig_getPlayerEnumIdx(_player_tag);
    scene_player_map[scene_player_map_max].playerCount++;
    scene_player_map_max++;
}

// UNUSED: needed to match (code from Incredibles GC, Incredibles PS2)
U32 zConfig_findNextScene(U32 sceneID, U32 numChars, U32& startIdx)
{
    xASSERT(numChars && numChars < 5);

    for (U32 i = startIdx; i < scene_player_map_max; i++) {
        const U32& _tag = scene_player_map[i].scene_tag;
        char c0 = (_tag >> 24) & 0xFF,
             c1 = (_tag >> 16) & 0xFF,
             c2 = (_tag >> 8) & 0xFF,
             c3 = _tag & 0xFF,
             s0 = (sceneID >> 24) & 0xFF,
             s1 = (sceneID >> 16) & 0xFF,
             s2 = (sceneID >> 8) & 0xFF,
             s3 = sceneID & 0xFF;

        switch (numChars) {
        case 1:
            if (c0 == s0) {
                startIdx = i;
                return _tag;
            }
            break;
        case 2:
            if (c0 == s0 && c1 == s1) {
                startIdx = i;
                return _tag;
            }
            break;
        case 3:
            if (c0 == s0 && c1 == s1 && c2 == s2) {
                startIdx = i;
                return _tag;
            }
            break;
        case 4:
            if (c0 == s0 && c1 == s1 && c2 == s2 && c3 == s3) {
                startIdx = i;
                return _tag;
            }
            break;
        }
    }

    return 0;
}

U32 zConfig_getPlayerTag(U32 sceneID, S32 playerIdx)
{
    for (S32 i = 0; i < scene_player_map_max; i++) {
        U32 mask = scene_player_map[i].scene_mask;
        if ((sceneID & mask) == (mask & scene_player_map[i].scene_tag)) {
            xASSERT_L(204, playerIdx >= 0 && playerIdx < scene_player_map[i].playerCount);
            // BUG: always false, && instead of ||
            if (playerIdx < 0 && playerIdx >= scene_player_map[i].playerCount) {
                break;
            }

            return player_tag[scene_player_map[i].player_enum[playerIdx]];
        }
    }

    return player_tag[REMY];
}

const char* zConfig_getPlayerModelInfoString(U32 _tag, U32 idx)
{
    for (S32 i = 0; i < PLAYER_COUNT; i++) {
        if (_tag == player_tag[i])
            return player_model_info[i][idx];
    }

    return player_model_info[REMY][0];
}

U32 zConfig_getPlayerLorezModelID(U32 _tag)
{
    for (S32 i = 0; i < PLAYER_COUNT; i++) {
        if (_tag == player_tag[i]) {
            if (player_lorez_model[i])
                return xStrHash(player_lorez_model[i]);
            else
                return 0;
        }
    }

    return 0;
}

S32 zConfig_getPlayerCount(U32 sceneID)
{
    for (S32 i = 0; i < scene_player_map_max; i++) {
        if ((sceneID & scene_player_map[i].scene_mask) == (scene_player_map[i].scene_tag & scene_player_map[i].scene_mask)) {
            return scene_player_map[i].playerCount;
        }
    }

    xASSERT_FMT_L(255, 0, "No players defined for the scene. Check Rats.INI for player and scene mapping. Returning as if there was one player!");
    return 1;
}

PLAYER_ENUM zConfig_getPlayerEnum(U32 sceneID, S32 playerIdx)
{
    for (S32 i = 0; i < scene_player_map_max; i++) {
        if (sceneID == scene_player_map[i].scene_tag) {
            xASSERT_L(266, playerIdx >= 0 && playerIdx < scene_player_map[i].playerCount);
            // BUG: always false, && instead of ||
            if (playerIdx < 0 && playerIdx >= scene_player_map[i].playerCount) {
                break;
            }

            return scene_player_map[i].player_enum[playerIdx];
        }
    }

    return REMY;
}

S32 zConfig_getPlayers(U32 sceneID, U32* players)
{
    S32 i, j;
    *players = 0;
    S32 playerCount = 0;

    for (i = 0; i < scene_player_map_max; i++) {
        if (sceneID == scene_player_map[i].scene_tag) {
            for (j = 0; j < scene_player_map[i].playerCount; j++) {
                // BUG: = instead of |=
                *players = 1 << scene_player_map[i].player_enum[j];
            }

            return scene_player_map[i].playerCount;
        }
    }

    xASSERT_FMT_L(297, 0, "No players defined for the scene. Check RATS.INI for player and scene mapping. Returning as if there was one player!");
    *players = 1;
    return 1;
}

#if RELEASE
void zConfigInitMemReserveMap()
{
    memset(sceneMemReserveMap, 0, sizeof(sceneMemReserveMap));
}

void zConfigSetSceneMemReserveMapEntry(U32 sceneID, S32 memRes)
{
    for (S32 i = 0; i < COUNT_MAX_SCENES; i++) {
        if (sceneMemReserveMap[i].sceneTag == 0) {
            sceneMemReserveMap[i].sceneTag = sceneID;
            sceneMemReserveMap[i].memReserve = memRes;
            sceneMemReserveMap[i].enabled = true;
            return;
        }
    }

    xFAIL_MSG_L(322, "Too many scenes for memory piggy bank system.");
}

void zConfigDisableSceneMemReserveMapEntry(U32 sceneID)
{
    for (S32 i = 0; i < COUNT_MAX_SCENES; i++) {
        if (sceneMemReserveMap[i].sceneTag == 0) {
            sceneMemReserveMap[i].sceneTag = sceneID;
            sceneMemReserveMap[i].memReserve = -24 * 1024;
            sceneMemReserveMap[i].enabled = false;
            return;
        }
    }

    xFAIL_MSG_L(337, "Too many scenes for memory piggy bank system.");
}

S32 zConfigGetSceneMemReserve(U32 sceneID)
{
    for (S32 i = 0; i < COUNT_MAX_SCENES; i++) {
        if (sceneID == sceneMemReserveMap[i].sceneTag) {
            return sceneMemReserveMap[i].memReserve;
        }
    }

    return 0;
}
#endif
