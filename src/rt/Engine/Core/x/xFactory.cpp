#include "xFactory.h"

static S32 OrdTest_infotype(const void* vkey, void* vitem);
static S32 OrdComp_infotype(void* vkey, void* vitem);

// RESTORED: from Incredibles GC
xFactoryInst* xFactory::CreateItem(S32 typeID, void* userdata, RyzMemGrow* callerzgrow)
{
    S32 idx;
    xFactoryInst* item;
    XGOFTypeInfo pattern = {};
    XGOFTypeInfo* darec = NULL;
    RyzMemGrow* grow = callerzgrow;

    pattern.tid = typeID;

    idx = XOrdLookup(&infolist, &pattern, OrdTest_infotype);
    if (idx >= 0) {
        darec = (XGOFTypeInfo*)infolist.list[idx];
    }

    if (!darec) {
        return NULL;
    }

    if (!grow && growContextData.IsEnabled()) {
        grow = &growContextData;
    }

    item = darec->creator(darec->tid, grow, userdata);
    if (!item) {
        return item;
    }

    item->itemType = darec->tid;
    item->prevprod = NULL;
    item->nextprod = NULL;

    if (products) {
        item->nextprod = products;
        products->prevprod = item;
        products = item;
    } else {
        products = item;
    }

    return item;
}

void xFactory::DestroyAll()
{
    while (products) {
        DestroyItem(products);
    }
}

void xFactory::DestroyItem(xFactoryInst* item)
{
    S32 idx;
    XGOFTypeInfo pattern = {};
    XGOFTypeInfo* darec;

    pattern.tid = item->itemType;

    if (item) {
        if (products == item) {
            products = item->nextprod;
            if (products) {
                products->prevprod = NULL;
            }
        }

        if (item->prevprod) {
            item->prevprod->nextprod = item->nextprod;
        }

        if (item->nextprod) {
            item->nextprod->prevprod = item->prevprod;
        }

        item->prevprod = NULL;
        item->nextprod = NULL;

        idx = XOrdLookup(&infolist, &pattern, OrdTest_infotype);
        darec = (XGOFTypeInfo*)infolist.list[idx];
        darec->destroyer(item);
    }
}

static S32 OrdTest_infotype(const void* vkey, void* vitem)
{
    S32 rc = 0;
    XGOFTypeInfo* key = (XGOFTypeInfo*)vkey;
    XGOFTypeInfo* item = (XGOFTypeInfo*)vitem;

    if (key->tid < item->tid) {
        rc = -1;
    } else if (key->tid > item->tid) {
        rc = 1;
    } else {
        rc = 0;
    }

    return rc;
}

static S32 OrdComp_infotype(void* vkey, void* vitem)
{
    S32 rc = 0;
    XGOFTypeInfo* key = (XGOFTypeInfo*)vkey;
    XGOFTypeInfo* item = (XGOFTypeInfo*)vitem;

    if (key->tid < item->tid) {
        rc = -1;
    } else if (key->tid > item->tid) {
        rc = 1;
    } else {
        rc = 0;
    }

    return rc;
}
// END RESTORED
