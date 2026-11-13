#include "xSpacePartitionTree.h"

#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "zPlayer.h"

#include <stdio.h>
#include <string.h>

namespace {

namespace tweak {

#if DEBUG || RELEASE
bool render_intersect;
bool render_empty;
S32 cast_player_point;
S32 cast_player_sphere;
S32 cast_player_line;
F32 cast_line_length = 10.0f;
bool validate;
#endif

#if DEBUG || RELEASE
void add_tweaks()
{
    static bool added = false;
    if (!added) {
        added = true;
        const char* prefix = "Debugging Options|\1Space Partition Tree|";

        xAUTOTWEAK_BOOL(prefix, "render_intersect", &render_intersect, NULL, NULL, 0, true);
        xAUTOTWEAK_BOOL(prefix, "render_empty", &render_empty, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "cast_player_point", &cast_player_point, 0, 1000, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "cast_player_sphere", &cast_player_sphere, 0, 1000, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "cast_player_line", &cast_player_line, 0, 1000, NULL, NULL, 0, true);
        xAUTOTWEAK(prefix, "cast_line_length", &cast_line_length, 0.0f, 1e9f, NULL, NULL, 0, true);
        xAUTOTWEAK_BOOL(prefix, "validate", &validate, NULL, NULL, 0, true);
    }
}
#else
void add_tweaks()
{
}
#endif

} // namespace tweak

} // namespace

void xSpacePartitionTree2D::create(F32 minx, F32 miny, F32 maxx, F32 maxy, F32 min_width, F32 max_width, S32 max_entries)
{
    origin.assign(minx, miny);
    size.assign(maxx - minx, maxy - miny);

    grid_shift_x = get_grid_shift(size.x, max_width);
    grid_size_x = 1 << grid_shift_x;
    grid_mask_x = grid_size_x - 1;
    grid_shift_y = get_grid_shift(size.y, max_width);
    grid_size_y = 1 << grid_shift_y;
    grid_mask_y = grid_size_y - 1;

    xVec2 gsize = { size.x / grid_size_x, size.y / grid_size_y };
    cell_size[0] = MAX(gsize.x, gsize.y);
    cell_isize[0] = 1.0f / cell_size[0];
    cell_frac[0] = 1.0f;
    size.assign(cell_size[0] * grid_size_x, cell_size[0] * grid_size_y);

    xASSERT_L(71, min_width > 0.0f);
    ilogdepth = -1.0f / xlog(2.0f);
    if (min_width > cell_size[0])
        max_depth = 0;
    else
        max_depth = (S32)(ilogdepth * xlog(min_width / cell_size[0]));
    if (max_depth > MAX_DEPTH) max_depth = MAX_DEPTH;

    F32* cs = cell_size + 1, *end_cs = cs + max_depth;
    F32* ics = cell_isize + 1;
    F32* cf = cell_frac + 1;
    for (; cs != end_cs; ++cs, ++ics, ++cf) {
        *cs = 0.5f * cs[-1];
        *ics = 2.0f * ics[-1];
        *cf = 0.5f * cf[-1];
    }

    node_buffer_size = max_entries * 4;
    node_buffer = new (eMemStaticType_0, eMemMgrTag_EnvSpatial) entry_node[node_buffer_size];

    entry_buffer_size = max_entries;
    entry_buffer = new (eMemStaticType_0, eMemMgrTag_EnvSpatial) entry_data[entry_buffer_size];

    cell_buffer_size = (S32)(4.0f * max_entries);
    cell_buffer_size = (cell_buffer_size + 3) / 4 * 4 + grid_size_x * grid_size_y;
    cell_buffer = new (eMemStaticType_0, eMemMgrTag_EnvSpatial) cell[cell_buffer_size];
    grid = cell_buffer;

    clear();

    tweak::add_tweaks();
}

