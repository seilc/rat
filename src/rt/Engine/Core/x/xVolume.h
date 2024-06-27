#ifndef XVOLUME_H
#define XVOLUME_H

#include "xBase.h"
#include "xVolumeAsset.h"

class xVolume : public xBase
{
public:
    xVolumeAsset* asset;

    void Init(xVolumeAsset* asset);
    void Reset();
    void Save(xSerial* s);
    void Load(xSerial* s);
    xBound* GetBound();
};

#endif
