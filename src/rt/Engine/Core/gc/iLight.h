#ifndef ILIGHT_H
#define ILIGHT_H

#include "xMath3.h"
#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>

struct iLight
{
    U32 type;
    RpLight* hw;
    RpLightTie lightTie;
    xSphere sph;
    F32 radius_sq;
    xFColor color;
    xVec3 dir;
    F32 coneangle;
};

struct xModelInstance;
struct xEnv;

void iLightInit();
void iLightDeinit();
iLight* iLightCreate(iLight* light, U32 type);
void iLightSetColor(iLight* light, const xFColor& col);
void iLightGetColor(iLight* light, xFColor& col);
void iLightSetPos(iLight* light, const xVec3& pos);
void iLightGetPos(iLight* light, xVec3& pos);
void iLightSetRadius(iLight* light, F32 radius);
xSphere* iLightGetBoundingSphere(iLight* light);
void iLightDestroy(iLight* light);
void iLightAddForModel(xModelInstance* minst, iLight** lights, S32 numLights);
void iLightRemoveFromModel(xModelInstance* minst, iLight** lights, S32 numLights);
void iLightEnv(iLight* light, S32 env);
void iLightJSPPreRender(xEnv* env, iLight* light);
void iLightJSPPostRender(xEnv* env);
#if DEBUG || RELEASE
void iLightAddTweak(iLight* light, const char* name);
#endif

#endif