void xSpacePartitionTree2D::clear()
{
#if DEBUG || RELEASE
    memset(entry_buffer, 0, entry_buffer_size * sizeof(entry_data));
    memset(cell_buffer, 0, cell_buffer_size * sizeof(cell));
#endif

    cell* gc = grid, *end_gc = gc + grid_size_x * grid_size_y;
    for (; gc != end_gc; ++gc)
        create_cell(*gc, INVALID_INDEX, 0);

    free_entries = entry_buffer;
    entry_data* fe = entry_buffer, *end_fe = fe + entry_buffer_size - 1;
    for (; fe != end_fe; ++fe) fe->next_free_entry = fe + 1;
    end_fe->next_free_entry = NULL;

#if DEBUG || RELEASE
    fe = entry_buffer;
    end_fe = fe + entry_buffer_size;
    for (; fe != end_fe; ++fe) {
        fe->type = -1;
        for (S32 i = 0; i < MAX_OWNERS; ++i)
            fe->owner[i] = INVALID_INDEX;
    }
#endif

    free_nodes = node_buffer;
    entry_node* fn = node_buffer, *end_fn = fn + node_buffer_size - 1;
    for (; fn != end_fn; ++fn) fn->next_free_node = fn + 1;
    end_fn->next_free_node = NULL;

    S32 grid_size = grid_size_x * grid_size_y;
    free_cells = cell_buffer + grid_size;
    cell* fc = free_cells, *end_fc = fc + cell_buffer_size - grid_size - 4;
    for (; fc != end_fc; fc = fc->next_free_cell)
        fc->next_free_cell = fc + 4;
    end_fc->next_free_cell = NULL;

    free_cells_size = cell_buffer_size - grid_size;
    safety_free_cells_size = (max_depth + 1) * 4;

#if DEBUG || RELEASE
    total_cells_used = 0;
#endif

    current_searchid = 0;
    reset_searchid();
}

void xSpacePartitionTree2D::create_cell(cell& c, U16 parent, S32 cid)
{
    c.parent = parent;
    c.children = INVALID_INDEX;
    c.entries = INVALID_INDEX;
    c.cid = cid;

    if (parent == INVALID_INDEX) {
        c.level = max_depth;
        c.xcode = c.ycode = 0;
    } else {
        cell* const pc = cell_buffer + parent;
        xASSERT_L(181, pc->level > 0);
        c.level = pc->level - 1;

        S32 xbit = cid & 1, ybit = (cid >> 1) & 1;
        c.xcode = (U8)(pc->xcode | (xbit << c.level));
        c.ycode = (U8)(pc->ycode | (ybit << c.level));
    }
}

void xSpacePartitionTree2D::create_children(cell& c)
{
    cell* const cell_list = cell_buffer;
    cell* cc = alloc_children(), *end_cc = cc + 4;
    c.children = cc - cell_list;
    U16 parent = &c - cell_list;
    for (; cc != end_cc; ++cc)
        create_cell(*cc, parent, cc - cell_list - c.children);
    xASSERT_L(199, c.children != 0);
}

void xSpacePartitionTree2D::reset_searchid()
{
    current_searchid = 1;
    entry_data* e = entry_buffer, *end = e + entry_buffer_size;
    for (; e != end; ++e)
        e->searchid = 0;
}

namespace {

struct cb_fp_wrapper
{
    cb_fp_wrapper(xSpacePartitionTree2D::intersect_callback cb, void* context) : cb(cb), context(context) {}

    bool operator()(xSpacePartitionTree2D::entry_data* e) const
    {
        return cb(e, context);
    }

    xSpacePartitionTree2D::intersect_callback cb;
    void* context;
};

struct cb_accum_wrapper
{
    cb_accum_wrapper(xSpacePartitionTree2D::entry_data** results, S32 max_results) : results(results), end_results(results + max_results)
    {
#if DEBUG || RELEASE
        maxResults = max_results;
#endif
    }

