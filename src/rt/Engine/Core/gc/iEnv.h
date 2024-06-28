#ifndef IENV_H
#define IENV_H

#include "xMath3.h"

#include <rwcore.h>
#include <rpworld.h>

struct xJSPHeader;
struct xJSPNodeInfo;

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

void iEnvLoadBegin(iEnv* env, S32 dataType, S32 count);
bool iEnvLoadJSP(iEnv* env, U32 aid, const void* data, U32, S32 dataType, S32 index);
void iEnvLoadEnd(iEnv* env, S32 dataType);
void iEnvFree(iEnv* env);
void iEnvDefaultLighting(iEnv*);
void iEnvSetup(iEnv* env);
void iEnvRender(iEnv* env, bool alpha);

#endif
