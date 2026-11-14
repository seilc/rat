#ifndef ZUIIMAGEASSET_H
#define ZUIIMAGEASSET_H

#include "zUIAsset.h"
#include "xColor.h"

struct zUIImageAsset : zUIAsset
{
    U32 image;
    F32 u1;
    F32 v1;
    F32 u2;
    F32 v2;
    F32 u3;
    F32 v3;
    F32 u4;
    F32 v4;
    F32 rotation;
    U16 iflags;
    U8 addressModeU;
    U8 addressModeV;
    xColor color1;
    xColor color2;
    xColor color3;
    xColor color4;
    U8 forceAlphaWrite;
    U8 pad[3];
};

#endif
