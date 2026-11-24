#include "xCommon.h"
#include "zTransient.h"

#include "containers.h"
#include "xEnt.h"
#include "xFont.h"
#include "xPad.h"
#include "xTransient.h"
#include "zExplosion.h"
#include "zFire.h"
#include "zFlameEmitter.h"
#include "zGlobals.h"
#include "zScene.h"

namespace {

void get_world_box(xBox& world_box)
{
    world_box = *xEntGetAllEntsBox();

    zScene& scene = *globals.sceneCur;
    xBox box;

    const zFlameEmitter* flame = (const zFlameEmitter*)scene.baseList[eBaseTypeFlameEmitter], *end_flame = flame + scene.baseCount[eBaseTypeFlameEmitter];

    for (; flame != end_flame; ++flame) {
        flame->get_bounding_box(box);
        xBoxUnion(world_box, world_box, box);
    }

    const xBox* jspbox = globals.sceneCur->env->geom->jsp_bound;
    const xBox* const end_jspbox = jspbox + globals.sceneCur->env->geom->jsp_count;
    for (; jspbox != end_jspbox; ++jspbox)
        xBoxUnion(world_box, world_box, *jspbox);
}

struct check_damage_data
{
    zTransient::damage_result* results;
    S32 coll_type;
    S32 type_filter;
    S32 max_results;
    S32 total;
    xBound bound;
};

bool cb_check_damage(xSpacePartitionTree2D::entry_data* e, void* context)
{
    check_damage_data* data = (check_damage_data*)context;
    if (!(e->type & data->type_filter))
        return true;

    xASSERT_L(56, data->total < data->max_results);

    zTransient::damage_result& result = data->results[data->total];

    switch (e->type) {
    case k_ZTRANSIENT_TYPE_FLAME:
    {
        const zFlame* flame = (const zFlame*)e->value;
        result.damage = flame->get_damage();

        if (data->coll_type & k_XENT_COLLTYPE_NPC)
            return true;
        if (result.damage <= 0.0f)
            return true;
        if (!flame->hits_bound(data->bound))
            return true;
        flame->get_knockback(result.knockbackDir, result.knockbackAmt, data->bound.box.center);
        result.source = zHS_FIRE;
        flame->get_bound(result.bound);
        break;
    }
    case k_ZTRANSIENT_TYPE_EXPLOSION:
        const zExplosion::presence_volume* plode = (const zExplosion::presence_volume*)e->value;
        switch (data->coll_type) {
        case k_XENT_COLLTYPE_NPC:
            result.damage = plode->damage_npc;
            break;
        case k_XENT_COLLTYPE_PC:
            result.damage = plode->damage_player;
            break;
        default:
            result.damage = plode->damage_other;
        }

        if (result.damage <= 0.0f)
            return true;
        if (!xSphereHitsBound(plode->center, plode->radius, data->bound))
            return true;
        result.knockbackAmt = plode->knockback;
        result.knockbackDir = xVec3::m_Null;
        result.source = zHS_EXPLOSION;
        xBoundFromSphere(result.bound, plode->center, plode->radius);
        break;
    default:
        xASSERT_L(104, 0);
        return false;
    }

    result.type = e->type;
    result.object = e->value;
    data->total++;

    return data->total < data->max_results;
}

}

namespace zTransient {

void scene_enter()
{
    xBox world_box;
    get_world_box(world_box);
    xTransient::scene_enter(world_box);

    zFire::scene_enter();
    zExplosion::scene_enter();
}

void scene_exit()
{
    zFire::scene_exit();
    zExplosion::scene_exit();

    xTransient::scene_exit();
}

void setup()
{
    zFire::setup();
}

void reset()
{
    xTransient::reset();
    zFire::reset();
    zExplosion::reset();
    zFlameEmitter::fire_reset();
}

void update(F32 dt)
{
    xTransient::debug_update(dt);
    zFire::update(dt);
    zExplosion::update(dt);
}

void render()
{
    zFire::render();
    zExplosion::render();
    xTransient::debug_render();
}

S32 check_damage(damage_result* results, const xBox& box, S32 max_results, S32 coll_type, S32 type_filter)
{
    xASSERT_L(188, max_results > 0);
    check_damage_data data;
    data.results = results;
    data.type_filter = type_filter & k_ZTRANSIENT_TYPE_ALL;
    data.max_results = max_results;
    data.total = 0;
    data.coll_type = coll_type;
    xBoundFromBox(data.bound, box);
    xTransient::space_tree.intersect_rect(box.lower.x, box.lower.z, box.upper.x, box.upper.z, cb_check_damage, &data);

    return data.total;
}

}
