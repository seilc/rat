#include "xordarray.h"

#include "xDebug.h"
#include "xMemMgr.h"

#include "decomp.h"

void XOrdInit(st_XORDEREDARRAY* array, S32 size, U32 memtag, S32 tempAlloc)
{
    xVALIDATE(63, size>0);
    if (size<2) {
        xWARN("XOrdArray: TINY list size! (%d)\n", size);
    }
    size = (size<1) ? 1 : size;

    if (tempAlloc) {
        array->list = (void**)xMEMPUSHTEMP(size * sizeof(void*), 'XORD', 75);
    } else {
        array->list = (void**)xMEMALLOC(size * sizeof(void*), 0, memtag, 'XORD', 78);
    }

    array->cnt = 0;
    array->max = size;
    array->warnlvl = 0.95f * size;
    if (array->warnlvl == array->max) {
        array->warnlvl = (array->max-1 < 0) ? 0 : array->max-1;
    }
}

void XOrdDone(st_XORDEREDARRAY* array, S32 wasTempAlloc)
{
    if (array->max) {
        if (wasTempAlloc) {
            xMEMPOPTEMP(array->list);
        } else {
            xVERBOSE("XOrdDone - (Game) Skipping de-allocation of array\n");
        }
    }

    array->list = NULL;
    array->cnt = 0;
    array->max = 0;
    array->warnlvl = 0;
}

void XOrdAppend(st_XORDEREDARRAY* array, void* elt)
{
    xASSERT(134, array->cnt < array->max);

    if (array->cnt >= array->max) {
        xWARN("XOrdArray: Max size exceeded! (max:%d)\n", array->max);
    } else {
        if (array->cnt >= array->warnlvl) {
            xVERBOSE("XOrdArray: Nearly Full! (%d/%d)\n", array->cnt, array->max);
        }
        array->list[array->cnt++] = elt;
    }
}

void XOrdInsert(st_XORDEREDARRAY* array, void* elt, XOrdCompareCallback compare)
{
    S32 i = 0;

    xVALIDATE(167, compare);
    xASSERT(168, array->cnt <= (array->max-1));

    if (array->cnt >= array->max) {
        xWARN("XOrdArray: Insert exceeds array size\n");
        return;
    }

    array->cnt++;

    for (i = array->cnt-1; i > 0; i--) {
        if (compare(array->list[i-1], elt) <= 0) {
            array->list[i] = elt;
            return;
        } else {
            array->list[i] = array->list[i-1];
        }
    }

    array->list[0] = elt;
}

S32 XOrdLookup(st_XORDEREDARRAY* array, const void* key, XOrdTestCallback test)
{
    S32 da_idx = -1;
    S32 k0 = 0;
    S32 k1 = 0;
    S32 k = 0;
    S32 v = 0;

    k0 = 0;
    k1 = array->cnt;
    while (k1 > k0) {
        k = (k0 + k1) / 2;
        v = test(key, array->list[k]);
        if (v == 0) {
            da_idx = k;
            break;
        }
        if (v > 0) {
            k0 = k + 1;
        } else {
            k1 = k;
        }
    }

    return da_idx;
}

void XOrdSort(st_XORDEREDARRAY* array, XOrdCompareCallback test)
{
    void** list = array->list;
    S32 num = array->cnt;
    S32 i = 0;
    S32 j = 0;
    S32 h = 0;
    void* v = NULL;

    h = 1;
    while (h <= num) {
        h = h*3+1;
    }
    while (h != 1) {
        h /= 3;
        for (i = h; i < num; i++) {
            v = list[i];
            for (j = i; j >= h && test(v, list[j-h]) < 0; j -= h) {
                list[j] = list[j-h];
            }
            list[j] = v;
        }
    }
}

DECOMP_FORCEACTIVE(
    "idx>=0",
    "idx<array->cnt",
    "XOrdArray: InsIdx exceeds array size\n",
    "array->cnt <= array->max",
    "i!=0",
    "index >= 0",
    "index < array->max",
    "elt == array->list[index]",
    DEBUG ? "%s(%d) : (XOrdRemove failed to locate element) in '%s'\n" : 0,
    DEBUG ? "%s(%d) : (Src:Tgt lists sizes differ ... proceeding anyway) in '%s'\n" : 0,
    "((src->cnt-1) < tgt->max)",
    "Tgt list can't hold src",
    "(!(tgt->cnt))",
    "Tgt list not empty ... just warning ... proceeding",
    "src != tgt",
    "XOrdLocate: Failed to find first match\n"
)
