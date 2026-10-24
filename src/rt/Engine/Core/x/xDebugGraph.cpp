#include "xDebugGraph.h"

#include "xColor.h"
#include "xDebug.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xMath2.h"
#include "xString.h"
#include "xutil.h"
#include "zMain.h"

#include <stdio.h>
#include <string.h>

namespace {
    enum
    {
        FLAG_RENDER = 0x1,
        FLAG_PAUSE = 0x2,
        FLAG_AUTO_SCALE = 0x4,
        FLAG_BIND_INT = 0x8
    };

    static const S32 MAX_GRAPHS = 128;

    const char global_tweak_prefix[] = "Graph|\001Globals|";

    namespace tweak {
        bool render_all;
        bool render_none;

        void add_tweaks(const char* prefix)
        {
            xAUTOTWEAKBOOL(prefix, "render_all", &render_all, NULL, NULL, 0, true);
            xAUTOTWEAKBOOL(prefix, "render_none", &render_none, NULL, NULL, 0, true);
        }
    }

    struct graph_context
    {
        char name[128];
        S32 flags;
        F32 scale;
        F32 originValue;
        F32 offset;
        xColor color;
        F32 update_delay;
        F32 update_time;
        void* binding;
        F32 data[128];
        S32 start;
        S32 size;
        graph_context* next;
    };

    graph_context* graph_buffer;
    graph_context* head_free_graph;

    struct graph_handle
    {
        union
        {
            graph_context* graph;
            char* name;
        };
    };

    graph_handle* sorted_graphs;
    S32 graphs_size;
    bool need_sort;
    xVec2* point_buffer;

    const xColor common_colors[] =
    {
        { 0xFF, 0x00, 0x00, 0xFF },
        { 0x00, 0xFF, 0x00, 0xFF },
        { 0x00, 0x00, 0xFF, 0xFF },
        { 0x00, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0x00, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0x20, 0x00, 0xFF },
        { 0x20, 0xFF, 0x00, 0xFF },
        { 0x20, 0x20, 0xFF, 0xFF },
        { 0xF0, 0x80, 0x80, 0xFF },
        { 0xBC, 0x40, 0x99, 0xFF },
        { 0x80, 0x00, 0x00, 0xFF },
        { 0x19, 0x19, 0x70, 0xFF },
        { 0x80, 0xCC, 0x99, 0xFF },
        { 0x33, 0xB3, 0x4D, 0xFF },
        { 0xD7, 0xDC, 0x13, 0xFF },
        { 0xFF, 0xA5, 0x00, 0xFF },
        { 0xF0, 0xE6, 0x8C, 0xFF },
        { 0xC8, 0x69, 0xE5, 0xFF },
        { 0xFF, 0x80, 0xFF, 0xFF }
    };

    const char* common_color_names[] =
    {
        "red",
        "green",
        "blue",
        "cyan",
        "magenta",
        "yellow",
        "white",
        "neon_red",
        "neon_green",
        "neon_blue",
        "peach",
        "fuschia",
        "maroon",
        "midnightblue",
        "seagreen",
        "forestgreen",
        "pimp_gold",
        "orange",
        "khaki",
        "lavender",
        "pink"
    };

    xColor choose_color(const char* name)
    {
        U32 namehash = (xStrHash(name) + 7349) * 2003 / 4789;
        return common_colors[namehash % ARRAY_LENGTH(common_colors)];
    }

    char* get_tweak_prefix(const char* name)
    {
        static char buffer[128];
        const char* sep = strrchr(name, '|');
        if (!sep) {
            sprintf(buffer, "Graph|%s|", name);
            return buffer;
        } else {
            return (char*)name;
        }
    }

    void on_change_scale(const tweak_info& info)
    {
        graph_context& graph = *(graph_context*)info.context;
        if (graph.scale <= 0.0f) {
            graph.flags |= FLAG_AUTO_SCALE;
            graph.scale = 0.0f;
        } else {
            graph.flags &= ~FLAG_AUTO_SCALE;
        }
    }

    const tweak_callback cb_change_scale = { on_change_scale };

