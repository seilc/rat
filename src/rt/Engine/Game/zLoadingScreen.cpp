#include "xCommon.h"
#include "zLoadingScreen.h"

#include "containers.h"
#include "xDynAsset.h"
#include "xFont.h"
#include "xGroupAsset.h"
#include "xPad.h"
#include "xstransvc.h"
#include "zBase.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zUIBox.h"
#include "zUIController.h"
#include "zUIImage.h"
#include "zUIMgr.h"
#include "zUIText.h"

#include <string.h>

namespace {

void* CustomStaticAllocator(U32 size, void* user)
{
    return ((zLoadingScreen*)user)->CustomAllocate(size);
}

void CustomSignalHandler(zUI* from, U32 dest, U32 event, const F32* param, xBase* paramWidget, U32 paramWidgetID, void* user)
{
    ((zLoadingScreen*)user)->CustomSignal(from, dest, event, param, paramWidget, paramWidgetID);
}

} // namespace

static bool show_stats;

void zLoadingScreen::UnSetShowStats()
{
    show_stats = false;
}

void zLoadingScreen::Init()
{
    allocationCount = 0;
    zUISetCustomStaticAllocator(CustomStaticAllocator, this);

    controlCount = 0;

    xGroupAsset* group = (xGroupAsset*)xSTFindAsset(HASH_LOADING_SCREEN, NULL);

    show_stats = true;

    xASSERT_L(73, group != NULL && group->baseType == eBaseTypeGroup);
    for (S16 i = 0; i < group->itemCount; i++) {
        U32 size;
        xDynAsset* asset = (xDynAsset*)xSTFindAsset(((U32*)(group + 1))[i], &size);
        if (!asset)
            continue;
        void* test;
        zUI* element = NULL;
        switch (asset->type) {
        case HASH_ui_image:
            test = RwMallocL(sizeof(zUIImage), rwMEMHINTDUR_NADURATION, 85);
            element = (zUIImage*)test;
            xASSERT_L(87, reinterpret_cast<U32>( test ) == reinterpret_cast<U32>( element ));
            memset(element, 0, sizeof(zUIImage));
            zUIImage_Init(*element, *asset, size);
            break;
        case HASH_ui_box:
            element = (zUI*)RwMallocL(sizeof(zUIBox), rwMEMHINTDUR_NADURATION, 94);
            memset(element, 0, sizeof(zUIBox));
            zUIBox_Init(*element, *asset, size);
            break;
        case HASH_ui_text:
            element = (zUI*)RwMallocL(sizeof(zUIText), rwMEMHINTDUR_NADURATION, 100);
            memset(element, 0, sizeof(zUIText));
            zUIText_Init(*element, *asset, size);
            break;
        case HASH_ui_controller:
            element = (zUI*)RwMallocL(sizeof(zUIController), rwMEMHINTDUR_NADURATION, 106);
            memset(element, 0, sizeof(zUIController));
            zUIController_Init(*element, *asset, size);
            break;
        }

        if (element) {
            xASSERT_L(114, controlCount < MAX_CONTROLS);
            controls[controlCount++] = element;
            xASSERT_L(116, allocationCount < MAX_ALLOCATION);
            allocations[allocationCount++] = element;
        }
    }

    zUISetCustomStaticAllocator(NULL, NULL);
    zUISetCustomSignalHandler(CustomSignalHandler, this);

    zUIMgrSortList(controls, controlCount);

    for (S32 i = 0; i < controlCount; i++) {
        controls[i]->HandleEvent(controls[i], eEventVisible, NULL, NULL, 0);
        controls[i]->Signal(eEventVisible);
    }
}

void zLoadingScreen::Update(F32 dt)
{
    for (S32 i = 0; i < controlCount; i++)
        if (controls[i]->visible)
            controls[i]->Update(dt);
}

void zLoadingScreen::Render()
{
    for (S32 i = controlCount - 1; i >= 0; i--)
        if (controls[i]->visible) {
            controls[i]->Render();
        }

    xFontPrintTopText();
}

void zLoadingScreen::Exit()
{
    for (S32 j = 0; j < controlCount; j++)
        controls[j]->Exit();

    zUISetCustomSignalHandler(NULL, NULL);
    for (S32 i = 0; i < allocationCount; i++)
        RwFree(allocations[i]);
    allocationCount = 0;

    xPadUpdate(0, 0.0f);
    xPadUpdate(1, 0.0f);
#if DEBUG
    xPadUpdate(2, 0.0f);
    xPadUpdate(3, 0.0f);
#endif
}

void* zLoadingScreen::CustomAllocate(U32 size)
{
    xASSERT_L(214, allocationCount < MAX_ALLOCATION);
    allocations[allocationCount] = RwMallocL(size, rwMEMHINTDUR_NADURATION, 215);
    return allocations[allocationCount++];
}

void zLoadingScreen::CustomSignal(zUI* from, U32 dest, U32 event, const F32* param, xBase* paramWidget, U32 paramWidgetID)
{
    for (S32 i = 0; i < controlCount; i++)
        if (dest == controls[i]->GetAsset()->id) {
            controls[i]->HandleEvent(from, event, param, paramWidget, paramWidgetID);
            break;
        }
}
