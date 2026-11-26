#include "xCommon.h"
#include "zGrid.h"

#include "xEnt.h"
#include "xMath.h"
#include "xModel.h"
#include "xSurface.h"
#include "xutil.h"
#include "zBase.h"
#include "zHashes.h"
#include "zReactiveAnimation.h"
#include "zScene.h"
#include "zSurface.h"

xGrid colls_grid;
xGrid colls_oso_grid;
xGrid npcs_grid;
xGrid grabbable_grid;
static S32 zGridInitted;

static U32 special_models[] = {
    HASH_console_boomerang_monitor_glowcard_rs,
    0x0FD1D253,
    HASH_tree_banion_md_vines_ft,
    HASH_palmtree_a_new_ft,
    HASH_rock_cluster_small_ni,
    HASH_metal_prison_glow_ft,
    HASH_roomc_lights_a_ft,
    HASH_generator_switch_rs,
    HASH_light_hall_top_jspd_ft,
    HASH_tree_ti_new_ft,
    HASH_shadow_long_hs,
    HASH_leafclump_ft,
    HASH_flame_glow_floor_ld,
    HASH_leaves_groundcover_ft,
    HASH_console_boomerang_glowcard_rs,
    HASH_door_sign_a_rs,
    HASH_door_sign_b_rs,
    HASH_shadow_plant_hs,
    0x547C8C5C,
    HASH_vine_acrosspath_ft,
    HASH_bush_clump_ft,
    HASH_bush_clump_NJ,
    HASH_rock_cluster_new_ft,
    HASH_computer_blank_glow_rs,
    HASH_leaves_groundcover_new_ft,
    0x77AB7D1B,
    0x7893EE81,
    HASH_rooftop_junk_om,
    HASH_plant_tropical_test_ft,
    HASH_vine_short_ft,
    HASH_roomc_lightcards_a_ft,
    HASH_light_ray_ft,
    HASH_pipes_roof_electrical_om,
    HASH_light_hall_top_ft,
    HASH_computer_blank_wide_glow_rs,
    HASH_leafs_ground_nj,
    HASH_plant_leafy_new_ft,
    HASH_computer_blank_rs,
    0xB09D3EB5,
    HASH_tree_banion_md_top_ft,
    HASH_Bunker_DirtyB_FT,
    HASH_console_glow_B_FT,
    HASH_bunker_dirty_ft,
    HASH_rock_cluster_ft,
    HASH_rock_cluster_NJ,
    HASH_card_edgeburnb_ld,
    HASH_wall_decals_ft,
    0xDB1673B5,
    0xDB683F11,
    HASH_bush_clump_new_ft,
    HASH_palm_clump_new_ft,
    HASH_computer_screen_glow_b_rs,
    HASH_computer_screen_glow_c_rs,
    HASH_computer_screen_glow_d_rs,
    HASH_shadow_short_hs,
    HASH_computer_blank_wide_rs,
    HASH_debris_floorjunk_ld,
    HASH_rooftop_ventfloor_om,
};

static void hack_flag_shadows(zScene* s)
{
    const U32* end_special_models = special_models + ARRAY_LENGTH(special_models);

    xBase** it = s->base, **end = it + s->num_base;
    for (; it != end; it++) {
        xEnt* ent = (xEnt*)*it;
        if (!ent || !(ent->baseFlags & k_XBASE_IS_ENTITY) || !ent->asset)
            continue;

        for (U32* id = special_models; id != end_special_models; id++) {
            if (ent->asset->modelInfoID == *id) {
                ent->chkby |= k_XENT_COLLTYPE_UNK_0x80;
                ent->baseFlags |= k_XBASE_RECEIVES_SHADOWS;
                ent->asset->baseFlags |= k_XBASE_RECEIVES_SHADOWS;
                break;
            }
        }
    }
}

void zGridReset(zScene* s)
{
    hack_flag_shadows(s);

    for (S32 i = 0; i < s->num_base; i++) {
        xBase* base = s->base[i];

        if (base && (base->baseFlags & k_XBASE_IS_ENTITY) && base->baseType != eBaseTypeTrigger &&
            base->baseType != eBaseTypePlayer && ((xEnt*)base)->model) {
            xEnt* ent = (xEnt*)base;
            if (ent->bupdate) {
                ent->bupdate(ent, (xVec3*)&ent->model->Mat->pos);
            } else {
                xEntDefaultBoundUpdate(ent, (xVec3*)&ent->model->Mat->pos);
            }

            zGridUpdateEnt(ent);
        }
    }
}

