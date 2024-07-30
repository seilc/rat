#ifndef XSOUNDFXASSET_H
#define XSOUNDFXASSET_H

#include "xBaseAsset.h"
#include "xVec3.h"
#include "iSnd.h"

struct xSoundFXAsset : xBaseAsset
{
    union
    {
        U32 soundAssetID;
        iSndGroupHandle soundAsset;
    };
    U32 attachID;
    xVec3 pos;
    U32 uFlags;
};

inline void xSoundFXSetSendsDone(xSoundFXAsset* pSoundFX, bool bIsSet)
{
    if (bIsSet) {
        pSoundFX->uFlags |= 0x1;
    } else {
        pSoundFX->uFlags &= ~0x1;
    }
}

inline void xSoundFXSetSoundFXPlaying(xSoundFXAsset* pSoundFX, bool bIsSet)
{
    if (bIsSet) {
        pSoundFX->uFlags |= 0x2;
    } else {
        pSoundFX->uFlags &= ~0x2;
    }
}

inline void xSoundFXSetHandleSet(xSoundFXAsset* pSoundFX, bool bIsSet)
{
    if (bIsSet) {
        pSoundFX->uFlags |= 0x8;
    } else {
        pSoundFX->uFlags &= ~0x8;
    }
}

inline bool xSoundFXIsAttached(xSoundFXAsset* pSoundFX)
{
    return pSoundFX->uFlags & 0x4;
}

inline bool xSoundFXSendsDone(xSoundFXAsset* pSoundFX)
{
    return pSoundFX->uFlags & 0x1;
}

inline bool xSoundFXIsSoundFXPlaying(xSoundFXAsset* pSoundFX)
{
    return pSoundFX->uFlags & 0x2;
}

inline bool xSoundFXIsHandleSet(xSoundFXAsset* pSoundFX)
{
    return pSoundFX->uFlags & 0x8;
}

#endif
