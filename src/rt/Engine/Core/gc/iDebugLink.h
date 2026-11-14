#ifndef IDEBUGLINK_H
#define IDEBUGLINK_H

#include "xDebugLink.h"

class iDebugLink : public xDebugLink
{
public:
    static void Allocate();

    virtual eError Initialize();
    virtual eError ReadyForAssetData(char**, S32, void*);
    virtual eError SendEntEvent(char**, S32, void*);
    virtual void Update(F32);
    virtual eError HeartBeatResponse(char** argv, S32 argc, void*);
};

BOOL hioEnumCallback(s32 chan);
void hioCallback();
void procReadMailbox();

#endif
