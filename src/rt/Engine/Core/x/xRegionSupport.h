#ifndef XREGIONSUPPORT_H
#define XREGIONSUPPORT_H

#include "iDebug.h"

#include <string.h>

enum xRegion
{
    eRegionUnknown = -1,
    eRegionUS = 0,
    eRegionBE,
    eRegionCH,
    eRegionCZ,
    eRegionDE,
    eRegionDK,
    eRegionES,
    eRegionFI,
    eRegionFR,
    eRegionIT,
    eRegionJP,
    eRegionKR,
    eRegionNL,
    eRegionNO,
    eRegionPL,
    eRegionPT,
    eRegionRU,
    eRegionSE,
    eRegionSK,
    eRegionTW,
    eRegionUK,
    eRegionCount,
    eRegionMaxCount = 32
};

extern char* region_array[eRegionCount];

bool xRegionCodeIsValid(const char* pszRegionCode);

inline char* xRegionCode(xRegion region)
{
    if (region < eRegionUS || region >= eRegionCount) {
        iprintf("xRegionCode:: Invalid region %d; using US\n", region);
        return region_array[eRegionUS];
    }
    return region_array[region];
}

inline xRegion xRegionString(const char* pszRegion)
{
    for (xRegion iRegion = eRegionUS; iRegion < eRegionCount; iRegion = (xRegion)(iRegion + 1)) {
        if (stricmp(region_array[iRegion], pszRegion) == 0) {
            return iRegion;
        }
    }

    return eRegionUnknown;
}

#endif
