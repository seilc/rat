#include "xTextAsset.h"

#include "xstransvc.h"
#include "xString.h"
#include "xDebug.h"

const char* xTextFindString(U32 key, U32* len)
{
    if (key == 0) {
        return NULL;
    }

    U32 size;
    const xTextAsset* asset = (const xTextAsset*)xSTFindAsset(key, &size);

    if (!asset) {
        return NULL;
    }

    xASSERT(46, asset->len < 10000 && asset->len + 4 + sizeof(xTextAsset) >= size && asset->len + sizeof(xTextAsset) <= size);

    if (len) {
        *len = asset->len;
    }

    return (const char*)((U8*)asset + sizeof(xTextAsset));
}

const char* xTextFindString(const char* key, U32* len)
{
    return xTextFindString(xStrHash(key), len);
}