    bool operator()(xSpacePartitionTree2D::entry_data* e)
    {
        if (results < end_results) {
            *results = e;
            ++results;
        }
#if DEBUG || RELEASE
        else {
            char tempString[128];
            sprintf(tempString, "There are more than %d triggers around the player/enemy. Not all triggers will work!", maxResults);
            xFAIL_ONCE_L(255, "%s", tempString);
        }
#endif

        return results != end_results;
    }

    xSpacePartitionTree2D::entry_data** results;
    xSpacePartitionTree2D::entry_data** end_results;
#if DEBUG || RELEASE
    S32 maxResults;
#endif
};

} // namespace

// UNUSED: needed to match (speculative code)
void xSpacePartitionTree2D::intersect_point(F32 x, F32 y, intersect_callback cb, void* context)
{
    cb_fp_wrapper fp_wrapper(cb, context);
    intersect_point(x, y, fp_wrapper);
}

// UNUSED: needed to match (speculative code)
S32 xSpacePartitionTree2D::intersect_point(entry_data** results, S32 max_results, F32 x, F32 y)
{
    cb_accum_wrapper accum_wrapper(results, max_results);
    intersect_point(x, y, accum_wrapper);
    return accum_wrapper.results - results;
}

void xSpacePartitionTree2D::intersect_rect(F32 x0, F32 y0, F32 x1, F32 y1, intersect_callback cb, void* context)
{
    cb_fp_wrapper fp_wrapper(cb, context);
    intersect_rect(x0, y0, x1, y1, fp_wrapper);
}

S32 xSpacePartitionTree2D::intersect_rect(entry_data** results, S32 max_results, F32 x0, F32 y0, F32 x1, F32 y1)
{
    cb_accum_wrapper accum_wrapper(results, max_results);
    intersect_rect(x0, y0, x1, y1, accum_wrapper);
    return accum_wrapper.results - results;
}

void xSpacePartitionTree2D::insert_internal(entry_data* e, F32 x0, F32 y0, F32 x1, F32 y1, F32 width)
{
    F32 scale = cell_isize[0];
    x0 = scale * (x0 - origin.x);
    y0 = scale * (y0 - origin.y);
    x1 = scale * (x1 - origin.x);
    y1 = scale * (y1 - origin.y);
    width = scale * width;

    xASSERT_MSG_L(319, (x1 >= 0.0f) && ((int)x0 < grid_size_x), "Out of this world!");
    xASSERT_MSG_L(320, (y1 >= 0.0f) && ((int)y0 < grid_size_y), "Out of this world!");
    xASSERT_MSG_L(321, width <= 1.0f, "Object too damned big");

    S32 gx0 = CLAMP((S32)x0, 0, grid_size_x - 1);
    S32 gy0 = CLAMP((S32)y0, 0, grid_size_y - 1);
    S32 gx1 = CLAMP((S32)x1, 0, grid_size_x - 1);
    S32 gy1 = CLAMP((S32)y1, 0, grid_size_y - 1);
    xASSERT_L(329, (gx1 - gx0 + 1) <= 2);
    xASSERT_L(330, (gy1 - gy0 + 1) <= 2);

    S32 end_depth;
    if (width < 0.0001f) end_depth = max_depth;
    else {
        end_depth = (S32)(ilogdepth * xlog(width));
        if (end_depth > max_depth) end_depth = max_depth;
    }

    e->owner_size = 0;
    for (S32 gy = gy0; gy <= gy1; ++gy)
        for (S32 gx = gx0; gx <= gx1; ++gx)
            insert_internal(e, &grid_cell(gx, gy), end_depth, x0 - gx, y0 - gy, x1 - gx, y1 - gy);

    e->searchid = current_searchid;
}

