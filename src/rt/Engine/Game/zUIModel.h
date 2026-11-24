#ifndef ZUIMODEL_H
#define ZUIMODEL_H

#include "zUI.h"
#include "zUIModelAsset.h"

struct xModelInstance;
struct xAnimTable;
struct xSurface;

class zUIModel : public zUI
{
public:
    virtual U32 GetSortKey() const { return GetAsset()->model; }

    zUIModelAsset* GetAsset() const { return (zUIModelAsset*)zUI::GetAsset(); }

    virtual void DoInit();
    virtual void DoReset();
    virtual void DoRender() const;
    virtual void DoUpdate(F32 dt);
    virtual void DoExit();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
#if DEBUG || RELEASE
    virtual void DoAddTweaks(const char* baseName);
#endif

private:
    void InitModel();
    void InitUVs();

    xModelInstance* model;
    xAnimTable* atbl;
    xSurface* surf;
    F32* uvCoords;
    U32 uvCoordsCount;
};

void zUIModel_Init(xBase& data, xDynAsset& asset, size_t);
void zUIModel_Init(zUIModel* model, zUIModelAsset* asset);

#endif
