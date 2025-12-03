#ifndef ZCOMPPARAMETERS_H
#define ZCOMPPARAMETERS_H

#include "types.h"

class zCompParameters
{
public:
    void AddTweak(const char* parentName, const char* groupName, const char* name, F32* v, F32 vmin, F32 vmax);
    void AddTweak(const char* parentName, const char* groupName, const char* name, bool* v);
};

#endif
