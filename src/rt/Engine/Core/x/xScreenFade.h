#ifndef XSCREENFADE_H
#define XSCREENFADE_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xColor.h"

enum eScreenFadeState
{
    eScreenFadeStateOff,
    eScreenFadeStateFadeDown,
    eScreenFadeStateWait,
    eScreenFadeStateFadeUp,
    eScreenFadeStateMax
};

struct xScreenFade : xBase
{
    struct asset_type : xDynAsset
    {
        static const U16 VERSION = 1;
        
        xColor dest;
        F32 fadeDownTime;
        F32 waitTime;
        F32 fadeUpTime;

        static const char* type_name() { return "effect:ScreenFade"; }
    };

    asset_type* asset;
    eScreenFadeState state;
    xColor src;
    xColor current_color;
    F32 time_passed;
    xBase* owner;

    void load(asset_type& a);
    U8 InterpCol(F32 t, U8 s, U8 d);
    void StartFade();
    void update(F32 seconds);
    void render();
    void reset();

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);
    static void update_all(F32 dt);
    static void render_all();
    static void cb_dispatch(xBase* from, xBase* to, U32 event, const F32*, xBase*, U32);
};


#endif
