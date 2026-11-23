#include "xCommon.h"
#include "zAnimCache.h"

#include "xAnim.h"
#include "xModel.h"
#include "xutil.h"
#include "iAnim.h"
#include "iModel.h"
#include "zCombatAttack.h"

#include <cmath>

#define kMaxSamples 2560
#define kMaxCachedAnimations 128

namespace {

U32 cachedAnimations;
zAnimCacheEntry cachedEntry[kMaxCachedAnimations];
U32 currentCacheIndex;
xVec3 cache[kMaxSamples];

void fillInCache(zAnimCacheEntry* cacheEntry, RpAtomic* model, F32 startTime, F32 endTime)
{
    xVec3 tempPositions[IANIM_MAX_BONES];
    xQuat tempQuats[IANIM_MAX_BONES];
    xMat4x3 tempMats[IANIM_MAX_BONES];

    if (endTime < 0.0f) {
        endTime = cacheEntry->animFile->Duration;
    }

    currentCacheIndex += (U32)std::ceilf((endTime - startTime) / (1.0f / 240.0f));
    xASSERT_FMT_L(51, currentCacheIndex <= kMaxSamples, "The animation cache size needs to be increased to at least %i", currentCacheIndex);

    xVec3* currentCachePos = cacheEntry->data;
    for (F32 time = startTime; time <= endTime; time += (1.0f / 240.0f)) {
        xAnimFileEval(cacheEntry->animFile, time, NULL, 0, tempPositions, tempQuats, NULL);
        iModelAnimMatrices(model, tempQuats, tempPositions, (RwMatrix*)tempMats);
        *currentCachePos = tempMats[cacheEntry->bone + 1].pos;
        currentCachePos++;
    }
}

inline bool operator!=(const zAnimCacheEntry& entry1, const zAnimCacheEntry& entry2)
{
    return entry1.animFile != entry2.animFile || entry1.bone != entry2.bone;
}

} // namespace

void zAnimCacheClear()
{
    cachedAnimations = 0;
    currentCacheIndex = 0;
}

void zAnimCacheCreateData(zCombatAttack* attackTableStates, U32 attackTableCount, xAnimPlay* animPlay)
{
    xASSERT_FMT_L(84, attackTableStates && animPlay, "Invalid data passed into zAnimCacheCreateData");
    for (U32 i = 0; i < attackTableCount; i++) {
        if (attackTableStates[i].effect != 1) {
            continue;
        }

        for (U32 j = 0; j < ARRAY_LENGTH(attackTableStates[i].effectBones); j++) {
            attackTableStates[i].effectBones[j].positionCache = NULL;
            if (attackTableStates[i].effectBones[j].bone != 0xFFFF) {
                zAnimCacheEntry newCacheEntry;
                xAnimState* state = xAnimTableGetStateID(animPlay->Table, attackTableStates[i].state);
                xASSERT_FMT_L(94, state, "Invalid animation ID in the attack table state");
                if (!state)
                    continue;

                newCacheEntry.animFile = state->Data;
                newCacheEntry.bone = attackTableStates[i].effectBones[j].bone;
                U32 entryIndex = 0;
                while (entryIndex < cachedAnimations && cachedEntry[entryIndex] != newCacheEntry) {
                    entryIndex++;
                }

                if (entryIndex == cachedAnimations) {
                    xASSERT_FMT_L(106, cachedAnimations < kMaxCachedAnimations, "Sorry, the animation cache entry table is full. Please increase the size");
                    if (cachedAnimations >= kMaxCachedAnimations)
                        break;

                    cachedAnimations++;
                    cachedEntry[entryIndex] = newCacheEntry;
                    cachedEntry[entryIndex].data = &cache[currentCacheIndex];

                    fillInCache(&cachedEntry[entryIndex], animPlay->ModelInst->Data, attackTableStates[i].effectStart, attackTableStates[i].effectEnd);
                }

                attackTableStates[i].effectBones[j].positionCache = &cachedEntry[entryIndex];
            }
        }
    }
}