    void add_graph_tweaks(graph_context& graph)
    {
        const char* prefix = get_tweak_prefix(graph.name);

        xAUTOTWEAKFLAG(prefix, "render", &graph.flags, FLAG_RENDER, NULL, NULL, 0);
        xAUTOTWEAKFLAG(prefix, "pause", &graph.flags, FLAG_PAUSE, NULL, NULL, 0);
        xAUTOTWEAKRANGE(prefix, "scale", &graph.scale, -1.0f, 1e9f, &cb_change_scale, &graph, 0, false);
        xAUTOTWEAKRANGE(prefix, "originValue", &graph.originValue, -1e9f, 1e9f, NULL, NULL, 0, false);
        xAUTOTWEAKRANGE(prefix, "offset", &graph.offset, -1000.0f, 1000.0f, NULL, NULL, 0, false);
        xAUTOTWEAKSELECT(prefix, "color", (U32*)&graph.color, common_color_names, (const U32*)common_colors, ARRAY_LENGTH(common_colors), NULL, NULL, 0, false);
    }

    void remove_graph_tweaks(graph_context& graph)
    {
        const char* prefix = get_tweak_prefix(graph.name);
        char buffer[128];
        strcpy(buffer, prefix);
        char* tail = buffer + strlen(buffer);

        strcpy(tail, "render"); xTWEAKREMOVE(buffer);
        strcpy(tail, "pause"); xTWEAKREMOVE(buffer);
        strcpy(tail, "scale"); xTWEAKREMOVE(buffer);
        strcpy(tail, "originValue"); xTWEAKREMOVE(buffer);
        strcpy(tail, "offset"); xTWEAKREMOVE(buffer);
        strcpy(tail, "update_delay"); xTWEAKREMOVE(buffer);
        strcpy(tail, "color"); xTWEAKREMOVE(buffer);
    }

    void create_graph(graph_context& graph, const char* name)
    {
        strcpy(graph.name, name);
        graph.flags = FLAG_AUTO_SCALE;
        graph.scale = 0.0f;
        graph.color = choose_color(name);
        graph.update_delay = graph.update_time = 0.0f;
        graph.binding = NULL;
        graph.start = graph.size = 0;

        add_graph_tweaks(graph);
    }

    void destroy_graph(graph_context& graph)
    {
        remove_graph_tweaks(graph);
    }

    graph_handle* alloc_graph()
    {
        xASSERT(213, graphs_size < MAX_GRAPHS);
        graph_context* g = head_free_graph;
        head_free_graph = g->next;
        graph_handle* gh = sorted_graphs + graphs_size;
        gh->graph = g;
        graphs_size++;
        need_sort = true;
        return gh;
    }

    graph_handle* find_graph(const char* name)
    {
        if (need_sort) {
            quicksort_istr(sorted_graphs, graphs_size, sizeof(graph_handle), 0);
            need_sort = false;
        }

        graph_handle* tail = sorted_graphs + graphs_size;
        graph_handle* gh = (graph_handle*)binary_find((const char**)sorted_graphs, (const char**)tail, name, stricmp);

        return gh == tail ? NULL : gh;
    }

    graph_handle* grab_graph(const char* name)
    {
        graph_handle* gh = find_graph(name);
        if (gh) {
            return gh;
        }
        if (graphs_size >= MAX_GRAPHS) {
            return NULL;
        }
        gh = alloc_graph();
        create_graph(*gh->graph, name);
        return gh;
    }

    void add_value(graph_context& graph, F32 value)
    {
        xFIXNAN(261, value);

        if (graph.flags & FLAG_AUTO_SCALE) {
            if (graph.scale < xabs(value - graph.originValue)) {
                graph.scale = xabs(value - graph.originValue);
            }
        }

        if (graph.size >= 128) {
            if (!(graph.flags & FLAG_PAUSE)) {
                graph.start = (graph.start + 1) & 127;
            }
        } else {
            graph.size++;
        }

        F32& d = graph.data[(graph.start + graph.size - 1) & 127];
        d = value;
    }

    void bind_graph(const char* name, const void* binding, F32 update_delay, S32 flags)
    {
        graph_handle* gh = grab_graph(name);
        if (!gh) return;
        graph_context& graph = *gh->graph;

        graph.binding = (void*)binding;
        graph.update_delay = update_delay;
        graph.update_time = 0.0f;
        graph.flags |= flags;

        const char* prefix = get_tweak_prefix(graph.name);
        if (!binding) {
            char buffer[128];
            sprintf(buffer, "%supdate_delay", prefix);
            xTWEAKREMOVE(buffer);
        } else {
            xAUTOTWEAKRANGE(prefix, "update_delay", &graph.update_delay, 0.0f, 1e9f, NULL, NULL, 0, false);
        }
    }
}

