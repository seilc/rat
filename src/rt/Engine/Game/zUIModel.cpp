#include "xCommon.h"
#include "zUIModel.h"

#include "containers.h"
#include "xAnim.h"
#include "xEnt.h"
#include "xFont.h"
#include "xLightKit.h"
#include "xMath2.h"
#include "xModel.h"
#include "xPad.h"
#include "xRenderState.h"
#include "xScene.h"
#include "xstransvc.h"
#include "xSurface.h"
#include "zAnimList.h"
#include "zEnt.h"
#include "zEvent.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zUIUVs.h"

#include <new>

void zUIModel::DoInit()
{
    zUI::DoInit();
    if (linkCount) {
        link = (const xLinkAsset*)(GetAsset() + 1);
    }

    InitModel();
}

void zUIModel::InitModel()
{
    const zUIModelAsset* asset = GetAsset();

    xASSERT_FMT_L(48, !model, "UI Model with asset ID %X is being initialized twice.  Possible memory leak.", asset->id);
    if (model) {
        return;
    }

    atbl = NULL;

    if (!asset->model) {
        model = NULL;
        return;
    }

    RpAtomic* imodel = (RpAtomic*)xSTFindAsset(asset->model, NULL);
    if (imodel) {
        model = xModelInstanceAlloc(imodel, this, 0, 0, NULL);
        xModelInstanceCheckAlloc_L(66, model, xSTAssetName(asset->model)); model->modelID = asset->model;

        while (imodel = iModelFile_RWMultiAtomic(imodel))
            xModelInstanceAttach(xModelInstanceAlloc(imodel, this, 0x8, 0, NULL), model);

        if (asset->animationList && zAnimListGetNumUsed(asset->animationList) > 0) {
            atbl = zAnimListGetTable(asset->animationList);
            xAnimPoolAlloc(&globals.sceneCur->mempool, this, atbl, model);

            xAnimState* state = xAnimTableGetState(atbl, "idle");
            if (state) {
                xAnimPlaySetState(model->Anim->Single, state, 0.0f);
            }
        }

        if (asset->surface) {
            xSurface* surf = (xSurface*)xSceneResolvID(globals.sceneCur, asset->surface);
            if (surf) {
                surf->type = 3;

                xModelInstance* minst = model;
                while (minst) {
                    minst->Surf = surf;
                    minst = minst->Next;
                }
            }
        }

        InitUVs();
    } else
        model = NULL;
}

void zUIModel::InitUVs()
{
    uvCoords = zUIUVs_Init(model->Data, uvCoordsCount);
}

void zUIModel::DoReset()
{
    zUI::DoReset();
}

void zUIModel::DoRender() const
{
    if (!model) {
        return;
    }

    static xVec3 from = { 0.0f, 0.0f, 1.0f };
    static xVec3 to = { 0.0f, 0.0f, 0.0f };

    basic_rect<F32> r = {
        current.x * (1.0f / SCREEN_WIDTH), current.y * (1.0f / SCREEN_HEIGHT),
        current.width * (1.0f / SCREEN_WIDTH), current.height * (1.0f / SCREEN_HEIGHT)
    };

    if (r.w <= 0.0f || r.h <= 0.0f) return;

    xModelSetMaterialAlpha(model, current.color.a);

    U32 srcblend = (model->Pipe.Flags >> eSRCBLEND_SHIFT) & 0xf;
    U32 destblend = (model->Pipe.Flags >> eDESTBLEND_SHIFT) & 0xf;

    srcblend = !srcblend ? rwBLENDSRCALPHA : srcblend;
    destblend = !destblend ? rwBLENDINVSRCALPHA : destblend;

    if ((model->Pipe.Flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
        xRenderStateBlendAndZModesSet((RwBlendFunction)srcblend, (RwBlendFunction)destblend, FALSE, FALSE);
    } else {
        xRenderStateBlendAndZModesSet((RwBlendFunction)srcblend, (RwBlendFunction)destblend, TRUE, TRUE);
    }

    xEntSetupPipeline(model);

    model->Scale.assign(1.0f, 1.0f, 1.0f);

    if (!zGameIsPaused()) {
        xRenderStateZModesSet(FALSE, FALSE);
    }
    xLightKit_Enable(*model, xglobals->currWorld);
    xModelRender2D(*model, r, from, to);

    xEntRestorePipeline(model);

    if ((model->Pipe.Flags & eZBUFFER_MASK) == ePIPEZBUFFER_DISABLE) {
        xRenderStateZModesSet(TRUE, TRUE);
    }

    xModelSetMaterialAlpha(model, 255);
}

void zUIModel::DoUpdate(F32 dt)
{
    zUI::DoUpdate(dt);

    if (model) {
        xModelUpdate(model, dt);
        xModelEval(model);
    }
}

void zUIModel::DoExit()
{
    zUI::DoExit();
    if (model) {
        xModelInstanceFree_L(model, true, 199);
        model = NULL;
        surf = NULL;
    }
}

void zUIModel::DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventAnimPlay:
    case eEventAnimPlayLoop:
    case eEventAnimStop:
    case eEventAnimPause:
    case eEventAnimResume:
    case eEventAnimTogglePause:
    case eEventAnimPlayRandom:
    case eEventAnimPlayMaybe:
        zEntAnimEventCore(model, atbl, toEvent, toParam, NULL);
        break;

    case eEventUIResetMotion:
        zUI::DoHandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
        if (model->Anim && model->Anim->Single->State)
            xAnimPlaySetState(model->Anim->Single, model->Anim->Single->State, 0.0f);
        break;

    default:
        zUI::DoHandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
        break;
    }
}

#if DEBUG || RELEASE
void zUIModel::DoAddTweaks(const char* baseName)
{
    zUI::DoAddTweaks(baseName);
}
#endif

void zUIModel_Init(xBase& data, xDynAsset& asset, size_t)
{
    zUIModel_Init((zUIModel*)&data, (zUIModelAsset*)&asset);
}

void zUIModel_Init(zUIModel* model, zUIModelAsset* asset)
{
    new (model) zUIModel();
    zUI_Init(model, asset);
}