void xSpacePartitionTree2D::insert_internal(entry_data* e, cell* c, S32 end_depth, F32 x0, F32 y0, F32 x1, F32 y1)
{
    S32 index = 0;
    F32 dxy = cell_frac[1];
    F32 cx0 = 0.0f;
    F32 cy0 = 0.0f;
    F32 cx1 = 1.0f;
    F32 cy1 = 1.0f;
    S32 depth = 0;
    struct { F32 cx0, cy0, cx1, cy1; } bound_stack[MAX_DEPTH];

    cell* const cell_list = cell_buffer;
    entry_node* const node_list = node_buffer;
    entry_data* const entry_list = entry_buffer;

    for (;;) {
descend:
        if (depth == end_depth) {
            xASSERT_L(384, e->owner_size < MAX_OWNERS);
            e->owner[e->owner_size] = c - cell_list;
            entry_node* n = alloc_node();
            xASSERT_L(387, n != 0);
            n->entry = e - entry_list;
            n->next = c->entries;
            c->entries = n - node_list;
            ++e->owner_size;
        } else {
            for (; index < 4; ++index) {
                S32 ix = index & 1, iy = index / 2;
                F32 ccx0 = cx0 + dxy * ix, ccy0 = cy0 + dxy * iy, ccx1 = ccx0 + dxy, ccy1 = ccy0 + dxy;

                if (ccx0 > x1 || ccy0 > y1 || ccx1 < x0 || ccy1 < y0)
                    continue;

                if (c->children == INVALID_INDEX)
                    create_children(*c);
                xASSERT_L(412, c->children != 0);

                cell* cc = cell_list + c->children + index;

                bound_stack[depth].cx0 = cx0; bound_stack[depth].cy0 = cy0;
                bound_stack[depth].cx1 = cx1; bound_stack[depth].cy1 = cy1;

                c = cc;
                index = 0;
                ++depth; dxy = cell_frac[depth + 1];
                cx0 = ccx0; cy0 = ccy0;
                cx1 = ccx1; cy1 = ccy1;
                goto descend;
            }
        }

        if (depth <= 0)
            return;

        --depth;
        index = c->cid + 1;
        dxy = cell_frac[depth + 1];
        cx0 = bound_stack[depth].cx0; cy0 = bound_stack[depth].cy0; cx1 = bound_stack[depth].cx1; cy1 = bound_stack[depth].cy1;
        c = cell_list + c->parent;
    }
}

void xSpacePartitionTree2D::erase_internal(entry_data* e)
{
    xASSERT_L(447, (e->owner_size > 0) && (e->owner_size <= MAX_OWNERS));
#if DEBUG || RELEASE
    xASSERT_L(450, e->type != -1);
    for (S32 i = 0; i < e->owner_size; ++i)
        xASSERT_L(452, e->owner[i] != INVALID_INDEX);
    for (S32 i = e->owner_size; i < MAX_OWNERS; ++i)
        xASSERT_L(454, e->owner[i] == INVALID_INDEX);
#endif

    cell* const cell_list = cell_buffer;
    entry_node* const node_list = node_buffer;
    entry_data* const entry_list = entry_buffer;

    U16* owner = e->owner, *end_owner = owner + e->owner_size;
    for (; owner != end_owner; ++owner) {
        xASSERT_L(471, *owner != INVALID_INDEX);
        cell* c = cell_list + *owner;

        xASSERT_L(476, c->entries != INVALID_INDEX);

        entry_node* prev = NULL, *n = node_list + c->entries;
        U16 key = e - entry_list;
        for (;;) {
            xASSERT_L(481, n != 0);
            if (n->entry == key) break;
            prev = n;
            xASSERT_L(484, n->next != INVALID_INDEX);
            n = node_list + n->next;
        }

        if (prev == NULL) c->entries = n->next;
        else prev->next = n->next;
        free_node(n);

        remove_empty(c);
    }

#if DEBUG || RELEASE
    e->owner_size = 0;
    for (S32 i = 0; i < MAX_OWNERS; ++i)
        e->owner[i] = INVALID_INDEX;
#endif
}

