#ifndef XFONT_H
#define XFONT_H

#include "xColor.h"
#include "xMath2.h"
#include "xString.h"

F32 NSCREENX(F32 v);
F32 NSCREENY(F32 v);

class xfont
{
public:
    U32 id;
    F32 width;
    F32 height;
    F32 space;
    xColor color;
    xColor shadowColor;
    F32 shadowOffsetX;
    F32 shadowOffsetY;
    basic_rect<F32> clip;

    static xfont create(U32 id, F32 width, F32 height, F32 space, xColor color, const basic_rect<F32>& clip, xColor shadowColor, F32 shadowOffsetX, F32 shadowOffsetY);

    void start_render() const;
    void stop_render() const;
    void irender(const char* text, F32 x, F32 y) const;

    void render(const char* text, F32 x, F32 y) const
    {
        start_render();
        irender(text, x, y);
        stop_render();
    }
};

class xtextbox
{
public:
    struct jot;

    struct split_tag
    {
        substr tag;
        substr name;
        substr action;
        substr value;
    };

    struct tag_type
    {
        substr name;
        void(*parse_tag)(jot&, xtextbox&, xtextbox&, split_tag&);
        void(*reset_tag)(jot&, xtextbox&, xtextbox&, split_tag&);
        void* context;
    };

    struct callback
    {
        void(*render)(jot&, xtextbox&, F32, F32);
        void(*layout_update)(jot&, xtextbox&, xtextbox&);
        void(*render_update)(jot&, xtextbox&, xtextbox&);
    };

    struct jot
    {
        substr s;
        struct
        {
            bool invisible : 1;
            bool ethereal : 1;
            bool merge : 1;
            bool word_break : 1;
            bool word_end : 1;
            bool line_break : 1;
            bool stop : 1;
            bool tab : 1;
            bool insert : 1;
            bool dynamic : 1;
            bool page_break : 1;
            bool stateful : 1;
            bool japanese_break : 1;
            U16 dummy : 3;
        } flag;
        U16 context_size;
        void* context;
        basic_rect<F32> bounds;
        basic_rect<F32> render_bounds;
        const callback* cb;
        const tag_type* tag;
    };

    xfont font;
    basic_rect<F32> bounds;
    U32 flags;
    F32 line_space;
    F32 tab_stop;
    F32 left_indent;
    F32 right_indent;
    const callback* cb;
    void* context;

    static xtextbox create();
    static xtextbox create(const xfont& font, const basic_rect<F32>& bounds, U32 flags, F32 line_space, F32 tab_stop, F32 left_indent, F32 right_indent);

    void set_text(const char* text);
    void render(bool cache) const;

private:
    const char** texts;
    const size_t* text_sizes;
    size_t texts_size;
    substr text;
    U32 text_hash;
};

static const basic_rect<F32> screen_bounds = { 0, 0, 1, 1 };

#endif
