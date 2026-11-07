#ifndef XDEBUGTWEAK_H
#define XDEBUGTWEAK_H

#include "xString.h"

#include <string.h>

struct tweak_info;
struct xVec3;

struct tweak_callback
{
    typedef void(*event_callback)(const tweak_info&);
    typedef void(*convert_callback)(const tweak_info&, void*);

    event_callback on_change;
    event_callback on_select;
    event_callback on_unselect;
    event_callback on_start_edit;
    event_callback on_stop_edit;
    event_callback on_expand;
    event_callback on_collapse;
    event_callback on_update;
    convert_callback convert_mem_to_tweak;
    convert_callback convert_tweak_to_mem;

    static tweak_callback create(event_callback on_change, event_callback on_select = NULL, event_callback on_unselect = NULL, event_callback on_start_edit = NULL, event_callback on_stop_edit = NULL, event_callback on_expand = NULL, event_callback on_collapse = NULL, event_callback on_update = NULL, convert_callback convert_mem_to_tweak = NULL, convert_callback convert_tweak_to_mem = NULL)
    {
        tweak_callback cb = { on_change, on_select, on_unselect, on_start_edit, on_stop_edit,
                              on_expand, on_collapse, on_update,
                              convert_mem_to_tweak, convert_tweak_to_mem };
        return cb;
    }

    static tweak_callback create_change(event_callback on_change)
    { tweak_callback cb = { on_change, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }; return cb; }

    static tweak_callback create_expand(event_callback on_expand, event_callback on_collapse)
    { tweak_callback cb = { NULL, NULL, NULL, NULL, NULL, on_expand, on_collapse, NULL, NULL, NULL }; return cb; }

    static tweak_callback create_update(event_callback on_update)
    { tweak_callback cb = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, on_update, NULL, NULL }; return cb; }
};

struct tweak_info
{
    substr name;
    void* value;
    const tweak_callback* cb;
    void* context;
    U8 type;
    U8 value_size;
    U16 flags;
    union
    {
        struct
        {
            S32 value_def;
            S32 value_min;
            S32 value_max;
        } int_context;
        struct
        {
            U32 value_def;
            U32 value_min;
            U32 value_max;
        } uint_context;
        struct
        {
            F32 value_def;
            F32 value_min;
            F32 value_max;
        } float_context;
        struct
        {
            bool value_def;
        } bool_context;
        struct
        {
            U32 value_def;
            U32 labels_size;
            const char** labels;
            void* values;
        } select_context;
        struct
        {
            U32 value_def;
            U32 mask;
        } flag_context;
        struct
        {
            U8 pad[16];
        } all_context;
    };
};

#if DEBUG || RELEASE
void* xDebugTweak_StaticAlloc(U32 size);
tweak_info* xDebugGetTweak(const char* name);
tweak_info* xDebugGetParentTweak(const tweak_info& info);