void xSpacePartitionTree2D::remove_empty(cell* c)
{
    cell* const cell_list = cell_buffer;

    U16 parent = c->parent;
    while (parent != INVALID_INDEX) {
        cell* pc = cell_list + parent;
        xASSERT_L(512, pc->children != INVALID_INDEX);

        parent = pc->parent;
        cell* bro = cell_list + pc->children, *end_bro = bro + 4;
        for (; bro != end_bro; ++bro) {
            if (bro->entries != INVALID_INDEX || bro->children != INVALID_INDEX)
                return;
        }
        free_children(cell_list + pc->children);
        pc->children = INVALID_INDEX;
    }
}

S32 xSpacePartitionTree2D::get_cell_bound(const cell* c, F32& x, F32& y, F32& r) const
{
    cell* const cell_list = cell_buffer;
    S32 depth = 0;
    x = 0.0f; y = 0.0f;
    r = cell_size[0];
    for (; c->parent != INVALID_INDEX; c = cell_list + c->parent, ++depth) {
        S32 ix = (U32)c->cid & 1, iy = c->cid / 2;
        x += ix; y += iy;
        x *= 0.5f; y *= 0.5f;
        r *= 0.5f;
    }
    S32 grid_index = c - grid, gx = grid_index & grid_mask_x, gy = grid_index >> grid_shift_x;
    x *= cell_size[0];
    y *= cell_size[0];
    x += origin.x + gx * cell_size[0];
    y += origin.y + gy * cell_size[0];
    return depth;
}

#define CLIP_MIN(v0, u0, v1, u1)                \
do {                                            \
    if ((v1) >= 0.0f) {                         \
        if (!((v0) >= 0.0f)) {                  \
            F32 s = -(v0) / ((v1) - (v0));      \
            (v0) = 0.0f;                        \
            (u0) = (u0) + s * ((u1) - (u0));    \
        }                                       \
    } else if ((v0) >= 0.0f) {                  \
        F32 s = -(v0) / ((v1) - (v0));          \
        (v1) = 0.0f;                            \
        (u1) = (u0) + s * ((u1) - (u0));        \
    } else {                                    \
        return false;                           \
    }                                           \
} while (0)

#define CLIP_MAX(v0, u0, v1, u1, c)                                                               \
do {                                                                                              \
    F32 _c = (c);                                                                                 \
    if ((v1) < _c) {                                                                              \
        if (!((v0) < _c)) {                                                                       \
            F32 s = (_c - (v0)) / ((v1) - (v0));                                                  \
            (v0) = _c;                                                                            \
            (u0) = (u0) + s * ((u1) - (u0));                                                      \
        }                                                                                         \
    } else if ((v0) < _c) {                                                                       \
        F32 s = (_c - (v0)) / ((v1) - (v0));                                                      \
        (v1) = _c;                                                                                \
        (u1) = (u0) + s * ((u1) - (u0));                                                          \
    } else {                                                                                      \
        return false;                                                                             \
    }                                                                                             \
} while (0)

bool xSpacePartitionTree2D::clip_line(F32& x0, F32& y0, F32& x1, F32& y1) const
{
    CLIP_MIN(x0, y0, x1, y1);
    CLIP_MAX(x0, y0, x1, y1, grid_size_x);
    CLIP_MIN(y0, x0, y1, x1);
    CLIP_MAX(y0, x0, y1, x1, grid_size_y);
    return true;
}

#define SET_INFINITE(v, sign)                   \
do {                                            \
    (v) = 1e9f;                                 \
    if ((sign) < 0.0f) {                        \
        if ((v) > 0.0f) (v) = -(v);             \
    } else {                                    \
        if ((v) < 0.0f) (v) = -(v);             \
    }                                           \
} while (0)

