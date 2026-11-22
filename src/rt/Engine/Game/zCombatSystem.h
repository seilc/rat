#ifndef ZCOMBATSYSTEM_H
#define ZCOMBATSYSTEM_H

#include "types.h"

struct xEnt;
struct xAnimPlay;
struct xAnimState;

void zCombatSystemInit();
void zCombatSystemReset();
void zCombatSystemFrameStart();
void zCombatSystemUpdate(F32 dt);
void zCombatSystemRenderEffects();
void zCombatSystemUpdateEntity(xEnt* ent, F32 dt);
void zCombatHitChooser(xAnimPlay*, xAnimState* state, void* object); // UNUSED

#endif
