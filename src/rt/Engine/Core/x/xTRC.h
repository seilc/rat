#ifndef XTRC_H
#define XTRC_H

#include "types.h"

#define k_MAX_TRC_PADS 4

typedef enum _tagTRCState
{
    TRC_Unknown,
    TRC_PadMissing,
    TRC_PadInserted,
    TRC_PadActive,
    TRC_PadPreInserted,
    TRC_PadInvalidNoAnalog,
    TRC_PadInvalidType,
    TRC_DiskNotIdentified,
    TRC_DiskIdentified,
    TRC_DiskTrayOpen,
    TRC_DiskTrayClosed,
    TRC_DiskNoDisk,
    TRC_DiskInvalid,
    TRC_DiskRetry,
    TRC_DiskFatal,
    TRC_Total
} TRCState;

bool xTRCPadFindFirst(S32& idx, TRCState state);
S32 xTRCPadGetPadPort(S32 idx);
void xTRCInit();

#endif