void xDebugGraphSceneEnter()
{
    S32 blech = 3;
    S32 blah = 4;
    graph_buffer = (graph_context*)xDebugTweak_StaticAlloc(MAX_GRAPHS * sizeof(graph_context));
    sorted_graphs = (graph_handle*)xDebugTweak_StaticAlloc(MAX_GRAPHS * sizeof(graph_handle));
    point_buffer = (xVec2*)xDebugTweak_StaticAlloc(128 * sizeof(xVec2));

    graphs_size = 0;
    head_free_graph = graph_buffer;
    graph_context* g = head_free_graph, *endg = g + MAX_GRAPHS - 1;
    for (; g != endg; ++g)
        g->next = g + 1;
    endg->next = NULL;

    tweak::add_tweaks(global_tweak_prefix);
}

void xDebugGraphSceneExit()
{
    xDebugRemoveTweaks(global_tweak_prefix, NULL, NULL);

    graph_handle* gh = sorted_graphs, *endgh = gh + graphs_size;
    for (; gh != endgh; ++gh) {
        graph_context& graph = *gh->graph;
        destroy_graph(graph);
    }
}

void xDebugGraphUpdate(F32 dt)
{
    graph_handle* gh = sorted_graphs, *endgh = gh + graphs_size;
    for (; gh != endgh; ++gh) {
        graph_context& graph = *gh->graph;
        if (!graph.binding) continue;

        graph.update_time += dt;
        if (graph.update_time < graph.update_delay) continue;

        graph.update_time -= graph.update_delay;
        if (graph.update_time > graph.update_delay) {
            graph.update_time = graph.update_delay;
        }
        if (graph.flags & FLAG_BIND_INT)
            add_value(graph, *(S32*)graph.binding);
        else
            add_value(graph, *(F32*)graph.binding);
    }
}

void xDebugGraphRender()
{
    if (tweak::render_none) return;

    xVec2* point = point_buffer;
    F32 sx = 0.05f * FB_XRES, sy = 0.05f * FB_YRES;
    F32 sw = FB_XRES - 2.0f * sx, sh = FB_YRES - 2.0f * sy;
    F32 xoffset = sx;
    F32 yoffset = sy + 0.5f * sh;
    F32 xscale = sw / 127.0f;
    S32 rendered = 0;

    graph_handle* gh = sorted_graphs, *endgh = gh + graphs_size;
    for (; gh != endgh; ++gh) {
        graph_context& graph = *gh->graph;
        if (!tweak::render_all && !(graph.flags & FLAG_RENDER)) continue;

        rendered++;
        if (graph.size <= 1 || graph.scale <= 0.0f) continue;

        F32 yscale = -0.5f * sh / graph.scale;
        for (S32 i = 0; i < graph.size; ++i) {
            F32 value = graph.data[(graph.start + i) & 127];
            point[i].x = xoffset + xscale * i;
            point[i].y = yoffset + yscale * (value - graph.originValue) - graph.offset;
        }

        xDrawSetColor(graph.color);
        xDrawPolyLine2D(point, graph.size);
    }

    if (rendered > 0) {
        xColor color = { 0x80, 0x80, 0x80, 0x40 };
        xDrawSetColor(color);
        xDrawLine2D(xoffset, yoffset, xoffset + sw, yoffset);
    }
}

void xDebugGraphSetScale(const char* name, F32 scale)
{
    graph_handle* gh = scale <= 0.0f ? find_graph(name) : grab_graph(name);
    if (!gh) return;
    graph_context& graph = *gh->graph;
    if (scale <= 0.0f) {
        graph.flags |= FLAG_AUTO_SCALE;
        graph.scale = 0.0f;
    } else {
        graph.flags &= ~FLAG_AUTO_SCALE;
        graph.scale = scale;
    }
}

void xDebugGraphSetOriginValue(const char* name, F32 originValue)
{
    graph_handle* gh = grab_graph(name);
    if (!gh) return;
    graph_context& graph = *gh->graph;
    graph.originValue = originValue;
}

void xDebugGraphAdd(const char* name, F32 value)
{
    graph_handle* gh = grab_graph(name);
    if (!gh) return;
    graph_context& graph = *gh->graph;
    add_value(graph, value);
}

void xDebugGraphAdd(const char* name, S32 value)
{
    graph_handle* gh = grab_graph(name);
    if (!gh) return;
    graph_context& graph = *gh->graph;
    add_value(graph, value);
}

void xDebugGraphBind(const char* name, const F32* value, F32 update_delay)
{
    bind_graph(name, value, update_delay, 0);
}
