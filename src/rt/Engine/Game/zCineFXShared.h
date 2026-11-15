#ifndef ZCINEFXSHARED_H
#define ZCINEFXSHARED_H

#include "xVec3.h"

#include <rwcore.h>
#include <rpworld.h>

struct zCutsceneMgr;
struct zLightning;
struct zParEmitter;
struct xEnt;
struct xFXRibbon;
struct NCINRibbonInput;
struct NCINEntry;

struct TypeInfo
{
    union
    {
        S32 fxTypeID;
        S32 usrTypeID;
    };
    F32 tym_beg;
    F32 tym_end;
    char* twk_name;
};

struct Callbacks
{
    void (*fxupd)(const zCutsceneMgr*, NCINEntry*, S32);
    void (*fxanim)(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrix*, U32, U32, U32&);
    void (*fxrend)(const zCutsceneMgr*, NCINEntry*);
    void (*tweaks)(const char*, NCINEntry*);
};

struct IndexInput
{
    S32 idx_anim;
    S32 idx_bone;
    S32 idx_boneA;
    S32 idx_boneB;
};

struct PosInput
{
    xVec3 pos_A[2];
    xVec3 pos_B[2];
};

union NCINInput
{
    void* whateverMakesTheCompilerHappyIsFineWithMe;
    void* fxparm;
    char* str_fx;
    S32 int_val;
    F32 flt_val;
    NCINRibbonInput* ribbon;
};

struct NCINLyt
{
    zLightning* lyt_zap;
};

struct NCINStreak
{
    U32 streakID;
};

struct NCINArc
{
    zLightning* lightning;
    xVec3 endPos;
};

struct NCINPar
{
    zParEmitter* emitter;
};

struct NCINEnts
{
    xEnt* ent[4];
};

struct NCINCustom
{
    F32 f[1];
    xVec3 v[2];
    S32 n[3];
};

struct NCINMat
{
    RwMatrix* mat;
};

struct NCINRibbon
{
    xFXRibbon* ribbon;
};

struct NCINBoneProbe
{
    U8 render;
};

struct NCINData
{
    union
    {
        NCINLyt lytdata;
        NCINStreak streakdata;
        NCINArc arcdata;
        NCINPar pardata;
        NCINEnts entdata;
        NCINCustom customdata;
        NCINMat matdata;
        NCINRibbon ribbondata;
        NCINBoneProbe probedata;
    };
};

struct NCINEntry
{
    TypeInfo info;
    Callbacks cb;
    IndexInput index;
    PosInput basic;
    NCINInput input;
    void* userFXData;
    NCINData fxdata;
    S32 flg_stat;
};

void zCinFX_BoneProbe_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit);
void zCinFX_BoneProbe_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model, RwMatrix* animMat, U32 animIndex,
                         U32, U32&);
#if DEBUG || RELEASE
void zCinFX_BoneProbe_Twk(const char* prefix_root, NCINEntry* fxrec);
#endif

#endif
