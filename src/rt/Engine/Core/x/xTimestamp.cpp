#include "xTimestamp.h"

#ifdef DEBUG
#define BUILDNUM "129"
#else
#define BUILDNUM "135"
#endif

char timestamp[] =
    "The Incredibles\n"
    "Build " BUILDNUM "\n"
    "(c) 2004 Heavy Iron Studios\n"
    "Confidential -- internal use only\n"
    "GameCube build: " __DATE__ ", " __TIME__ "\n";
