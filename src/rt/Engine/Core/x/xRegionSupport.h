#ifndef XREGIONSUPPORT_H
#define XREGIONSUPPORT_H

#include "types.h"

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

#endif