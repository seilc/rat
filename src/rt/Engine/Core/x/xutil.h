#ifndef XUTIL_H
#define XUTIL_H

#include "types.h"
#include "xDebug.h"

#define IDTAG(a,b,c,d) (((U32)(a)<<24)|((U32)(b)<<16)|((U32)(c)<<8)|((U32)(d)))
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

S32 xUtilStartup();
S32 xUtilShutdown();
char* xUtil_idtag2string(U32 srctag, S32 bufidx);
#if DEBUG
void xUtil_niceBufDump(char* membuf, S32 bytcnt, S32 ashex, S32 rptoff);
#endif
U32 xUtil_crc_init();
U32 xUtil_crc_update(U32 crc_accum, char* data, S32 datasize);
void strtosjis(U8* string, U8* dest);
S32 sjistostr(U8* string, U8* destination);
U8 BCDtoi(U8 hex);
U8 itoBCD(U8 dec);
U8 itoBCD(U16 dec);
S32 xUtil_yesno(F32 wt_yes);
void xUtil_wtadjust(F32* wts, S32 cnt, F32 arbref);
void xUtil_Sort(F32* list, S32 cnt);
#if DEBUG
void xUtil_spit_flagbits(char flag, S32 toScreen);
void xUtil_spit_flagbits(U8 flag, S32 toScreen);
void xUtil_spit_flagbits(S8 flag, S32 toScreen);
void xUtil_spit_flagbits(S16 flag, S32 toScreen);
void xUtil_spit_flagbits(U16 flag, S32 toScreen);
void xUtil_spit_flagbits(U32 flag, S32 toScreen);
void xUtil_spit_flagbits(S32 flag, S32 toScreen);
void xUtil_spit_flagbits(S32 flag, S32 toScreen, S32 numbytes);
#endif
F32 xClampAngle0_2PI(F32 angle);
void quicksort_int(void* base, size_t num, size_t width, size_t key_offset);
void quicksort_uint(void* base, size_t num, size_t width, size_t key_offset);
void quicksort_float(void* base, size_t num, size_t width, size_t key_offset);
void quicksort_str(void* base, size_t num, size_t width, size_t key_offset);
void quicksort_istr(void* base, size_t num, size_t width, size_t key_offset);
void isort_int(void* base, size_t num, size_t width, size_t key_offset);
void isort_float(void* base, size_t num, size_t width, size_t key_offset);
void isort_str(void* base, size_t num, size_t width, size_t key_offset);
void isort_istr(void* base, size_t num, size_t width, size_t key_offset);
void xTouchObject(void*);

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

#define XUTIL_SWAP(a, b, width)                                                                   \
do {                                                                                              \
    U8* _b0 = (a);                                                                                \
    U8* _b1 = (b);                                                                                \
    U8* _endb0 = (a) + (width);                                                                   \
    while (_b0 != _endb0) {                                                                       \
        U32 _temp = *(U32*)_b0;                                                                   \
        *(U32*)_b0 = *(U32*)_b1;                                                                  \
        *(U32*)_b1 = _temp;                                                                       \
        _b0 += 4, _b1 += 4;                                                                       \
    }                                                                                             \
} while (0)

template <class T>
void quicksort(void* base, size_t num, size_t width, T cmp)
{
    size_t thresh = width * 7;
    U8* stack[40];
    U8** sp = stack;
    U8 *pivot = (U8*)base, *tail = pivot + num * width;

    for (;;) {
        if (tail - pivot <= thresh) {
            U8 *v = pivot, *next = v + width;
            for (; next < tail; v = next, next += width) {
                while (cmp(v, v + width) > 0) {
                    XUTIL_SWAP(v, v + width, width);
                    if (v == pivot) {
                        break;
                    }
                    v -= width;
                }
            }

            if (sp == stack) {
                break;
            }

            pivot = sp[-2];
            tail = sp[-1]; sp -= 2;
            continue;
        }

        size_t half = (size_t)(tail - pivot) / 2;
        half -= half % width;
        XUTIL_SWAP(pivot + half, pivot, width);
        U8 *left = pivot + width, *right = tail - width;
        if (cmp(left, right) > 0) {
            XUTIL_SWAP(left, right, width);
        }
        if (cmp(pivot, right) > 0)
            XUTIL_SWAP(pivot, right, width);
        else if (cmp(left, pivot) > 0)
            XUTIL_SWAP(left, pivot, width);

        for (;;) {
            do left += width; while (cmp(left, pivot) < 0);
            do right -= width; while (cmp(right, pivot) > 0);
            if (left > right) {
                break;
            }
            XUTIL_SWAP(left, right, width);
        }
        XUTIL_SWAP(pivot, right, width);

        if (right - pivot > tail - left) {
            sp[0] = pivot; sp[1] = right;
            pivot = left;
        } else {
            sp[0] = left; sp[1] = tail;
            tail = right;
        }
        sp += 2;
    }
}

template <class T>
void isort(void* base, size_t num, size_t width, T cmp)
{
    xASSERT_L(676, (num <= 100) &&"Use quicksort, fool.");
    xASSERT_L(677, (width & 3) == 0);
    U8 *v = (U8*)base, *next = v + width, *end = v + num * width;
    for (; next != end; v = next, next += width) {
        while (cmp(v, v + width) > 0) {
            XUTIL_SWAP(v, v + width, width);
            if (v == base) {
                break;
            }
            v -= width;
        }
    }
}

#endif
