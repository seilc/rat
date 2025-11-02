#include "xJaw.h"

#include "xstransvc.h"
#include "xDebug.h"

#include <cmath>

struct xJawDataTable
{
    U32 soundHashID;
    U32 dataStart;
    U32 dataLength;
};

xJawData* xJawData::Find(U32 soundID)
{
    S32 i, numJawTables;

    numJawTables = xSTAssetCountByType('JAW ');

    for (i = 0; i < numJawTables; i++) {
        U32 j;
        void* data;
        U32 jawcount;
        xJawDataTable* tbl;
        void* rawdata;

        data = xSTFindAssetByType('JAW ', i, NULL);
        jawcount = *(U32*)data;
        tbl = (xJawDataTable*)((U8*)data + sizeof(U32));
        rawdata = (void*)((U8*)tbl + jawcount * sizeof(xJawDataTable));

        for (j = 0; j < jawcount; j++) {
            if (soundID == tbl[j].soundHashID) {
                return (xJawData*)((U8*)rawdata + tbl[j].dataStart);
            }
        }
    }
    
    iprintf("Missing JAW data for %08X (%s)!\n", soundID, xSTAssetName(soundID));
    return NULL;
}

xJawSpeaker xJawData::GetIndex(S32 index, F32& value)
{
    value = (F32)(samples[index] & 63) / 63;
    return (xJawSpeaker)(samples[index] >> 6);
}

xJawSpeaker xJawData::Eval(F32 time, F32& jawValue)
{
    F32 lerp;
    U32 idx;
    F32 thisFrameValue;
    F32 nextFrameValue;
    xJawSpeaker thisFrameSpeaker;

    xASSERTM(53, header.numSamples <= 65535, "The jaw header doesn't seem to be swapped... This will break");
    
    time *= 60.0f;
    idx = std::floorf(time);
    lerp = time - std::floorf(time);

    if (idx >= header.numSamples - 1) {
        jawValue = 0.0f;
        return exJawSpeaker_Remy;
    }

    thisFrameSpeaker = GetIndex(idx, thisFrameValue);

    if (thisFrameSpeaker != GetIndex(idx + 1, nextFrameValue)) {
        nextFrameValue = 0.0f;
    }

    jawValue = thisFrameValue * (1.0f - lerp) + nextFrameValue * lerp;

    return thisFrameSpeaker;
}
