#include "xScreenFade.h"

#include "zGlobals.h"
#include "zBase.h"
#include "zGame.h"
#include "zEvent.h"
#include "xScrFx.h"
#include "zScene.h"

#include <string.h>

namespace ScreenFade {
    namespace GlobalData {
        static S32 cnt_fadesInProgress = 0;
    }
}

void xScreenFade::load(asset_type& a)
{
    memset(this, 0, sizeof(xScreenFade));
    xBaseInit(this, &a);

    baseType = eBaseTypeScreenFade;
    asset = &a;
    eventFunc = cb_dispatch;

    if (linkCount) {
        link = (xLinkAsset*)(&a + 1);
    }

    state = eScreenFadeStateOff;
    src = g_CLEAR;

    if (asset->fadeDownTime < 0.0f) {
        asset->fadeDownTime = 2.0f;
    }
    if (asset->fadeUpTime < 0.0f) {
        asset->fadeUpTime = 2.0f;
    }
    if (asset->waitTime < 0.0f) {
        asset->waitTime = 2.0f;
    }

    owner = NULL;
}

U8 xScreenFade::InterpCol(F32 t, U8 s, U8 d)
{
    U8 uval;

    F32 val = (F32)(d-s);
    val *= t;
    val += (F32)s;

    uval = (U8)val;
    return uval;
}

void xScreenFade::StartFade()
{
    state = eScreenFadeStateFadeDown;
    time_passed = 0.0f;

    xASSERT(73, ScreenFade::GlobalData::cnt_fadesInProgress>=0);
    ScreenFade::GlobalData::cnt_fadesInProgress++;
}

void xScreenFade::update(F32 seconds)
{
#ifdef DEBUGRELEASE
    if (xglobals->ForceCutscene) {
        state = eScreenFadeStateOff;
        return;
    }
#endif

    if (zGameIsPaused()) return;
    if (state == eScreenFadeStateOff) return;

    xASSERT(121, ScreenFade::GlobalData::cnt_fadesInProgress > 0);

    time_passed += seconds;

    switch (state) {
    case eScreenFadeStateFadeDown:
        if (time_passed >= asset->fadeDownTime) {
            state = eScreenFadeStateWait;
            time_passed = 0.0f;
            current_color = asset->dest;

            zEntEvent(this, this, eEventFadeDownDone);
        } else {
            F32 percent = time_passed / asset->fadeDownTime;
            current_color.r = InterpCol(percent, src.r, asset->dest.r);
            current_color.g = InterpCol(percent, src.g, asset->dest.g);
            current_color.b = InterpCol(percent, src.b, asset->dest.b);
            current_color.a = InterpCol(percent, src.a, asset->dest.a);
        }
        break;
    case eScreenFadeStateWait:
        if (time_passed >= asset->waitTime) {
            state = eScreenFadeStateFadeUp;
            time_passed = 0.0f;
        }
        break;
    case eScreenFadeStateFadeUp:
        if (time_passed >= asset->fadeUpTime) {
            state = eScreenFadeStateOff;
            current_color = src;

            zEntEvent(this, this, eEventFadeUpDone);

            ScreenFade::GlobalData::cnt_fadesInProgress--;
            xASSERT(190, ScreenFade::GlobalData::cnt_fadesInProgress>=0);

            if (ScreenFade::GlobalData::cnt_fadesInProgress < 0) {
                ScreenFade::GlobalData::cnt_fadesInProgress = 0;
            }
        } else {
            F32 percent = time_passed / asset->fadeUpTime;
            current_color.r = InterpCol(percent, asset->dest.r, src.r);
            current_color.g = InterpCol(percent, asset->dest.g, src.g);
            current_color.b = InterpCol(percent, asset->dest.b, src.b);
            current_color.a = InterpCol(percent, asset->dest.a, src.a);
        }
        break;
    }
}

void xScreenFade::render()
{
    if (state == eScreenFadeStateOff) return;

    RwVideoMode video_mode;
    RwEngineGetVideoModeInfo(&video_mode, RwEngineGetCurrentVideoMode());

    xScrFxDrawBox(0.0f, 0.0f, video_mode.width, video_mode.height, current_color.r, current_color.g, current_color.b, current_color.a, true);
}

void xScreenFade::reset()
{
    state = eScreenFadeStateOff;
    owner = NULL;
    ScreenFade::GlobalData::cnt_fadesInProgress = 0;
}

void xScreenFade::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(236, asset, xScreenFade::asset_type);
    ((xScreenFade&)data).load((xScreenFade::asset_type&)asset);
}

void xScreenFade::update_all(F32 dt)
{
    zScene& scene = *xglobals->sceneCur;
    xScreenFade* it = (xScreenFade*)scene.baseList[eBaseTypeScreenFade];
    xScreenFade* end = it + scene.baseCount[eBaseTypeScreenFade];
    while (it != end) {
        it->update(dt);
        it++;
    }
}

void xScreenFade::render_all()
{
    zScene& scene = *xglobals->sceneCur;
    xScreenFade* it = (xScreenFade*)scene.baseList[eBaseTypeScreenFade];
    xScreenFade* end = it + scene.baseCount[eBaseTypeScreenFade];
    while (it != end) {
        it->render();
        it++;
    }
}

void xScreenFade::cb_dispatch(xBase* from, xBase* to, U32 event, const F32*, xBase*, U32)
{
    xASSERT(261, to != 0);

    xScreenFade& e = (xScreenFade&)*to;
    xASSERT(263, e.baseType == eBaseTypeScreenFade);

    switch (event) {
    case eEventReset:
    case eEventDebugReset:
    case eEventSceneEnd:
        e.reset();
        break;
    case eEventStartFade:
        e.owner = from;
        e.StartFade();
    }
}
