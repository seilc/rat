#ifndef XUTIL_H
#define XUTIL_H

#include "types.h"

#define IDTAG(a,b,c,d) (((U32)(a)<<24)|((U32)(b)<<16)|((U32)(c)<<8)|((U32)(d)))
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

char* xUtil_idtag2string(U32 srctag, S32 bufidx);
#if DEBUG
void xUtil_niceBufDump(char* membuf, S32 bytcnt, S32 ashex, S32 rptoff);
#endif
F32 xClampAngle0_2PI(F32 angle);
void quicksort_uint(void* base, unsigned long num, unsigned long width, unsigned long key_offset);
void quicksort_istr(void* base, unsigned long num, unsigned long width, unsigned long key_offset);

template <class T, class V, class C>
T binary_find(T first, T last, const V& value, C cmp)
{
    const T notfound = last;
    while (first < last)
    {
        const T mid = first + (last - first) / 2;
        S32 c = cmp(value, *mid);
        if (c < 0)
            last = mid;
        else if (c > 0)
            first = mid + 1;
        else
            return mid;
    }
    return notfound;
}

#endif
