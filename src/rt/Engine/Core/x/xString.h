#ifndef XSTRING_H
#define XSTRING_H

#include "xDebug.h"

#include <stddef.h>

struct xVec3;

struct substr
{
    const char* text;
    size_t size;

    static substr create(const char* text, size_t size) { substr s = { text, size }; return s; }
};

U32 xStrHash(const char* str);
U32 xStrHash(const char* s, size_t size);
U32 xStrHashCat(U32 prefix, const char* str);
char* xStrTok(char* string, const char* control, char** nextoken);
U32 xStrTokCount(const char* string, const char* control, U32* maxsize);
const char* xStrTokBuffer(const char* string, const char* control, void* buffer);
const char* xStristr(const char* str1, const char* str2);
S32 xStricmp(const char* string1, const char* string2);
S32 xMemicmp(const void* string1, const void* string2, size_t chars);
char* xStrupr(char* string);
S32 xStrParseFloatList(F32* dest, const char* strbuf, S32 max);
F32 xStrParseFloat(const char* x);
U8 xStrConvert(const char* istr, F32* oValue); // UNUSED
U8 xStrConvert(const char* istr, U32* oValue); // UNUSED
U8 xStrConvert(const char* istr, U8* oValue); // UNUSED
U8 xStrConvert(const char* istr, S32* oValue); // UNUSED
U8 xStrConvert(const char* istr, bool* oValue); // UNUSED
U8 xStrConvert(const char* istr, xVec3* oValue); // UNUSED
S32 xStrWildCmp(const char* wild, const char* string); // UNUSED
S32 xStrWildCmpi(const char* wild, const char* string); // UNUSED
S32 imemcmp(const void* d1, const void* d2, size_t size);
S32 icompare(const substr& s1, const substr& s2);
U32 atox(const substr& s, size_t& read_size);
const char* find_char(const substr& s, const substr& cs);
char* ifind_string(const substr& s, const substr& key); // UNUSED

#if DEBUG
inline U32 xStrHashVerify(const char* str, U32 hash)
{
    U32 correct_hash = xStrHash(str);
    xASSERT_MSG_L(78, correct_hash == hash, "XSTRHASH macro has incorrect hard-coded hash value, please fix this code.");
    return correct_hash;
}
#endif

inline bool is_ws(char c) { return c == ' ' || c == '\t' || c == '\n'; }

inline const char* skip_ws(const char*& text, size_t& size)
{
    xASSERT_L(175, (size == 0) || (text != 0));
    for (size_t i = 0; i < size && *text; i++, ++text)
        if (!is_ws(*text)) { size -= i; break; }
    return text;
}

inline size_t rskip_ws(const char*& text, size_t& size)
{
    xASSERT_L(192, text != 0);
    while (size && is_ws(text[size - 1])) size--;
    return size;
}

#endif
