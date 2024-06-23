#include "xTimestamp.h"

#ifdef DEBUG
#define BUILDNUM "129"
#else
#define BUILDNUM "135"
#endif

#ifdef DEBUG
#define __DATE__ "Jan 11 2006"
#define __TIME__ "00:29:14"
#else
#define __DATE__ "Jan 18 2006"
#define __TIME__ "15:52:40"
#endif

char timestamp[] =
    "The Incredibles\n"
    "Build " BUILDNUM "\n"
    "(c) 2004 Heavy Iron Studios\n"
    "Confidential -- internal use only\n"
    "GameCube build: " __DATE__ ", " __TIME__ "\n";