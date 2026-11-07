#ifndef IENV_H
#define IENV_H

#include "xMath3.h"
#include "xJSP.h"
#include "iTime.h"

#include <rwcore.h>
#include <rpworld.h>

struct iEnvMatOrder
{
    U16 jspIndex;
    U16 nodeIndex;
    S32 matGroup;
    RpAtomic* atomic;
    xJSPNodeInfo* nodeInfo;
};

struct iEnv
{
    RpWorld* world;
    RpWorld* collision;
    RpWorld* fx;
    RpWorld* camera;
    S32 jsp_count;
    U32* jsp_aid;
    xJSPHeader** jsp_list;
    xBox* jsp_bound;
    S32* jsp_visibilityCount;
    S32 jspMatOrderCount;
    iEnvMatOrder* jspMatOrderList;
    RpLight* light[2];
    RwFrame* light_frame[2];
    S32 memlvl;
    U16 numOpaque;
    U16 numTransparent;
};

#if DEBUG || RELEASE
extern bool iEnvAllJSPSActive;
extern bool debugNoJspsRendering;
#endif

void iEnvLoadBegin(iEnv* env, S32 dataType, S32 count);
bool iEnvLoadJSP(iEnv* env, U32 aid, const void* data, U32, S32 dataType, S32 index);
void iEnvLoadEnd(iEnv* env, S32 dataType);
void iEnvFree(iEnv* env);
void iEnvDefaultLighting(iEnv*);
void iEnvSetup(iEnv* env);
void iEnvRender(iEnv* env, bool alpha);

inline bool iEnvIsJSPActive(const iEnv* env, S32 which)
{
    return env->jsp_visibilityCount[which] > 0;
}

inline bool iEnvIsJSPVisible(const iEnv* env, S32 which)
{
#if DEBUG || RELEASE
    if (debugNoJspsRendering) {
        return true;
    }

    if (env->jsp_visibilityCount[which] < 0) {
        S64 time = iTimeGet();
        return (time / 125) % 2 == 1;
    }

    return env->jsp_visibilityCount[which] > 0 || iEnvAllJSPSActive;
#else
    return env->jsp_visibilityCount[which] > 0;
#endif
}

#endif
