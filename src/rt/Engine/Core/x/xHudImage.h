#ifndef XHUDIMAGE_H
#define XHUDIMAGE_H

#include "xHud.h"

namespace xhud {

struct image_asset;

class image_widget : public widget
{
public:
    enum RENDERTYPE
    {
        TOPLEFT,
        BOTTOMLEFT,
        BOTTOMRIGHT,
        TOPRIGHT,
        OFFSET_CENTERPT,
        OFFSET_PIE,
        STROBE_EFFECT,
        ROTATE_CCW,
        OFFSET_ROTATE_CCW,
        VARIABLE_TOP,
        VARIABLE_RIGHT,
        COUNTER_CLOCKWISE,
        HALF_CCW,
        RANDOM_V,
        SPECIAL_STROBE
    };

    struct widget_chunk;

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);

    image_widget(const image_asset& a);

    void destruct();
    virtual void destroy();
    virtual U32 type() const;
    virtual bool is(U32 id) const;
    virtual void update(F32 dt);
    void update_strobe_effect(U32 uLayer, F32 dt);
    virtual void render();
    bool render_one_to_one(U32 uLayer, RENDERTYPE eType);
    bool render_variable_top(U32 uLayer);
    bool render_variable_right(U32 uLayer);
    bool render_counter_clockwise(U32 uLayer);
    bool render_half_ccw(U32 uLayer);
    bool render_rotate_ccw(U32 uLayer);
    bool render_offset_rotate_ccw(U32 uLayer);
    bool render_offset_pie(U32 uLayer);
    bool render_strobe_effect(U32 uLayer);
    bool render_special_strobe(U32 uLayer);
    bool render_random_v(U32 uLayer);
    void set_buffer_to_layer(RENDERTYPE eType);

#if DEBUG || RELEASE
    virtual const char* category();
#endif

protected:
    struct LayerInfo;

    RwRaster* m_pRaster;
    LayerInfo* m_pLayers;
    U32 m_uLayerCount;
    U32 m_uRenderLayerCount;
    RwIm2DVertex* m_pVertices;
    RwIm2DVertex* m_paVertices;
    U16* m_paIndices;
    U16 m_uVertexCount;
    U16 m_uIndexCount;
    F32 m_fRecipZ;
    U16 m_VertexMax;
    U16 m_IndexMax;
};

struct image_widget::widget_chunk : xBase
{
    image_widget w;
};

}

#endif
