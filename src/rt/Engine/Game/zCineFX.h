#ifndef ZCINEFX_H
#define ZCINEFX_H

#include "types.h"

#define CINFX_FRAMES(n) ((n) * (1.0f / 30.0f))

struct NCINEntry;
struct xScene;
struct zCutsceneMgr;

struct NCINMap
{
    char* cinname;
    NCINEntry* cinfxtab;
    U32 cinhash;
};

void zCineFXStartup();
void zCineFXShutdown();
void zCineFXSceneSetup();
void zCineFXRegisterTable(NCINMap* maplist);
S32 zCineFXCutscenePrep(const xScene*, F32, const zCutsceneMgr* csnmgr);
void zCineFXCutsceneDone(const xScene*, F32, const zCutsceneMgr* csnmgr);
void zCineFXCutscene(const xScene*, F32, const zCutsceneMgr* csnmgr);

#endif
