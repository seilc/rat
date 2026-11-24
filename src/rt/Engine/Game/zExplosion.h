#ifndef ZEXPLOSION_H
#define ZEXPLOSION_H

#include "xMath3.h"

namespace zExplosion {

struct presence_volume
{
    F32 knockback;
    xVec3 center;
    F32 radius;
    F32 damage_player;
    F32 damage_npc;
    F32 damage_other;
};

void scene_enter();
void scene_exit();
void reset();
void update(F32 dt);
void render();

}

#endif
