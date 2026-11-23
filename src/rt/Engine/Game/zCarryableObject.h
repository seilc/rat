#ifndef ZCARRYABLEOBJECT_H
#define ZCARRYABLEOBJECT_H

#include "zCarryableObjectAsset.h"
#include "zEntSimpleObj.h"

struct zCarryableProperty;

class zCarryableObject : public zEntSimpleObj
{
public:
    zCarryableObject(zCarryableObjectAsset& asset);

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    void Setup();
    void Reset();
    void Update(F32 dt);
    void Drop();
    void HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetId);

protected:
    zCarryableObjectAsset& carryAsset;
    const zCarryableProperty* property;
    F32 time;
    xEnt* carriedBy;
};

#endif
