#ifndef IDEBUG_H
#define IDEBUG_H

#include "types.h"

#ifdef DEBUGRELEASE
static void iDebugBreak()
{
    asm { opword 0 }
}
#endif

#endif