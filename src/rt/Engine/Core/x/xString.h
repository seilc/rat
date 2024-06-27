#ifndef XSTRING_H
#define XSTRING_H

#include "types.h"

#include <stddef.h>

struct substr
{
    const char* text;
    size_t size;
};

U32 xStrHash(const char* str);

#endif
