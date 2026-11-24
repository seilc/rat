#ifndef ZTRANSIENT_H
#define ZTRANSIENT_H

#include "xBound.h"
#include "zHitParameters.h"

struct zFlame;

// zTransient::damage_result::type
#define k_ZTRANSIENT_TYPE_FLAME 0x1
#define k_ZTRANSIENT_TYPE_EXPLOSION 0x2
#define k_ZTRANSIENT_TYPE_ALL 0x3

namespace zTransient {

struct instance_data;

struct damage_result
{
    U32 flags;
    S32 type;
    F32 damage;
    F32 knockbackAmt;
    xVec3 knockbackDir;
    zHitSource source;
    xBound bound;
    union
    {
        void* object;
        zFlame* flame;
        instance_data* explosion;
    };
};

void scene_enter();
void scene_exit();
void setup();
void reset();
void update(F32 dt);
void render();
S32 check_damage(damage_result* results, const xSphere& o, S32 max_results, S32 type_filter, S32 coll_type); // UNUSED
S32 check_damage(damage_result* results, const xBox& box, S32 max_results, S32 coll_type, S32 type_filter);

}

#endif
