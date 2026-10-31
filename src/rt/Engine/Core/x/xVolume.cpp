#include "xVolume.h"

#include "xDebug.h"
#include "xVolumeAsset.h"

void xVolume::Init(xVolumeAsset* asset)
{
    xBaseInit(this, asset);

    this->asset = asset;

    if (this->linkCount)
        this->link = (xLinkAsset*)((U8*)this->asset + sizeof(xVolumeAsset));
    else
        this->link = NULL;
}

void xVolume::Reset()
{
    xASSERT_L(38, this->asset);

    xBaseReset(this, this->asset);
}

void xVolume::Save(xSerial* s)
{
    xBaseSave(this, s);

    xASSERT_L(58, s);
}

void xVolume::Load(xSerial* s)
{
    xBaseLoad(this, s);

    xASSERT_L(75, s);
}

xBound* xVolume::GetBound()
{
    return &this->asset->bound;
}
