#include "xHudText.h"

#include "xHudAsset.h"
#include "zTextBox.h"
#include "zScene.h"
#include "xTextAsset.h"
#include "zMain.h"
#include "xScrFx.h"

#include <string.h>

namespace xhud {
    
void text_widget::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(15, asset, text_asset, text_asset::type_name());

    init_base(data, asset, sizeof(widget_chunk));
    
    widget_chunk& wc = (widget_chunk&)data;
    new (&wc.w) text_widget((text_asset&)asset);
}

text_widget::text_widget(const text_asset& a)
    : widget(a)
{
    text[0] = '\0';
}

void text_widget::destruct()
{
    widget::destruct();
}

void text_widget::setup()
{
    presetup();

    const text_asset& ta = *(text_asset*)a;
    ztextbox* ztb = (ztextbox*)zSceneFindObject(ta.text_box);

    xASSERTFMT(35, ztb != 0, "HUD Text %s: Cannot find text-box %s", xSTAssetNameOrID((void*)a), xSTAssetNameOrID(ta.text_box));

    if (ztb == 0) {
        tb = xtextbox::create();
        return;
    }
    
    ztb->refresh();
    tb = ztb->tb;
    ztb->get_text(text, sizeof(text));

    if (ta.text != 0) {
        U32 len;
        const char* s = xTextFindString(ta.text, &len);

        xASSERTFMT(50, s != 0, "HUD Text %s: Cannot find text %s", xSTAssetNameOrID((void*)a), xSTAssetNameOrID(ta.text));

        if (s) {
            memcpy(text, s, len);
            text[len] = '\0';
        }
    }

    rc.r = (U8)(tb.font.color.r / 255.0f);
    rc.g = (U8)(tb.font.color.g / 255.0f);
    rc.b = (U8)(tb.font.color.b / 255.0f);
    rc.a = (U8)(tb.font.color.a / 255.0f);

    start_rc.r = rc.r;
    start_rc.g = rc.g;
    start_rc.b = rc.b;
    start_rc.a = rc.a;
}

void text_widget::destroy()
{
    destruct();
}

U32 text_widget::type() const
{
    static U32 myid = xStrHash(text_asset::type_name());
    return myid;
}

bool text_widget::is(U32 id) const
{
    return id == text_widget::type() || widget::is(id);
}

#ifndef NON_MATCHING // Force float literal order in .sdata2
#ifndef MASTER
static void __unused()
{
    *(F32*)0 = 0.0f;
}
#endif
#endif

void text_widget::update(F32 dt)
{
    updater(dt);

    tb.set_text(text);

    tb.bounds.x = rc.loc.x;
    tb.bounds.y = rc.loc.y;
    tb.bounds.w = rc.size.x;
    tb.bounds.h = rc.size.y;

    if (rc.size.x < 1.0f) {
        tb.bounds.w = 640.0f * rc.size.x / FB_XRES;
    }

    if (rc.size.y < 1.0f) {
        tb.bounds.h = 480.0f * rc.size.y / FB_YRES;
    }

    tb.font.clip = tb.bounds;
}

void text_widget::render()
{
#ifdef DEBUGRELEASE
    bool bDebug = *(bool*)&rc.size.z;
    if (bDebug) {
        F32 rc_x0 = rc.loc.x * FB_XRES;
        F32 rc_y0 = rc.loc.y * FB_YRES;
        F32 rc_x2 = rc_x0 + rc.size.x * FB_XRES;
        F32 rc_y2 = rc_y0 + rc.size.y * FB_YRES;
        F32 rc_x1 = (rc_x0 + rc_x2) * 0.5f;
        F32 rc_y1 = (rc_y0 + rc_y2) * 0.5f;
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)NULL);
        xScrFxDrawBox(rc_x0, rc_y0, rc_x1, rc_y2, 192, 128, 128, 200, false);
        xScrFxDrawBox(rc_x1, rc_y0, rc_x2, rc_y2, 144, 144, 192, 200, false);
        return;
    }
#endif

    xASSERT(125, tb.font.width);
    xASSERT(126, tb.font.height);
    tb.render(true);
}

void text_widget::set_text(const char* intext)
{
    xASSERTMSG(133, (intext), "String is not set !");

    size_t textchars = strlen(intext);
    size_t copychars = xmin(textchars, sizeof(text) - 1);

    memcpy(text, intext, copychars);
    text[copychars] = '\0';
}

#ifndef NON_MATCHING
static void __unused(widget& w, motive& m)
{
    text_widget& textWid = (text_widget&)w;
    xASSERT(0, m.value != 0);
    xASSERT(0, textWid.tb.font.width);
    xASSERT(0, textWid.tb.font.height);
    xASSERT(0, m.orig_height);
    xASSERT(0, m.orig_width);
    xASSERT(0, m.current_scale != 0);
}
#endif

}
