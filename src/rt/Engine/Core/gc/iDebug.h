#ifndef IDEBUG_H
#define IDEBUG_H

#include "types.h"

#include <dolphin.h>

struct RwCamera;

#if DEBUG || RELEASE
#define iprintf OSReport
#else
#define iprintf
#endif

#if DEBUG || RELEASE
static inline void iDebugBreak()
{
    asm { opword 0 }
}
#endif

#if DEBUG || RELEASE
void iDebugEventStart(U8); // UNUSED
void iDebugEventEnd(U8); // UNUSED
void iDebugInit();
void iDebugUpdate();
void iDebugWriteProfile();
void iDebugDrawScreen(char* txt, S32 x, S32 y); // UNUSED
void iDebugVSync();
void iDebugStackTrace();
void iDebugIDEOutput(char* buf);
void iDebugScreenDump(RwCamera* camera);
void iDebugDumpEFB(char* name);
#else
#define iDebugDumpEFB(name)
#endif

#endif
