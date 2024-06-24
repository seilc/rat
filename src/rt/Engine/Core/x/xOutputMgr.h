#ifndef XOUTPUTMGR_H
#define XOUTPUTMGR_H

#include "iDebug.h"

enum xOutSeverity
{
    xOutSevIgnore = -1,
    xOutSevCrit = 1,
    xOutSevErr,
    xOutSevWarn,
    xOutSevInfo,
    xOutSevBabble,
    xOutSevForce = FORCEENUMSIZEINT
};

#ifdef DEBUGRELEASE
#define xTRACEFUNCTION(line, name)                                                                  \
do {                                                                                                \
if (xOutTrumps((name), xOutSevInfo) || (xOutEnabled((name)) && xOutGetSev() >= xOutSevInfo)) {      \
    iprintf("~~ entering %s in %s(%d)", __FUNCTION__, __FILE__, (line));                            \
    iprintf("\n");                                                                                  \
}                                                                                                   \
} while (0)
#else
#define xTRACEFUNCTION(line, name)
#endif

#ifdef DEBUGRELEASE
S32 xOutGetSev();
S32 xOutEnabled(char* name);
S32 xOutTrumps(char* name, xOutSeverity usersev);
#endif

#endif