bool xSpacePartitionTree2D::intersect_line_setup(F32 x0, F32 y0, F32 x1, F32 y1, S32& ix, S32& iy, S32& dix, S32& diy, S32& end_ix, S32& end_iy, S32& a, F32& tx0, F32& ty0, F32& tx1, F32& ty1, F32& dtx, F32& dty)
{
    x0 = cell_isize[0] * (x0 - origin.x);
    y0 = cell_isize[0] * (y0 - origin.y);
    x1 = cell_isize[0] * (x1 - origin.x);
    y1 = cell_isize[0] * (y1 - origin.y);

    if (!clip_line(x0, y0, x1, y1))
        return false;

    ++current_searchid;
    if (current_searchid == 0)
        reset_searchid();

    F32 dx = x1 - x0;
    F32 dy = y1 - y0, idx, idy;
    if (xfeq0(dx)) SET_INFINITE(idx, dx);
    else idx = 1.0f / dx;
    if (xfeq0(dy)) SET_INFINITE(idy, dy);
    else idy = 1.0f / dy;

    ix = (S32)x0;
    iy = (S32)y0;
    dix = 1 - 2 * xsignbit(dx);
    diy = 1 - 2 * xsignbit(dy);
    end_ix = (S32)(x1 + 0.999f * dix);
    end_iy = (S32)(y1 + 0.999f * diy);

    a = 0;
    if (dx < 0.0f) a |= 1;
    if (dy < 0.0f) a |= 2;

    dtx = xabs(idx);
    dty = xabs(idy);

    if (dx >= 0.0f) { tx0 = idx * (ix - x0); tx1 = tx0 + idx; }
    else { tx1 = idx * (ix - x0); tx0 = tx1 + idx; }
    if (dy >= 0.0f) { ty0 = idy * (iy - y0); ty1 = ty0 + idy; }
    else { ty1 = idy * (iy - y0); ty0 = ty1 + idy; }

    return true;
}

#if DEBUG || RELEASE

struct cb_test_intersect
{
    S32& count;

    cb_test_intersect(S32& count) : count(count) {}

    bool operator()(xSpacePartitionTree2D::entry_data*) const
    {
        ++count;
        return true;
    }
};

void xSpacePartitionTree2D::debug_update(F32)
{
    S32 count = 0;
    cb_test_intersect cb(count);

    if (tweak::cast_player_point > 0) {
        for (S32 i = 0; i < tweak::cast_player_point; ++i) {
            const xVec3& player_loc = xEntGetFrame(xglobals->players[0])->pos;
            intersect_point(player_loc.x, player_loc.z, cb);
        }
    }

    if (tweak::cast_player_sphere > 0) {
        for (S32 i = 0; i < tweak::cast_player_sphere; ++i) {
            const xSphere& player_bound = xglobals->players[0]->bound.sph;
            intersect_circle(player_bound.center.x, player_bound.center.z, player_bound.r, cb);
        }
    }

    if (tweak::cast_player_line > 0) {
        for (S32 i = 0; i < tweak::cast_player_line; ++i) {
            const xVec3& player_loc = xEntGetFrame(xglobals->players[0])->pos;
            const xVec3& player_dir = xEntGetFrame(xglobals->players[0])->at;
            xVec3 v0 = player_loc, v1;
            v1.AddScale(v0, player_dir, tweak::cast_line_length);
            xDrawSetColor(g_GREEN);
            xDrawLine(&v0, &v1);
            intersect_line(v0.x, v0.z, v1.x, v1.z, cb);
        }
    }

    if (tweak::validate)
        debug_validate();
}

