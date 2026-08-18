#ifndef XPARSYS_H
#define XPARSYS_H

#include "xBase.h"
#include "xParSysAsset.h"
#include "xParCmd.h"
#include "xParGroup.h"

struct xScene;
struct zScene;

struct xParSys : xBase
{
    xParSysAsset* tasset;
    U32 cmdCount;
    xParCmd* cmd;
    xParSys* parent;
    xParGroup* group;
    bool visible;
    RwTexture* txtr_particle;
#if DEBUG || RELEASE
    xParCmdAny* cmd_buffer;
#endif
};

// Not present in DWARF, names are guesses except for eParSysRenderFuncCount which is present in an assert string
enum eParSysRenderFunc
{
    eParSysRenderFuncSprite,
    eParSysRenderFuncStreak,
    eParSysRenderFuncFlat,
    eParSysRenderFuncStatic,
    eParSysRenderFuncGround,
    eParSysRenderFuncQuadStreak,
    eParSysRenderFuncInvStreak,
    eParSysRenderFuncCount
};

xParGroupRenderFunction xParSysGetRenderFunction(S32 which);
void xParCmdTexInit(xParCmdTex* cmd);
void xParSysInit(void* b, void* tasset);
void xParSysInit(xBase* b, xParSysAsset* tasset);
void xParSysSetup(xParSys* t);
void xParSysReset(xParSys* t);
xBase* xParSysExit(xBase* b, zScene*, void*);
void xParSysExit(xParSys* t);
void xParSysEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
void xParSysUpdate(xBase* to, xScene*, F32 dt);
void xParGroupUpdate(xParSys* s, xParGroup* g, F32 dt);

#endif
