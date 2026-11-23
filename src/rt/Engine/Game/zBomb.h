#ifndef ZBOMB_H
#define ZBOMB_H

#include "zHitParameters.h"

struct xBound;
struct xEnt;
struct xVec3;
struct zPlayer;

void bound_damage(xBound& bound, xEnt* fromEnt, S32 player_damage, S32 obj_damage, S32 npc_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget);
void bound_damage_against_player(xBound& bound, xEnt* fromEnt, zPlayer* playerEnt, S32 player_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget); // UNUSED
void cylinder_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, F32 blastHeight_2, S32 player_damage, S32 npc_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget); // UNUSED
void sphere_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, S32 player_damage, S32 obj_damage, S32 npc_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget);
void sphere_damage_against_player(xEnt* fromEnt, zPlayer* playerEnt, const xVec3& epicenter, F32 blastRadius, S32 player_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget); // UNUSED

inline void bound_damage(xBound& bound, xEnt* fromEnt, S32 player_damage, S32 npc_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget); // UNUSED

inline void sphere_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, S32 player_damage, S32 npc_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget)
{
    sphere_damage(fromEnt, epicenter, blastRadius, player_damage, npc_damage, npc_damage, awayKnockback, upKnockback, hitSource, hitTarget);
}

inline void sphere_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, S32 damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget)
{
    sphere_damage(fromEnt, epicenter, blastRadius, damage, damage, awayKnockback, upKnockback, hitSource, hitTarget);
}

#endif
