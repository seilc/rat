#ifndef XORDARRAY_H
#define XORDARRAY_H

#include "types.h"

struct st_XORDEREDARRAY
{
    void** list;
    S32 cnt;
    S32 max;
    S32 warnlvl;
};

typedef S32(*XOrdCompareCallback)(void*, void*);
typedef S32(*XOrdTestCallback)(const void*, void*);

void XOrdInit(st_XORDEREDARRAY* array, S32 size, U32 memtag, S32 tempAlloc);
void XOrdReset(st_XORDEREDARRAY* array); // UNUSED
void XOrdDone(st_XORDEREDARRAY* array, S32 wasTempAlloc);
void XOrdAppend(st_XORDEREDARRAY* array, void* elt);
void XOrdInsert(st_XORDEREDARRAY* array, void* elt, XOrdCompareCallback compare);
S32 XOrdInsIdx(st_XORDEREDARRAY* array, void* elt, S32 idx); // UNUSED
void* XOrdRemove(st_XORDEREDARRAY* array, void* elt, S32 index); // UNUSED
void XOrdCopy(st_XORDEREDARRAY* tgt, const st_XORDEREDARRAY* src); // UNUSED
S32 XOrdFind(st_XORDEREDARRAY* array, const void* key, XOrdTestCallback test); // UNUSED
S32 XOrdLookup(st_XORDEREDARRAY* array, const void* key, XOrdTestCallback test);
S32 XOrdLocate(st_XORDEREDARRAY* array, const void* key, S32 nth, S32 issort, XOrdTestCallback test); // UNUSED
void XOrdSort(st_XORDEREDARRAY* array, XOrdCompareCallback test);

#endif
