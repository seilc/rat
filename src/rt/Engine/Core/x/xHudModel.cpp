#include "xHudModel.h"

#include "xHudAsset.h"

#ifndef NON_MATCHING
xAnimTable* xHUDCreateAnimTable()
{
    xAnimTable* table = xAnimTableNew("xHUD", 0);
    xASSERT(0, table);
    xAnimTableNewState(table, "Idle01", 0, 0, 1.0f, 0, 0, 1.0f, 0, 0, 0, 0, 0);
    return table;
}
#endif

namespace xhud {

void model_widget::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(40, asset, model_asset, model_asset::type_name());

    init_base(data, asset, sizeof(widget_chunk));

    widget_chunk& wc = (widget_chunk&)data;
    new (&wc.w) model_widget((model_asset&)asset);
}

model_widget::model_widget(const model_asset& a)
    : widget(a)
    , mid(a.model)
    , model(0)
{
    model = xhud::load_model(mid);
    xASSERTM(50, model != 0, "Could not load hud model: %s", xSTAssetNameOrID(mid));

    x_scale = 1.0f;
    y_scale = 1.0f;
}

void model_widget::destruct()
{
    widget::destruct();
}

void model_widget::destroy()
{
    destruct();
}

U32 model_widget::type() const
{
    static U32 myid = xStrHash(model_asset::type_name());
    return myid;
}

bool model_widget::is(U32 id) const
{
    return id == model_widget::type() || widget::is(id);
}

void model_widget::update(F32 dt)
{
    updater(dt);

    if (!visible() || rc.a <= 0.5f/255.0f) return;

    xModelUpdate(model, dt);
    xModelEval(model);
}

void model_widget::set_params(xModelInstance*& model, render_context& rc, F32 xs, F32 ys, F32 lx, F32 ly)
{
    x_scale = xs;
    y_scale = ys;
    model->Scale.x = xs;
    model->Scale.y = ys;
    rc.loc.x = lx;
    rc.loc.y = ly;
}

void model_widget::render()
{
    if (xfeq0(rc.a)) return;

    model->Scale.x = x_scale;
    model->Scale.y = y_scale;
    model->Scale.z = 1.0f;

    xhud::render_model(*model, rc);
}

}