void xSpacePartitionTree2D::debug_validate() const
{
    for (S32 gy = 0; gy < grid_size_y; ++gy) {
        for (S32 gx = 0; gx < grid_size_x; ++gx) {
            const cell* c = &grid_cell(gx, gy);
            xASSERT_L(726, c->parent == INVALID_INDEX);
            debug_validate_cell(c);
        }
    }

    entry_data* e = free_entries;
    for (; e; e = e->next_free_entry) {
        xASSERT_L(736, e->type == -1);
        xASSERT_L(737, e->owner_size == 0);
        for (S32 i = 0; i < MAX_OWNERS; ++i)
            xASSERT_L(739, e->owner[i] == INVALID_INDEX);
    }

    e = entry_buffer;
    entry_data* ende = e + entry_buffer_size;
    for (; e != ende; ++e) {
        if (e->type == -1) {
            xASSERT_L(749, e->owner_size == 0);
            for (S32 i = 0; i < MAX_OWNERS; ++i)
                xASSERT_L(751, e->owner[i] == INVALID_INDEX);
        } else {
            xASSERT_L(755, (e->owner_size > 0) && (e->owner_size <= MAX_OWNERS));
            for (S32 i = 0; i < e->owner_size; ++i)
                xASSERT_L(757, e->owner[i] != INVALID_INDEX);
            for (S32 i = e->owner_size; i < MAX_OWNERS; ++i)
                xASSERT_L(759, e->owner[i] == INVALID_INDEX);
        }
    }
}

bool xSpacePartitionTree2D::any_layer_empty(const cell* c) const
{
    if (c->children == INVALID_INDEX)
        return false;

    S32 total = 0;
    const cell* cc = cell_buffer + c->children, *end_cc = cc + 4;
    for (; cc != end_cc; ++cc) {
        if (any_layer_empty(cc))
            return true;
        if (cc->entries != INVALID_INDEX || cc->children != INVALID_INDEX)
            ++total;
    }
    return total <= 0;
}

void xSpacePartitionTree2D::debug_validate_cell(const cell* c) const
{
    cell* const cell_list = cell_buffer;
    entry_node* const node_list = node_buffer;
    entry_data* const entry_list = entry_buffer;

    xASSERT_L(788, c->children != 0);
    xASSERT_L(789, c >= cell_list);
    xASSERT_L(790, c < (cell_list + cell_buffer_size));

    U16 key = c - cell_list;
    for (U16 i = c->entries; i != INVALID_INDEX;) {
        entry_node* const n = node_list + i;
        xASSERT_L(797, n->entry < entry_buffer_size);
        entry_data* const e = entry_list + n->entry;

        xASSERT_L(800, e->type != -1);
        xASSERT_L(801, (e->owner_size > 0) && (e->owner_size <= MAX_OWNERS));
        for (S32 j = 0; j < e->owner_size; ++j)
            xASSERT_L(803, e->owner[j] != INVALID_INDEX);
        for (S32 j = e->owner_size; j < MAX_OWNERS; ++j)
            xASSERT_L(805, e->owner[j] == INVALID_INDEX);

        U16* owner = e->owner, *end_owner = owner + e->owner_size;
        for (; owner != end_owner; ++owner) {
            if (*owner == key) break;
        }
        xASSERT_L(812, owner != end_owner);
        i = n->next;
    }

    xASSERT_L(817, !any_layer_empty(c));
}

void xSpacePartitionTree2D::debug_intersect_cell(const cell* c) const
{
    static const xColor box_colors[8] = {
        { 0xFF, 0x00, 0x00, 0xFF },
        { 0x00, 0xFF, 0x00, 0xFF },
        { 0x00, 0x00, 0xFF, 0xFF },
        { 0x00, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0x00, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xA5, 0x00, 0xFF }
    };

    if (tweak::render_intersect && (tweak::render_empty || c->entries != INVALID_INDEX)) {
        F32 x, z, r;
        S32 depth = get_cell_bound(c, x, z, r);
        F32 y = xEntGetFrame(xglobals->players[0])->pos.y;
        xDrawSetColor(box_colors[depth]);
        xDrawBox(x, y - 0.5f * r, z, x + r, y + 0.5f * r, z + r);
    }
}

#endif
