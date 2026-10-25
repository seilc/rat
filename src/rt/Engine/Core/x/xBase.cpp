#include "xBase.h"

#include "xDebug.h"
#include "xserializer.h"

void xBaseInit(xBase* xb, const xBaseAsset* asset)
{
    xASSERT_L(23, xb);
    xASSERT_L(24, asset);

    xb->id = asset->id;
    xb->baseType = asset->baseType;
    xb->baseFlags = asset->baseFlags;
    xb->linkCount = asset->linkCount;
    xb->link = NULL;

    xBaseValidate(xb);
}

void xBaseSetup(xBase* xb)
{
    xASSERT_L(66, xb);
}

void xBaseSave(xBase* ent, xSerial* s)
{
    xASSERT_L(81, ent);
    xASSERT_L(82, s);

    if (xBaseIsEnabled(ent)) {
        s->Write_b1(1);
    } else {
        s->Write_b1(0);
    }
}

void xBaseLoad(xBase* ent, xSerial* s)
{
    xASSERT_L(104, ent);
    xASSERT_L(105, s);

    S32 b = 0;
    s->Read_b1(&b);

    if (b) {
        xBaseEnable(ent);
    } else {
        xBaseDisable(ent);
    }
}

void xBaseReset(xBase* xb, xBaseAsset* asset)
{
    xASSERT_L(120, xb);
    xASSERT_L(121, asset);

    xb->baseFlags = (xb->baseFlags & k_XBASE_RECEIVES_SHADOWS) | (asset->baseFlags & ~k_XBASE_RECEIVES_SHADOWS);

    xBaseValidate(xb);
}
