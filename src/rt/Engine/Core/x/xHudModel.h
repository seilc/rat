#ifndef XHUDMODEL_H
#define XHUDMODEL_H

#include "xHud.h"

namespace xhud {

struct model_asset;

class model_widget : public widget
{
public:
    struct widget_chunk;

    U32 mid;
    xModelInstance* model;
    F32 x_scale;
    F32 y_scale;

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);

    model_widget(const model_asset& a);
    
    void destruct();
    virtual void destroy();
    virtual U32 type() const;
    virtual bool is(U32 id) const;
    virtual void update(F32 dt);
    virtual void set_params(xModelInstance*& model, render_context& rc, F32 xs, F32 ys, F32 lx, F32 ly);
    virtual void render();

#ifdef DEBUGRELEASE
    virtual const char* category() { return "Model"; }
#endif
};

struct model_widget::widget_chunk : xBase
{
    model_widget w;
};

}

#endif
