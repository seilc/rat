#include "xCommon.h"
#include "zBomb.h"

#include "containers.h"
#include "xBound.h"
#include "xCam.h"
#include "xCollide.h"
#include "xEnt.h"
#include "xEntMotion.h"
#include "xGlobals.h"
#include "xGrid.h"
#include "xHud.h"
#include "xPtankPool.h"
#include "xSpacePartitionTree.h"
#include "zCombat.h"
#include "zGrid.h"
#include "zNPCCommon.h"
#include "zPlayer.h"
#include "zScene.h"

#include <string.h>

namespace
{
    inline bool boundHitsBound(const xBound& bound1, const xBound& bound2, xVec3& hitPos)
    {
        xCollis coll;
        memset(&coll, 0, sizeof(coll));
        coll.flags = k_HIT_CALC_TOHIT;
        xBoundHitsBound(&bound1, &bound2, &coll);
        if (coll.flags & k_HIT_IT) {
            hitPos.Add(coll.tohit, *xBoundCenter(&bound1));
            return true;
        }

        return false;
    }

    xVec3 getKnockback(const xBound* explosion, const xVec3& target, F32 away_speed, F32 up_speed)
    {
        xVec3 knockback;
        xVec3Sub(knockback, target, explosion->sph.center);
        knockback.y = 0.0f;
        knockback.safe_normalize(xVec3::m_Null);
        xVec3ScaleC(knockback, away_speed);
        knockback.AddScale(xVec3::m_UnitAxisY, up_speed);
        return knockback;
    }

    class damage_callback
    {
        xEnt* from;
        xBound* bound;
        S32 damage;
        F32 away_speed;
        F32 up_speed;
        zHitSource hit_source;
        zHitTarget hit_target;

    public:
        damage_callback(xEnt* from, xBound* bound, S32 damage, F32 away_speed, F32 up_speed, zHitSource hit_source, zHitTarget hit_target)
            : from(from), bound(bound), damage(damage), away_speed(away_speed), up_speed(up_speed), hit_source(hit_source), hit_target(hit_target) {}

        bool operator()(xEnt& ent)
        {
            if (!ent.chkby) {
                return true;
            }

            xVec3 hit_pos;
            if (from != &ent && boundHitsBound(*bound, ent.bound, hit_pos)) {
                xVec3 hit = getKnockback(bound, *xEntGetPos(&ent), away_speed, up_speed);
                zCombatDamage(&ent, zCombatDamageInfo(from, damage, hit_source, hit_target, hit, hit_pos, xVec3::m_Null, 0));
            }

            return true;
        }
    };
} // namespace

void bound_damage(xBound& bound, xEnt* fromEnt, S32 player_damage, S32 obj_damage, S32 npc_damage, F32 awayKnockback,
                  F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget)
{
    xQuickCullForBound(&bound.qcd, &bound);

    if (obj_damage > 0) {
        damage_callback cb(fromEnt, &bound, obj_damage, awayKnockback, upKnockback, hitSource, hitTarget);

        xGridCheckBound(colls_grid, bound, bound.qcd, cb);
        xGridCheckBound(colls_oso_grid, bound, bound.qcd, cb);
    }

    if (npc_damage > 0) {
        damage_callback cb(fromEnt, &bound, npc_damage, awayKnockback, upKnockback, hitSource, hitTarget);
        xGridCheckBound(npcs_grid, bound, bound.qcd, cb);
    }

    if (player_damage > 0) {
        for (S32 i = 0; i < CURPLAYERS.size(); i++) {
            if (fromEnt == CURPLAYERS[i]) {
                continue;
            }

            const xSphere* player_spheres[5];
            S32 num_spheres = CURPLAYERS[i]->GetCollisionSpheres(player_spheres);
            for (S32 j = 0; j < num_spheres; j++) {
                xBound test_bound;
                test_bound.type = k_XBOUNDTYPE_SPHERE;
                test_bound.sph = *player_spheres[j];
                xQuickCullForBound(&test_bound.qcd, &test_bound);

                xVec3 hit_pos;
                if (boundHitsBound(bound, test_bound, hit_pos)) {
                    xVec3 hit = getKnockback(&bound, *xEntGetPos(CURPLAYERS[i]), awayKnockback, upKnockback);
                    zCombatDamage(CURPLAYERS[i], zCombatDamageInfo(fromEnt, player_damage, hitSource, hitTarget, hit,
                                                                   hit_pos, xVec3::m_Null, 0));
                }
            }
        }
    }
}

void sphere_damage(xEnt* fromEnt, const xVec3& epicenter, F32 blastRadius, S32 player_damage, S32 obj_damage,
                   S32 npc_damage, F32 awayKnockback, F32 upKnockback, zHitSource hitSource, zHitTarget hitTarget)
{
    xBound bound;
    bound.type = k_XBOUNDTYPE_SPHERE;
    bound.sph.center = epicenter;
    bound.sph.r = blastRadius;
    bound_damage(bound, fromEnt, player_damage, obj_damage, npc_damage, awayKnockback, upKnockback, hitSource,
                 hitTarget);
}
