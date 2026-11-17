#ifndef ZUITEXTASSET_H
#define ZUITEXTASSET_H

#include "zUIAsset.h"
#include "xColor.h"

struct zUITextAsset : zUIAsset
{
    U32 textID;
    U8 font;
    U8 fontSizeW;
    U8 fontSizeH;
    U8 fontSpacingX;
    U8 fontSpacingY;
    U8 textBoxInsetTop;
    U8 textBoxInsetLeft;
    U8 textBoxInsetRight;
    U8 textBoxInsetBottom;
    U8 justifyX;
    U8 justifyY;
    U8 textFlags;
    xColor shadowColor;
    F32 shadowOffsetX;
    F32 shadowOffsetY;
    F32 textScaleX;
    F32 textScaleY;
};

#endif
