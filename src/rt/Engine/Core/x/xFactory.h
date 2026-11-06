#ifndef XFACTORY_H
#define XFACTORY_H

#include "xRMemData.h"
#include "xordarray.h"

struct xFactoryInst : RyzMemData
{
protected:
    S32 itemType;
    xFactoryInst* nextprod;
    xFactoryInst* prevprod;

    friend struct xFactory;
};

typedef xFactoryInst* (*xFactoryCreateCallback)(S32, RyzMemGrow*, void*);
typedef void (*xFactoryDestroyCallback)(xFactoryInst*);
typedef S32 (*xFactoryTraverseCallback)(xFactoryInst*, void*);

struct XGOFTypeInfo
{
    S32 tid;
    xFactoryCreateCallback creator;
    xFactoryDestroyCallback destroyer;
};

struct xFactory : RyzMemData
{
    xFactory(S32 maxTypes, U32 memtag); // UNUSED
    ~xFactory(); // UNUSED
    S32 RegItemType(XGOFTypeInfo* info); // UNUSED
    XGOFTypeInfo* GetInfoList(S32& count); // UNUSED
    S32 RegItemType(S32 tid, xFactoryCreateCallback create, xFactoryDestroyCallback destroy); // UNUSED
    void GrowDataEnable(xBase* user, U32 memtag, S32 isResume); // UNUSED
    void GrowDataDisable(); // UNUSED
    xFactoryInst* CreateItem(S32 typeID, void* userdata, RyzMemGrow* callerzgrow); // UNUSED
    void DestroyAll(); // UNUSED
    void DestroyItem(xFactoryInst* item); // UNUSED
    void TraverseProducts(xFactoryTraverseCallback trav, void* userdata); // UNUSED

protected:
    XGOFTypeInfo* infopool;
    st_XORDEREDARRAY infolist;
    xFactoryInst* products;
    RyzMemGrow growContextData;
};

#endif
