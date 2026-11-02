#ifndef IDEBUG_H
#define IDEBUG_H

#include "types.h"

#include <dolphin.h>

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
void iDebugDumpEFB(char* name);
#else
#define iDebugDumpEFB(name)
#endif

#endif
