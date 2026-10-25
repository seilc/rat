#include "xFactory.h"

// RESTORED: from Incredibles GC
static S32 OrdTest_infotype(const void* vkey, void* vitem);
static S32 OrdComp_infotype(void* vkey, void* vitem);

xFactory::~xFactory()
{
    infopool = NULL;
    XOrdDone(&infolist, FALSE);
}

S32 xFactory::RegItemType(XGOFTypeInfo* info)
{
    S32 rc = 0;

    for (XGOFTypeInfo* tptr = info; tptr->tid != 0; tptr++) {
        XGOFTypeInfo* nextrec;

        rc = 1;

        if (!tptr->creator) {
            rc = 0;
            break;
        }

        if (!tptr->destroyer) {
            rc = 0;
            break;
        }

        if (infolist.cnt >= infolist.max) {
            rc = -2;
            break;
        }

        S32 idx = XOrdLookup(&infolist, tptr, OrdTest_infotype);
        if (idx >= 0) {
            rc = -1;
            break;
        }

        nextrec = &infopool[infolist.cnt];
        nextrec->tid = tptr->tid;
        nextrec->creator = tptr->creator;
        nextrec->destroyer = tptr->destroyer;

        XOrdInsert(&infolist, nextrec, OrdComp_infotype);
    }

    return rc;
}
// END RESTORED

// RESTORED: from TSSM GC, BFBB GC
void xFactory::GrowDataDisable()
{
    growContextData.Done();
}
// END RESTORED

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