void zGridInit(zScene* s)
{
    const xBox* ebox = xEntGetAllEntsBox();
    xASSERT_L(155, ebox);
    F32 grsizex = MAX(0.001f, ebox->upper.x - ebox->lower.x);
    F32 grsizez = MAX(0.001f, ebox->upper.z - ebox->lower.z);
    F32 defr = 2.5f;
    F32 min_csize = 4.0f * defr;
    xGridInit(
        &colls_grid, ebox, MAX(1.0f, MIN(32.0f, std::floorf(grsizex / min_csize))),
        MAX(1.0f, MIN(32.0f, std::floorf(grsizez / min_csize))), 1);

    xBox osobox = *ebox;
    osobox.lower.x -= 1.0f;
    osobox.lower.z -= 1.0f;
    osobox.upper.x += 3.4567f;
    osobox.upper.z += 3.4567f;
    grsizex = MAX(0.001f, osobox.upper.x - osobox.lower.x);
    grsizez = MAX(0.001f, osobox.upper.z - osobox.lower.z);

    min_csize *= 6.0f;
    xGridInit(
        &colls_oso_grid, &osobox, MAX(1.0f, MIN(8.0f, std::floorf(grsizex / min_csize))),
        MAX(1.0f, MIN(8.0f, std::floorf(grsizez / min_csize))), 2);

    xGridInit(
        &grabbable_grid, &osobox, MAX(1.0f, MIN(8.0f, std::floorf(grsizex / min_csize))),
        MAX(1.0f, MIN(8.0f, std::floorf(grsizez / min_csize))), 4);

    defr = 5.0f;
    min_csize = 4.0f * defr;
    xGridInit(
        &npcs_grid, ebox, MAX(1.0f, MIN(16.0f, std::floorf(grsizex / min_csize))),
        MAX(1.0f, MIN(16.0f, std::floorf(grsizez / min_csize))), 3);

    zGridInitted = TRUE;

    zGridReset(s);
}

void zGridExit(zScene*)
{
    xGridKill(&colls_grid);
    xGridKill(&colls_oso_grid);
    xGridKill(&npcs_grid);
    xGridKill(&grabbable_grid);
    zGridInitted = FALSE;
}

static bool IsEntityGrabbable(xEnt* ent)
{
    if (!ent || !ent->model || !ent->model->Surf || !ent->model->Surf->moprops) {
        return false;
    }

    zSurfaceProps* props = (zSurfaceProps*)ent->model->Surf->moprops;

    return (props->asset->phys_flags & 0x80) != 0;
}

void zGridUpdateEnt(xEnt* ent)
{
    if (!zGridInitted) {
        return;
    }

    bool oversize = false;
    xGrid* grid = ent->gridb.grid;
    if ((ent->chkby &
         (k_XENT_COLLTYPE_NPC | k_XENT_COLLTYPE_PC | k_XENT_COLLTYPE_INHIBIT | k_XENT_COLLTYPE_UNK_0x80)) ||
        ent->baseType == eBaseTypePickup || zReactiveIsReactive(*ent)) {
        if (grid) {
            if (!oversize) {
                xGridUpdate(grid, ent);
            }
        } else {
            if (ent->collType == k_XENT_COLLTYPE_NPC) {
                if (xGridEntIsTooBig(&npcs_grid, ent)) {
                    ent->gridb.oversize = 1;
                } else {
                    ent->gridb.oversize = 0;
                }
                xGridAdd(&npcs_grid, ent);
            } else {
                if (xGridEntIsTooBig(&colls_grid, ent)) {
                    if (xGridEntIsTooBig(&colls_oso_grid, ent)) {
                        ent->gridb.oversize = 2;
                    } else {
                        ent->gridb.oversize = 1;
                    }
                    xGridAdd(&colls_oso_grid, ent);
                } else {
                    xGridAdd(&colls_grid, ent);
                    ent->gridb.oversize = 0;
                }
            }
        }
    } else {
        if (IsEntityGrabbable(ent) == true) {
            if (grid && grid != &grabbable_grid) {
                xGridRemove(&ent->gridb);
            } else {
                if (!grid) {
                    xGridAdd(&grabbable_grid, ent);
                    ent->gridb.oversize = 0;
                }
            }
        } else {
            if (grid) {
                xGridRemove(&ent->gridb);
            }
        }
    }
}