void xDebugAddTweak(const char* name, S32* v, S32 vmin, S32 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, S8* v, S8 vmin, S8 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, U32* v, U32 vmin, U32 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, U16* v, U16 vmin, U16 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, U8* v, U8 vmin, U8 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddSelectTweak(const char* name, S32* v, const char** labels, const S32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddSelectTweak(const char* name, S8* v, const char** labels, const S8* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddSelectTweak(const char* name, U32* v, const char** labels, const U32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddSelectTweak(const char* name, U8* v, const char** labels, const U8* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddFlagTweak(const char* name, U32* v, U32 mask, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddFlagTweak(const char* name, U8* v, U8 mask, const tweak_callback* cb, void* context, U32 flags);
U32 xDebugCountChildTweaks(const tweak_info& info);
void xDebugAddTweak(const char* name, xVec3* v, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddBranchTweak(const char* name, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, bool* v, const tweak_callback* cb, void* context, U32 flags);
void xDebugAddTweak(const char* name, const char* message, const tweak_callback* cb, void* context, U32 flags);

void _auto_tweak(const char* prefix, const char* name, S32* v, S32 vmin, S32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, U32* v, U32 vmin, U32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, U8* v, U8 vmin, U8 vmax, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, bool* v, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak(const char* prefix, const char* name, const char* message, const tweak_callback* cb, void* context, U32 flags, bool process);

void _auto_tweak_select(const char* prefix, const char* name, S32* v, const char** labels, const S32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak_select(const char* prefix, const char* name, U32* v, const char** labels, const U32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process);
void _auto_tweak_flag(const char* prefix, const char* name, U32* v, U32 mask, const tweak_callback* cb, void* context, U32 flags);

extern const tweak_callback cb_tweak_force_lt;
extern const tweak_callback cb_tweak_force_gt;
extern const tweak_callback cb_tweak_force_lteq;
extern const tweak_callback cb_tweak_force_gteq;
extern const tweak_callback cb_tweak_convert_degrees;

extern const char* tweak_blend_mode_labels[11];
extern const U32 tweak_blend_mode_values[11];

typedef bool(*xDebugRemoveTweaksCallback)(const tweak_info&, void*);

void xDebugRemoveTweak(const substr& name);
void xDebugRemoveTweaks(const substr& name, xDebugRemoveTweaksCallback cb, void* context);

inline void xDebugAddFlagTweak(const char* name, S32* v, S32 mask, const tweak_callback* cb, void* context, U32 flags)
{
    xDebugAddFlagTweak(name, (U32*)v, (U32)mask, cb, context, flags);
}

inline void xDebugRemoveTweak(const char* name)
{ xDebugRemoveTweak(substr::create(name, strlen(name))); }

inline void xDebugRemoveTweaks(const char* name, xDebugRemoveTweaksCallback cb, void* context)
{ xDebugRemoveTweaks(substr::create(name, strlen(name)), cb, context); }

inline void _auto_tweak_flag(const char* prefix, const char* name, S32* v, S32 mask, const tweak_callback* cb, void* context, U32 flags)
{ _auto_tweak_flag(prefix, name, (U32*)v, (U32)mask, cb, context, flags); }

template <class T, class T2>
inline void _t_auto_tweak(const char* prefix, const char* name, T* v, T2 vmin, T2 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{ _auto_tweak(prefix, name, v, vmin, vmax, cb, context, flags, process); }

template <class T, class T2>
inline void _t_auto_tweak_select(const char* prefix, const char* name, T* v, const char** labels, const T* values, T2 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process)
{ _auto_tweak_select(prefix, name, v, labels, values, labels_size, cb, context, flags, process); }

template <class T, class T2>
inline void _t_auto_tweak_flag(const char* prefix, const char* name, T* v, T2 mask, const tweak_callback* cb, void* context, U32 flags)
{ _auto_tweak_flag(prefix, name, v, mask, cb, context, flags); }

template <>
inline void _t_auto_tweak(const char* prefix, const char* name, bool* v, S32 vmin, S32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{ _auto_tweak(prefix, name, v, cb, context, flags, process); }

#define xTWEAK(name, v, vmin, vmax, cb, context, flags) xDebugAddTweak(name, v, vmin, vmax, cb, context, flags)
#define xTWEAK_SELECT(name, v, labels, values, labels_size, cb, context, flags) xDebugAddSelectTweak(name, v, labels, values, labels_size, cb, context, flags)
#define xTWEAK_FLAG(name, v, mask, cb, context, flags) xDebugAddFlagTweak(name, v, mask, cb, context, flags)
#define xTWEAK_BRANCH(name, cb, context, flags) xDebugAddBranchTweak(name, cb, context, flags)
#define xTWEAK_BOOL(name, v, cb, context, flags) xDebugAddTweak(name, v, cb, context, flags)
#define xTWEAK_MESSAGE(name, message, cb, context, flags) xDebugAddTweak(name, message, cb, context, flags)
#define xTWEAK_REMOVE(name) xDebugRemoveTweak(name)

#define xAUTOTWEAK(prefix, name, v, vmin, vmax, cb, context, flags, process) _t_auto_tweak(prefix, name, v, vmin, vmax, cb, context, flags, process)
#define xAUTOTWEAK_BOOL(prefix, name, v, cb, context, flags, process) _t_auto_tweak(prefix, name, v, 0, 0, cb, context, flags, process)
#define xAUTOTWEAK_SELECT(prefix, name, v, labels, values, labels_size, cb, context, flags, process) _t_auto_tweak_select(prefix, name, v, labels, values, labels_size, cb, context, flags, process)
#define xAUTOTWEAK_FLAG(prefix, name, v, mask, cb, context, flags) _t_auto_tweak_flag(prefix, name, v, mask, cb, context, flags)
#define xAUTOTWEAK_MESSAGE(prefix, name, message, cb, context, flags, process) _auto_tweak(prefix, name, message, cb, context, flags, process)

#define xTEMPTWEAKS_BEGIN(prefix_name)                                                            \
    static bool _temp_tweaks_registered = false;                                                  \
    if (!_temp_tweaks_registered) {                                                               \
        _temp_tweaks_registered = true;                                                           \
        const char* prefix_name = "Temp|";

#define xTEMPTWEAKS_END }

#define xTWEAKABLE
#else
inline void xDebugAddTweak(const char* name, const U32* v, U32 vmin, U32 vmax, const tweak_callback* cb, void* context, U32 flags) {}
inline void xDebugAddTweak(const char* name, const F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags) {}
inline void xDebugAddBranchTweak(const char* name, const tweak_callback* cb, void* context, U32 flags) {}

inline void xDebugRemoveTweak(const char* name) {}

#define xTWEAK(name, v, vmin, vmax, cb, context, flags) xDebugAddTweak(name, v, vmin, vmax, cb, context, flags)
#define xTWEAK_SELECT(name, v, labels, values, labels_size, cb, context, flags)
#define xTWEAK_FLAG(name, v, mask, cb, context, flags)
#define xTWEAK_BRANCH(name, cb, context, flags)
#define xTWEAK_BOOL(name, v, cb, context, flags)
#define xTWEAK_MESSAGE(name, message, cb, context, flags)
#define xTWEAK_REMOVE(name) xDebugRemoveTweak(name)

#define xAUTOTWEAK(prefix, name, v, vmin, vmax, cb, context, flags, process)
#define xAUTOTWEAK_BOOL(prefix, name, v, cb, context, flags, process)
#define xAUTOTWEAK_SELECT(prefix, name, v, labels, values, labels_size, cb, context, flags, process)
#define xAUTOTWEAK_FLAG(prefix, name, v, mask, cb, context, flags)
#define xAUTOTWEAK_MESSAGE(prefix, name, message, cb, context, flags, process)

#define xTWEAKABLE const
#endif

#endif
