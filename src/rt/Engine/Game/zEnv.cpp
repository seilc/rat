#include "xCommon.h"
#include "zEnv.h"

#include "xDebugTweak.h"
#include "xEnt.h"
#include "xEnv.h"
#include "xFX.h"
#include "xGroup.h"
#include "xOutputMgr.h"
#include "xstransvc.h"
#include "xSurface.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zSurface.h"

xTWEAKABLE bool renderJSPs = true;

void zEnvInit(void* env, void* easset)
{
    zEnvInit((zEnv*)env, (xEnvAsset*)easset);
}

void zEnvInit(zEnv* env, xEnvAsset* easset)
{
    xBaseInit(env, easset);

    xTWEAK_BOOL("Debugging Options|Show All JSPs", &iEnvAllJSPSActive, NULL, NULL, 0);
    xTWEAK_BOOL("Debugging Options|Performance|Render JSPs", &renderJSPs, NULL, NULL, 0);

    env->easset = easset;
    env->eventFunc = zEnvEventCB;

    if (env->linkCount) {
        env->link = (xLinkAsset*)(env->easset + 1);
    }

    globals.sceneCur->zen = env;
    globals.sceneCur->zen = env;

    xEnvLoadJSPList(globals.sceneCur->env, 0);

    S32 mapper_count = xSTAssetCountByType('MAPR');
    for (S32 i = 0; i < mapper_count; i++) {
        U32 size;
        xBase* material_map = (xBase*)xSTFindAssetByType('MAPR', i, &size);

        zSurfaceRegisterMapper(material_map->id);
    }

    if (globals.sceneCur->env) {
        if (!globals.sceneCur->env->geom) {
            xFAIL_MSG_L(118, "NO BSP?????  WTF!!!!????");
        }
    }

    xEntGrowGlobalBoundingBox(&easset->minBounds);
    xEntGrowGlobalBoundingBox(&easset->maxBounds);

    if (easset->flags & 0x1) {
        for (S32 i = 0; i < globals.sceneCur->env->geom->jsp_count; i++) {
            xEntGrowGlobalBoundingBox(&globals.sceneCur->env->geom->jsp_bound[i].upper);
            xEntGrowGlobalBoundingBox(&globals.sceneCur->env->geom->jsp_bound[i].lower);
        }
    }

    xOutInfo("ENV", "zEnv: JSPs load complete\n");
}

void zEnvSetup(zEnv* env)
{
    xASSERT_L(142, env);
    xASSERT_L(143, env->easset);

    xBaseSetup(env);

    iEnvLightingBasics(globals.sceneCur->env->geom, env->easset);
}

void zEnvStartingCamera(zEnv* env)
{
    xASSERT_L(158, env);
    xASSERT_L(159, env->easset);
}

void zEnvRender(xEnv* env)
{
    if (renderJSPs) {
        xASSERT_L(171, env);
        xASSERT_L(172, env->geom);
        xASSERT_L(173, env->geom->world);

        RpWorld* world = env->geom->world;

        S32 num = RpWorldGetNumMaterials(world);
        for (S32 i = 0; i < num; i++) {
            const xSurface* sp = zSurfaceGetSurface(i);
            zSurfaceProps* pp = (zSurfaceProps*)sp->moprops;

            if (!pp || !pp->asset) {
                continue;
            }

            if (pp->texanim_flags & SURF_TEXANIM_ON) {
                S32 tdx = 0;
                RpMaterial* mp = RpWorldGetMaterial(world, i);

                if (mp) {
                    xGroup* g = (xGroup*)pp->texanim[tdx].group_ptr;
                    if (g) {
                        U32 texid = xGroupGetItem(g, pp->texanim[tdx].group_idx);
                        RwTexture* texptr = (RwTexture*)xSTFindAsset(texid, NULL);
                        if (texptr) {
                            RpMaterialSetTexture(mp, texptr);
                        }
                    }
                }
            }
        }

        xEnvRender(env, false);
    }
}

void zEnvSave(zEnv* ent, xSerial* s)
{
    xBaseSave(ent, s);

    xASSERT_L(299, ent);
    xASSERT_L(300, s);
}

void zEnvLoad(zEnv* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    xASSERT_L(318, ent);
    xASSERT_L(319, s);
}

void zEnvReset(zEnv* env)
{
    xASSERT_L(324, env);
    xASSERT_L(325, env->easset);
    xBaseReset(env, env->easset);
}

void zEnvEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase* toParamWidget, U32)
{
    xASSERT_L(344, to);
    switch (toEvent) {
    case eEventReset:
    case eEventDebugReset:
        zEnvReset((zEnv*)to);
        break;

    case eEventSceneBegin:
        xEnvResetJSPVisibilities(globals.sceneCur->env);
        break;

    case eEventOn:
        xASSERT_FMT_L(360, toParamWidget != 0, "On event sent to Env without jsp parameter!");
        iEnvJSPOn(globals.sceneCur->env->geom, (xJSPHeader*)toParamWidget);
        break;

    case eEventOff:
        xASSERT_FMT_L(365, toParamWidget != 0, "Off event sent to Env without jsp parameter!");
        iEnvJSPOff(globals.sceneCur->env->geom, (xJSPHeader*)toParamWidget);
        break;

    case eEventJSPVisibilityIncrement:
        xASSERT_FMT_L(370, toParamWidget != 0, "JSPVisibilityIncrement event sent to Env without jsp parameter!");
        iEnvJSPVisibilityInc(globals.sceneCur->env->geom, (xJSPHeader*)toParamWidget);
        break;

    case eEventJSPVisibilityDecrement:
        xASSERT_FMT_L(375, toParamWidget != 0, "JSPVisibilityDecrement event sent to Env without jsp parameter!");
        iEnvJSPVisibilityDec(globals.sceneCur->env->geom, (xJSPHeader*)toParamWidget);
        break;

    case eEventSetLightKit:
        gCurXEnv->lightKit = (xLightKit*)toParamWidget;
        break;
    }
}

xLightKit* zEnvGetGlobalObjectLightKit()
{
    zEnv* zen = globals.sceneCur->zen;
    xEnvAsset* easset = zen->easset;

    if (easset->objectLightKit) {
        return (xLightKit*)xSTFindAsset(easset->objectLightKit, NULL);
    } else {
        return NULL;
    }
}
