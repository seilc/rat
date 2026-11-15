#ifndef ZFROZONEICE_H
#define ZFROZONEICE_H

#include "xVec3.h"

#include <rwcore.h>

struct zCutsceneMgr;
struct NCINEntry;
struct RpAtomic;

struct zFrozoneIceRecordEntry
{
    F32 time;
    xVec3 loc;
};

struct cinfx_frobridge_context
{
    S32 flags;
    const zFrozoneIceRecordEntry* record;
    S32 record_size;
};

void cinfx_frobridge_update(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit);
void cinfx_frobridge_animate(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrix* animMat, U32 animIndex, U32,
                             U32&);
void cinfx_frobridge_render(const zCutsceneMgr*, NCINEntry*);
void cinfx_frobridge_add_tweaks(const char*, NCINEntry*);

#endif
