#ifndef ZJSPEXTRADATA_H
#define ZJSPEXTRADATA_H

#include "xBase.h"
#include "xDynAsset.h"

struct zJSPExtraData : xBase
{
    class zJSPExtraDataAsset : public xDynAsset
    {
    public:
        U32 jspID;
        U32 groupID;
    };

    struct jspGroupPair
    {
        U32 jspID;
        U32 groupID;
        xBase* pGroup;
        xBase* pWidget;
    };

    static const S32 MAX_PAIRS = 10;
    static jspGroupPair sJspGroupPairs[MAX_PAIRS];
    static S32 sNumPairs;

    static void ShowAllAssociatedGroups(bool show);
    static void ShowAssociatedGroup(U32 jspAssetId, bool show);
    static void ClearGroupAssociations();
    static void AddPair(U32 jspID, U32 groupID, zJSPExtraData* widget);
    static void Load(xBase& data, xDynAsset& asset, size_t);
    void LoadPrivate(const zJSPExtraDataAsset& myAsset);
};

#endif
