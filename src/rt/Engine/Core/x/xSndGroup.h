#ifndef XSNDGROUP_H
#define XSNDGROUP_H

#include "xBaseAsset.h"

struct xSndGroupHeader : xBaseAsset
{
    U32 uPlayedMask;
    U8 uInfoCount;
    U8 uSetBits;
    S8 nMaxPlays;
    U8 uPriority;
    U8 uFlags;
    U8 eSoundCategory;
    U8 ePlayRule;
    U8 uLastSelected;
    F32 fInnerRadius;
    F32 fOuterRadius;
    const char* pszGroupName;
};

struct xSndGroupInfo
{
    U32 uSoundNameHash;
    F32 fVolume;
    F32 fMinPitchMult;
    F32 fMaxPitchMult;
};

struct xSndGroup
{
    xSndGroupHeader header;
    xSndGroupInfo aSndGroupInfo[0];
};

#endif
