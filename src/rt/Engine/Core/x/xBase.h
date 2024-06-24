#ifndef XBASE_H
#define XBASE_H

#include "xBaseAsset.h"
#include "xLinkAsset.h"

struct xBase;

typedef void(*xBaseEventCallback)(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

struct xBase
{
    U32 id;
    U8 baseType;
    U8 linkCount;
    U16 baseFlags;
    const xLinkAsset* link;
    xBaseEventCallback eventFunc;
};

struct xSerial;

void xBaseInit(xBase* xb, const xBaseAsset* asset);
void xBaseSave(xBase* ent, xSerial* s);
void xBaseLoad(xBase* ent, xSerial* s);
void xBaseReset(xBase* xb, xBaseAsset* asset);

#endif