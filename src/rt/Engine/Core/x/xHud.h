#ifndef XHUD_H
#define XHUD_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xModel.h"
#include "xColor.h"

#include <stddef.h>

namespace xhud {

struct render_context;
struct motive;
struct motive_node;
struct asset;
struct widget;

struct render_context
{
    xVec3 loc;
    xVec3 size;
    xVec3 rot;
    F32 r;
    F32 g;
    F32 b;
    F32 a;
};

struct motive
{
    union
    {
        F32* value;
        F32 time_to_blink;
    };
    union
    {
        F32 delta;
        F32 start_blink_period;
    };
    union
    {
        F32 max_offset;
        F32 begin_scale;
        F32 end_blink_period;
    };
    union
    {
        F32 accel;
        F32 final_scale;
        F32 fade_blink_time;
    };
    union
    {
        void* context;
        F32 begin_alpha;
        xColor on_blink_color;
    };
    union
    {
        F32 period;
        F32 end_alpha;
        xColor off_blink_color;
    };
    union
    {
        F32 elapsed;
        F32 orig_width;
    };
    union
    {
        F32 offset;
        F32 current_scale;
        F32 current_blink_time;
    };
    bool(*fp_update)(widget&, motive&, F32);
    bool(*fp_finish)(widget&, motive&);
    union
    {
        F32 start_delta;
        F32 orig_height;
        F32 period_blink_time;
    };
    bool hold;
    bool inverse;
};

class widget
{
public:
    struct
    {
        bool visible;
        bool enabled;
        bool active;
        F32 timer;
    } flag;
    render_context rc;
    render_context start_rc;
    const asset* a;
    enum
    {
        ACT_NONE,
        ACT_SHOW,
        ACT_HIDE,
        MAX_ACT
    } activity;

    widget(const asset& a);

    static void init_base(xBase& data, const xBaseAsset& asset, size_t chunk_size);

    void destruct();
    void updater(F32 dt);
    bool visible() const;

    virtual void destroy();
    virtual U32 type() const;
    virtual bool is(U32 id) const;
    virtual void init();
    virtual void setup();
    virtual void update(F32 dt);
    virtual void render();
    virtual void dispatch(xBase* from, U32 event, const F32* argf, xBase* argw, U32 argwi);

#ifdef DEBUGRELEASE
    virtual const char* category();
    virtual void add_tweaks(const char* type_prefix, const char* object_prefix);
#endif

private:
    motive_node* _motive_top;
    motive_node* _motive_temp;
    motive_node** _motive_temp_tail;
};

xModelInstance* load_model(U32 id);
void render_model(xModelInstance& m, const render_context& rc);

}

#endif
