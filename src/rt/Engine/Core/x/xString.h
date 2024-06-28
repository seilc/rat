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
char* xStrTok(char* string, const char* control, char** nextoken);
S32 xStricmp(const char* string1, const char* string2);

#endif
