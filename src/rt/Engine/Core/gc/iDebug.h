#ifndef IDEBUG_H
#define IDEBUG_H

#include "types.h"

#include <dolphin.h>

#ifdef DEBUGRELEASE
#define iprintf OSReport
#else
#define iprintf
#endif

#ifdef DEBUGRELEASE
static void iDebugBreak()
{
    asm { opword 0 }
}
#endif

#endif
