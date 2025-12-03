#include "zCompParameters.h"

#include "xDebugTweak.h"

#include <stdio.h>

void zCompParameters::AddTweak(const char* parentName, const char* groupName, const char* name, F32* v, F32 vmin, F32 vmax)
{
    char buffer[256];
    sprintf(buffer, "%s|%s|%s", parentName, groupName, name);
    xTWEAK(buffer, v, vmin, vmax, NULL, NULL, 0x2);
}

void zCompParameters::AddTweak(const char* parentName, const char* groupName, const char* name, bool* v)
{
    char buffer[256];
    sprintf(buffer, "%s|%s|%s", parentName, groupName, name);
    xTWEAKBOOL(buffer, v, NULL, NULL, 0x2);
}
