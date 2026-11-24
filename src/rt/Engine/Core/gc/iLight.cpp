#include "xCommon.h"
#include "iLight.h"

#include "xDebugTweak.h"
#include "xEnv.h"
#include "xModel.h"
#include "iEnv.h"

#include <stdio.h>

void iLightInit()
{
}

void iLightDeinit()
{
}

iLight* iLightCreate(iLight* light, U32 type)
{
    switch (type) {
    case 1:
        light->hw = RpLightCreate(rpLIGHTPOINT);
        break;
    case 2:
        light->hw = RpLightCreate(rpLIGHTSPOT);
        break;
    case 3:
        light->hw = RpLightCreate(rpLIGHTSPOTSOFT);
        break;
    default:
        iprintf("ERROR:  iLightCreate unknown type %d\n", type);
        return NULL;
    }

    if (!light->hw) {
        iprintf("ERROR:  iLightCreate unable to create RpLight\n");
        return NULL;
    }

    RpGameCubeLightSetAttenuation(light->hw, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f);

    RwFrame* frame = RwFrameCreate();
    if (!frame) {
        RpLightDestroy(light->hw);
        iprintf("ERROR:  iLightCreate unable to create RwFrame\n");
        return NULL;
    }

    RpLightSetFlags(light->hw, rpLIGHTLIGHTATOMICS | rpLIGHTLIGHTWORLD);
    RpLightSetFrame(light->hw, frame);

    RwFrameUpdateObjects(frame);

    light->type = type;
    light->sph.center = xVec3::m_Null;
    light->sph.r = 0.0f;
    light->color.r = 1.0f;
    light->color.g = 1.0f;
    light->color.b = 1.0f;
    light->color.a = 1.0f;
    light->dir = xVec3::m_UnitAxisZ;
    light->coneangle = 0.0f;

    light->lightTie.light = light->hw;

    return light;
}

void iLightSetColor(iLight* light, const xFColor& col)
{
    light->color = col;
    RpLightSetColor(light->hw, (const RwRGBAReal*)&col);
}

void iLightGetColor(iLight* light, xFColor& col)
{
    col = light->color;
}

void iLightSetPos(iLight* light, const xVec3& pos)
{
    RwFrame* f = RpLightGetFrame(light->hw);
    RwMatrix* m = RwFrameGetMatrix(f);

    m->pos.x = pos.x;
    m->pos.y = pos.y;
    m->pos.z = pos.z;
    light->sph.center = pos;

    RwMatrixUpdate(m);

    RwFrameUpdateObjects(f);
}

void iLightGetPos(iLight* light, xVec3& pos)
{
    pos = light->sph.center;
}

void iLightSetRadius(iLight* light, F32 radius)
{
    light->sph.r = radius;
    light->radius_sq = radius * radius;
    RpLightSetRadius(light->hw, light->sph.r);
}

xSphere* iLightGetBoundingSphere(iLight* light)
{
    return &light->sph;
}

void iLightDestroy(iLight* light)
{
    xASSERT_L(158, light->type);
    xASSERT_L(159, light->hw);

    light->type = 0;

    _rwFrameSyncDirty();

    RwFrame* frame = RpLightGetFrame(light->hw);
    if (frame) {
        RwFrameDestroy(frame);
    }

    RpLightDestroy(light->hw);
}

static RpTie dummyTie;
static RpWorldSector dummyWorldSector;

void iLightAddForModel(xModelInstance* minst, iLight** lights, S32 numLights)
{
    xASSERT_L(199, minst);
    xASSERT_L(200, lights);
    xASSERT_L(201, numLights);

    rwLinkListInitialize(&dummyWorldSector.lightsInWorldSector);
    dummyTie.worldSector = &dummyWorldSector;

    RpAtomic* atomic = minst->Data;
    rwLinkListAddLLLink(&atomic->llWorldSectorsInAtomic, &dummyTie.lWorldSectorInAtomic);

    for (S32 i = 0; i < numLights; i++) {
        xASSERT_L(213, lights[i]);

        rwLinkListAddLLLink(&dummyWorldSector.lightsInWorldSector, &lights[i]->lightTie.lightInWorldSector);
    }
}

void iLightRemoveFromModel(xModelInstance* minst, iLight** lights, S32 numLights)
{
    xASSERT_L(222, minst);
    xASSERT_L(223, lights);
    xASSERT_L(224, numLights);

    for (S32 i = 0; i < numLights; i++) {
        xASSERT_L(229, lights[i]);
        rwLinkListRemoveLLLink(&lights[i]->lightTie.lightInWorldSector);
    }

    RpAtomic* atomic = minst->Data;
    rwLinkListRemoveLLLink(&dummyTie.lWorldSectorInAtomic);
}

void iLightEnv(iLight* light, S32 env)
{
    RwUInt32 flags = 0;
    switch (env) {
    case 0:
        flags = 0;
        break;
    case 1:
        flags = rpLIGHTLIGHTWORLD;
        break;
    case 2:
        flags = rpLIGHTLIGHTATOMICS;
        break;
    case 3:
        flags = rpLIGHTLIGHTATOMICS | rpLIGHTLIGHTWORLD;
        break;
    default:
        xFAIL_L(262);
        break;
    }

    RpLightSetFlags(light->hw, flags);
}

void iLightJSPPreRender(xEnv* env, iLight* light)
{
    iEnvLightPreRender(&env->ienv, light->hw);
}

void iLightJSPPostRender(xEnv* env)
{
    iEnvLightPostRender(&env->ienv);
}

#if DEBUG || RELEASE
static tweak_callback colorUpdateCallback;
static tweak_callback radiusUpdateCallback;
static tweak_callback positionUpdateCallback;

void OnUpdateColorTweak(const tweak_info& info)
{
    iLight* light = (iLight*)info.context;
    iLightSetColor(light, light->color);
}

void OnUpdateRadiusTweak(const tweak_info& info)
{
    iLight* light = (iLight*)info.context;
    iLightSetRadius(light, light->sph.r);
}

void OnUpdatePositionTweak(const tweak_info& info)
{
    iLight* light = (iLight*)info.context;
    iLightSetPos(light, light->sph.center);
}

void iLightAddTweak(iLight* light, const char* name)
{
    colorUpdateCallback = tweak_callback::create_update(OnUpdateColorTweak);
    radiusUpdateCallback = tweak_callback::create_update(OnUpdateRadiusTweak);
    positionUpdateCallback = tweak_callback::create_update(OnUpdatePositionTweak);

    char buffer[128];
    sprintf(buffer, "%s|Position", name);
    xTWEAK_VEC3(buffer, &light->sph.center, &positionUpdateCallback, light, 0x2);
    sprintf(buffer, "%s|Radius", name);
    xTWEAK(buffer, &light->sph.r, 0.0f, 1000.0f, &radiusUpdateCallback, light, 0x2);
    sprintf(buffer, "%s|ColorR", name);
    xTWEAK(buffer, &light->color.r, 0.0f, 1.0f, &colorUpdateCallback, light, 0x2);
    sprintf(buffer, "%s|ColorG", name);
    xTWEAK(buffer, &light->color.g, 0.0f, 1.0f, &colorUpdateCallback, light, 0x2);
    sprintf(buffer, "%s|ColorB", name);
    xTWEAK(buffer, &light->color.b, 0.0f, 1.0f, &colorUpdateCallback, light, 0x2);
}
#endif
