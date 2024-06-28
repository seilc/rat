#ifndef XENV_H
#define XENV_H

#include "iEnv.h"
#include "xLightKit.h"

struct xEnv
{
    iEnv* geom;
    iEnv ienv;
    xLightKit* lightKit;
};

extern xEnv* gCurXEnv;

#ifdef DEBUGRELEASE
extern bool debugNoJspsRendering;
#endif

void xEnvLoadJSPList(xEnv* env, S32 dataType);
void xEnvSetup(xEnv* env);
void xEnvFree(xEnv* env);
void xEnvRender(xEnv* env, bool alpha);
void xEnvResetJSPVisibilities(xEnv* env);

#endif
