#include "zUIController.h"

#include <new>

void zUIController::DoInit()
{
    zUI::DoInit();
    if (linkCount) {
        link = (const xLinkAsset*)(GetAsset() + 1);
    }
}

void zUIController::DoRender() const
{
}

void zUIController_Init(xBase& data, xDynAsset& asset, size_t)
{
    zUIController_Init((zUIController*)&data, (zUIControllerAsset*)&asset);
}

void zUIController_Init(zUIController* text, zUIControllerAsset* asset)
{
    new (text) zUIController();
    zUI_Init(text, asset);
}
