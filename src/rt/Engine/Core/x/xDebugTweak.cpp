#include "xDebugTweak.h"

#include "containers.h"
#include "xDebug.h"
#include "xFile.h"
#include "xFont.h"
#include "xGlobals.h"
#include "iMath.h"
#include "iCamera.h"
#include "xIni.h"
#include "xMath.h"
#include "xMath2.h"
#include "xMemMgr.h"
#include "xString.h"
#include "xutil.h"
#include "zGame.h"
#include "zNPCAnimViewer.h"
#include "zPad.h"
#include "zRenderState.h"

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool gDebugTweakRendering;

static U8 HIMEM_DEBUG_TWEAK_BASE[0x400000];
static U8 HIMEM_MORE_TWEAK_BASE[0x200000];
static U8* xDebugTweak_StaticCurr = HIMEM_MORE_TWEAK_BASE;
static U8* xDebugTweak_StaticMax = HIMEM_MORE_TWEAK_BASE + sizeof(HIMEM_MORE_TWEAK_BASE);

void xDebugTweak_StaticReset()
{
    xDebugTweak_StaticCurr = HIMEM_MORE_TWEAK_BASE;
}

void* xDebugTweak_StaticAlloc(U32 size)
{
    size = (size + 15) & ~15;
    xASSERT_L(91, xDebugTweak_StaticCurr + size < xDebugTweak_StaticMax);

    void* result = xDebugTweak_StaticCurr;
    xDebugTweak_StaticCurr += size;
    memset(result, 0, size);

    return result;
}

namespace {
    struct tweak_callback_wrapper : tweak_callback
    {
#define WRAP_EVENT_L(name, first, second, line)                                                   \
        static void wrap_##name(const tweak_info& info)                                           \
        {                                                                                         \
            const tweak_callback_wrapper* cb_wrap = (const tweak_callback_wrapper*)info.cb;       \
            xASSERT_L(line, (cb_wrap->cb1 != 0) && (cb_wrap->cb2 != 0));                          \
            tweak_info ncinfo = info;                                                             \
            ncinfo.cb = cb_wrap->first;                                                           \
            cb_wrap->first->name(ncinfo);                                                         \
            ncinfo.cb = cb_wrap->second;                                                          \
            cb_wrap->second->name(ncinfo);                                                        \
        }
#define WRAP_EVENT(name, first, second) WRAP_EVENT_L(name, first, second, __LINE__)

        WRAP_EVENT_L(on_change, cb1, cb2, 117)
        WRAP_EVENT_L(on_select, cb1, cb2, 118)
        WRAP_EVENT_L(on_unselect, cb2, cb1, 119)
        WRAP_EVENT_L(on_start_edit, cb1, cb2, 120)
        WRAP_EVENT_L(on_stop_edit, cb2, cb1, 121)
        WRAP_EVENT_L(on_expand, cb1, cb2, 122)
        WRAP_EVENT_L(on_collapse, cb2, cb1, 123)
        WRAP_EVENT_L(on_update, cb1, cb2, 124)

#undef WRAP_EVENT
#undef WRAP_EVENT_L

        static void wrap_convert_mem_to_tweak(const tweak_info& info, void* value)
        {
            const tweak_callback_wrapper* cb_wrap = (const tweak_callback_wrapper*)info.cb;
            xASSERT_L(130, (cb_wrap->cb1 != 0) && (cb_wrap->cb2 != 0));
            tweak_info ncinfo = info;
            ncinfo.cb = cb_wrap->cb1;
            cb_wrap->cb1->convert_mem_to_tweak(ncinfo, value);
            ncinfo.cb = cb_wrap->cb2;
            cb_wrap->cb2->convert_mem_to_tweak(ncinfo, value);
        }

        static void wrap_convert_tweak_to_mem(const tweak_info& info, void* value)
        {
            const tweak_callback_wrapper* cb_wrap = (const tweak_callback_wrapper*)info.cb;
            xASSERT_L(140, (cb_wrap->cb1 != 0) && (cb_wrap->cb2 != 0));
            tweak_info ncinfo = info;
            ncinfo.cb = cb_wrap->cb2;
            cb_wrap->cb2->convert_tweak_to_mem(ncinfo, value);
            ncinfo.cb = cb_wrap->cb1;
            cb_wrap->cb1->convert_tweak_to_mem(ncinfo, value);
        }

        tweak_callback_wrapper(const tweak_callback& cb1, const tweak_callback& cb2)
        {
            this->cb1 = &cb1;
            this->cb2 = &cb2;
            create(wrap_on_change, wrap_on_select, wrap_on_unselect, wrap_on_start_edit, wrap_on_stop_edit,
                   wrap_on_expand, wrap_on_collapse, wrap_on_update,
                   wrap_convert_mem_to_tweak, wrap_convert_tweak_to_mem);
        }

        const tweak_callback* cb1;
        const tweak_callback* cb2;
    };
}

tweak_callback* xDebugTweakChainCallbacks(const tweak_callback& cb1, const tweak_callback& cb2)
{
    return new (eMemTweakType_0, 8) tweak_callback_wrapper(cb1, cb2);
}

namespace {
    const S16 special_char_lookup[256] =
    {
        -2,   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        ' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'', '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
        '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
        '@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
        'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '[',  '\\', ']',  '^',  '_',
        '`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
        'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '{',  -1,   '}',  '~',  0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
        0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
        0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
        0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
    };

    S32 simemcmp(const void* d1, const void* d2, size_t size)
    {
        const char* s1 = (const char*)d1, *end_s1 = s1 + size, *s2 = (const char*)d2;
        for (; s1 != end_s1; ++s1, ++s2) {
            S32 cmp = special_char_lookup[(U8)*s1] - special_char_lookup[(U8)*s2];
            if (cmp != 0) return cmp;
        }
        return 0;
    }

    S32 sistrcmp(const void* d1, const void* d2)
    {
        const char* s1 = (const char*)d1, *s2 = (const char*)d2;
        for (;; ++s1, ++s2) {
            char c1 = *s1, c2 = *s2;
            S32 cmp = special_char_lookup[(U8)c1] - special_char_lookup[(U8)c2];
            if (cmp != 0 || c1 == '\0')
                return cmp;
        }
    }

    S32 sicompare(const substr& s1, const substr& s2)
    {
        size_t len = (s1.size < s2.size) ? s1.size : s2.size;
        S32 c = simemcmp(s1.text, s2.text, len);
        if (c != 0) return c;
        return s1.size - s2.size;
    }
}

namespace {
    F32 tweak_move_delay = 0.3f;
    F32 tweak_move_speed = 20.0f;
    F32 tweak_move_super_speed = 100.0f;
    const F32 tweak_box_value_font_width = 0.75f * (18.0f / FB_XRES);
    const F32 tweak_box_value_font_height = 0.75f * (22.0f / FB_YRES);
    const xColor tweak_box_help_color = { 150, 150, 200, 255 };
    const xColor tweak_box_name_color[8] =
    {
        { 70, 255, 70, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
        { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }
    };
    const xColor tweak_box_value_normal_color[8] =
    {
        { 255, 70, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
        { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 100, 100, 255, 255 }, { 255, 255, 255, 255 }
    };
    const xColor tweak_box_value_edit_color[8] =
    {
        { 220, 50, 50, 255 }, { 220, 220, 50, 255 }, { 220, 220, 50, 255 }, { 220, 220, 50, 255 },
        { 220, 220, 50, 255 }, { 220, 220, 50, 255 }, { 220, 50, 50, 255 }, { 220, 220, 50, 255 }
    };
    const xColor tweak_box_value_press_color[8] =
    {
        { 220, 50, 50, 255 }, { 220, 50, 50, 255 }, { 220, 50, 50, 255 }, { 220, 50, 50, 255 },
        { 220, 50, 50, 255 }, { 220, 50, 50, 255 }, { 220, 50, 50, 255 }, { 220, 50, 50, 255 }
    };
    const xColor tweak_box_value_read_only_color[8] =
    {
        { 150, 150, 150, 255 }, { 150, 150, 150, 255 }, { 150, 150, 150, 255 }, { 150, 150, 150, 255 },
        { 150, 150, 150, 255 }, { 150, 150, 150, 255 }, { 150, 150, 150, 255 }, { 150, 150, 150, 255 }
    };
    const char help_text_short[] =
        "{i:text_button_small_02} - Show help  {i:text_button_small_01} - Edit  {i:text_button_small_03} - Reset value";
    const char help_text_long_browse[] =
        "{i:text_button_small_02} - Hide help  {i:text_button_small_01} - Edit  {i:text_button_small_03} - Reset value\n"
        "{i:text_button_small_14} - Close branch  {i:text_button_small_16} - Open branch\n"
        "{i:text_button_small_15} - Move up  {i:text_button_small_13} - Move down";
    const char help_text_long_edit[] =
        "{i:text_button_small_02} - Hide help  {i:text_button_small_01} - Commit  {i:text_button_small_04} - Cancel  {i:text_button_small_03} - Reset value\n"
        "{i:text_button_small_14} - Move focus left  {i:text_button_small_16} - Move focus right\n"
        "{i:text_button_small_15} - Increment value  {i:text_button_small_13} - Decrement value";
    const char* tweak_dump_files[] =
    {
        "tweaks0.ini",
        "tweaks1.ini",
        "tweaks2.ini",
        "tweaks3.ini",
        "tweaks4.ini",
        "tweaks5.ini",
        "tweaks6.ini",
        "tweaks7.ini",
        "tweaks8.ini",
        "tweaks9.ini",
        "autotweaks.ini"
    };
    const char whitespace_buffer[] = "                                                                                                    ";
    bool debug_tweak_inited;

    enum
    {
        DT_BRANCH,
        DT_INT,
        DT_UINT,
        DT_FLOAT,
        DT_BOOL,
        DT_SELECT,
        DT_MESSAGE,
        DT_FLAG
    };

    enum
    {
        DTF_READ_ONLY = 0x1,
        DTF_BUFFER = 0x2,
        DTF_NO_LOAD = 0x4,
        DTF_NO_SCALE_NAME = 0x8,
        DTF_NO_SCALE_VALUE = 0x10,
        DTF_CHECKED = 0x20,
        DTF_EXPANDED = 0x100,
        DTF_ERASED = 0x200,
        DTF_SELECTED = 0x400,
        DTF_CHANGED = 0x800
    };

    namespace debug_tweak {
        struct value_type : tweak_info
        {
            S16 edit_digit;
            U8 change_index;
            U8 pad;
            U16 parent;
            U16 next;
        };

        struct stripped_value_type
        {
            char name[128];
            value_type* value;
        };

        struct string_type
        {
            char text[128];
        };

        enum filter_mode
        {
            FILTER_ALL,
            FILTER_OPEN,
            FILTER_CHANGED
        };

        enum
        {
            VALUE_MEM = (sizeof(HIMEM_DEBUG_TWEAK_BASE) * 4 / 7 + 3) & ~3,
            STRING_MEM = sizeof(HIMEM_DEBUG_TWEAK_BASE) - VALUE_MEM,
            MAX_VALUES = VALUE_MEM / (sizeof(value_type) + sizeof(stripped_value_type)),
            MAX_STRINGS = 13216,
            MAX_INSERTS = 64,
            MAX_NAME = 128,
            INVALID_INDEX = 0xFFFF
        };

        typedef fixed_stack_list<string_type, MAX_STRINGS> string_list;
        typedef fixed_stack_list<tweak_info, MAX_INSERTS> insert_list;

        value_type* values = (value_type*)HIMEM_DEBUG_TWEAK_BASE;
        stripped_value_type* stripped_values = (stripped_value_type*)(HIMEM_DEBUG_TWEAK_BASE + MAX_VALUES * sizeof(value_type));
        string_list* strings = (string_list*)(HIMEM_DEBUG_TWEAK_BASE + VALUE_MEM);
        insert_list inserts;
        U32 values_size;
        U32 erase_size;
        U32 first_visible;
        U32 selected;
        xtextbox value_box;
        xtextbox help_box;
        bool whole_screen;
        bool moving;
        F32 move_time;
        bool editting;
        bool expand_help;
        xColor background_color;
        char dump_file[32];
        U32 dump_file_index;
        F32 font_size_multiplier;
        bool need_relink;
        bool need_sort_stripped;
        U32 old_uint_value;
        U16 changed_values[256];
        U32 changed_values_size;
        bool need_sort_changed;
        struct
        {
            bool any;
            bool selected;
            bool opened;
        } automenu_enable;
        bool warned_about_memory;

        void reset_help_box();
        void reset_value_box();
        void refresh_dump_file();
        void flush_and_relink();
        tweak_info& get(const substr& name, bool& exists, U16 flags);
        void erase(const substr& name, xDebugRemoveTweaksCallback cb, void* context);
        void free_buffer(tweak_info& v);
        void alloc_buffer(tweak_info& v);
        void add_test_tweaks();
        void relink();
        bool find(const substr& name, size_t& at);
        U16 end_child(U16 i);
        bool move_down();
        bool move_up();
        substr parent_name(const substr& name);
        void flush_erases();
        void refresh_help_box();
        void refresh_visible();
        void render_values();
        U16 begin_child(U16 i);
        void set_selected(size_t sel);
        bool reset_value(value_type& v, bool recurse);
        void store_value(value_type& v);
        void restore_value(value_type& v);
        void modify_value(value_type& v, S32 scale);
        void clip_edit_digit(value_type& v);
        void get_value(const value_type& v, U32& val);
        void set_value(const value_type& v, U32 val);
        void on_change(value_type& v);
        void on_start_edit(const value_type& v);
        void on_stop_edit(const value_type& v);
        void on_expand(const value_type& v);
        void on_collapse(const value_type& v);
        void convert_mem_to_tweak(const value_type& v, void* value);
        void convert_tweak_to_mem(const value_type& v, void* value);
        S32 timed_move(F32 dt);
        void expand_all(size_t at);
        U16 get_index(const value_type& v);
        void load_tweaks();
        void automenu_load();
        void save_tweaks(filter_mode filter);
        void automenu_save();
        void flush();
        void select(const char* name);
        size_t find_lowest(const char* name);
        void validate_stuff();
        void refresh_changed();
        void add_parent(const substr& name, U16 flags);
        F32 set_box_name(const value_type& v, F32 y);
        F32 set_box_value(const value_type& v, F32 y);
        U16 get_next_visible(U16 i);
        U16 get_prev_visible(U16 i);
        void on_select(const value_type& v);
        void on_unselect(const value_type& v);
        substr child_name(const substr& name);
        size_t depth(const value_type& v);
        void write_edit_value(char* buffer, const value_type& v, S32 digit);
        void write_value(char* buffer, const value_type& v, S32 width);
        void on_update(const value_type& v);
        U32 get_select_index(const value_type& v);
        U32 get_select_value(const value_type& v, U32 index);
        void strip_ugly(char* text);
        void mark_changed(value_type& v);
        void write_all_tweaks(xFile& file, filter_mode filter);
        void write_changed_tweaks(xFile& file, filter_mode filter);
        void collapse_all(size_t at);
        void on_change_file_index(const tweak_info&);
        void on_change_save(const tweak_info&);
        void on_change_save_changed(const tweak_info&);
        void on_change_save_opened(const tweak_info&);
        void on_change_load(const tweak_info&);
        void on_change_expand_all(const tweak_info&);
        void on_change_collapse_all(const tweak_info&);

        inline void autosave_patch() {}

        void reset()
        {
            warned_about_memory = false;

            {
                S32 max_values = MAX_VALUES;
                S32 max_strings = MAX_STRINGS;
                S32 max_values_size = MAX_VALUES * sizeof(value_type);
                S32 max_strings_size = strings->max_size() * sizeof(string_list::node_type);
                S32 diff = (U8*)strings - (U8*)values;
                S32 valmem = VALUE_MEM;
                S32 strmem = STRING_MEM;
                S32 asdf = 123;
            }

            need_relink = false;
            need_sort_stripped = false;
            need_sort_changed = false;
            debug_tweak_inited = true;
            values_size = erase_size = 0;
            first_visible = 0;
            selected = INVALID_INDEX;
            changed_values_size = 0;
            automenu_enable.any = automenu_enable.selected = automenu_enable.opened = true;
            inserts.reset();
            strings->reset();
            reset_help_box();
            reset_value_box();

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|\001Info|Tweaks Used", &values_size, 0, 0, NULL, NULL, 0);
            static S32 max_values = MAX_VALUES;
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|\001Info|Tweaks Max", &max_values, 0, 0, NULL, NULL, 0);

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|\001Info|Buffers Used", (U32*)strings, 0, 0, NULL, NULL, 0);
            static S32 max_strings = MAX_STRINGS;
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|\001Info|Buffers Max", &max_strings, 0, 0, NULL, NULL, 0);

            xColorInit(background_color, 0, 0, 0, 120);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Background Alpha", &background_color.a, 0, 255, NULL, NULL, 0);

            dump_file_index = 0;
            refresh_dump_file();

            static const tweak_callback file_index_cb = { on_change_file_index };
            xDebugAddSelectTweak("\001{c=ffffff00}Tweak Box|Save/Load File", &dump_file_index, tweak_dump_files, NULL,
                                 ARRAY_LENGTH(tweak_dump_files) - 1, &file_index_cb, NULL, 0);

            static const tweak_callback save_cb = { on_change_save };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Save To", dump_file, &save_cb, NULL, 0);

            static const tweak_callback save_changed_cb = { on_change_save_changed };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Save Changed To", dump_file, &save_changed_cb, NULL, 0);

            static const tweak_callback save_opened_cb = { on_change_save_opened };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Save Opened To", dump_file, &save_opened_cb, NULL, 0);

            static const tweak_callback load_cb = { on_change_load };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Load From", dump_file, &load_cb, NULL, 0);

            font_size_multiplier = 1.0f;
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Font Size", &font_size_multiplier, 0.1f, 4.0f, NULL, NULL, 0);

            static const tweak_callback expand_all_cb = { on_change_expand_all };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|\002Expand All", "Expand!", &expand_all_cb, NULL, 0);

            static const tweak_callback collapse_all_cb = { on_change_collapse_all };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|\002Collapse All", "Collapse!", &collapse_all_cb, NULL, 0);

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|AutoMenu|Enabled", &automenu_enable.any, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|AutoMenu|Save Selected", &automenu_enable.selected, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|AutoMenu|Save Opened", &automenu_enable.opened, NULL, NULL, 0);

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Move Delay", &tweak_move_delay, 0.0f, 1.0f, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Move Speed", &tweak_move_speed, 1.0f, 1000.0f, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|Move Super Speed", &tweak_move_super_speed, 1.0f, 1000.0f, NULL, NULL, 0);

            add_test_tweaks();
        }

        size_t size()
        {
            return values_size + inserts.size() - erase_size;
        }

        bool full()
        {
            return size() + 8 >= MAX_VALUES || strings->size() + 8 >= strings->max_size();
        }

        bool check_insert(const substr&)
        {
            if (!full()) return true;
            flush_and_relink();
            if (!full()) return true;

            if (gSoak)
                warned_about_memory = true;
            if (warned_about_memory) return false;
            warned_about_memory = true;

            return false;
        }

        void insert(const tweak_info& t)
        {
            bool exists;

            if (!check_insert(t.name)) {
                return;
            }

            tweak_info& nt = get(t.name, exists, t.flags & 0xFF);

            nt.value = t.value;
            nt.cb = t.cb;
            nt.context = t.context;
            nt.type = t.type;
            nt.value_size = t.value_size;
            nt.all_context = t.all_context;

            if ((nt.flags & DTF_BUFFER) && !(t.flags & DTF_BUFFER))
                free_buffer(nt);
            else if (!(nt.flags & DTF_BUFFER) && (t.flags & DTF_BUFFER))
                alloc_buffer(nt);

            nt.flags = (nt.flags & 0xFF00) | (t.flags & ~0xFF00);

            if (exists && nt.type != t.type && t.type == DT_BRANCH) {
                char buffer[128];
                memcpy(buffer, t.name.text, t.name.size);
                buffer[t.name.size] = '|';
                buffer[t.name.size + 1] = '\0';
                substr childstr = { buffer, t.name.size + 1 };
                flush_and_relink();
                erase(childstr, NULL, NULL);
            }
        }

        void erase(const substr& name)
        {
            size_t at;

            if (need_relink) relink();
            if (find(name, at)) {
                size_t end = end_child(at);
                for (size_t i = at; i < end; i++) {
                    if (values[i].flags & DTF_ERASED) continue;
                    if (i == selected && !move_down() && !move_up())
                        selected = INVALID_INDEX;
                    values[i].flags |= DTF_ERASED;
                    erase_size++;
                }
            }

            for (insert_list::iterator it = inserts.begin(); it != inserts.end();) {
                bool found = false;
                substr pname = it->name;
                for (; pname.size; pname = parent_name(pname))
                    if (sicompare(name, pname) == 0) {
                        if (it->flags & DTF_BUFFER) free_buffer(*it);
                        it = inserts.erase(it);
                        found = true;
                        break;
                    }
                if (!found) ++it;
            }

            if (erase_size > MAX_INSERTS) flush_erases();
        }

        void erase(const substr& name, xDebugRemoveTweaksCallback cb, void* context)
        {
            flush_and_relink();

            size_t at;
            bool found = find(name, at);
            if (!found) {
                if (at >= values_size) return;
                if (simemcmp(name.text, values[at].name.text, name.size) != 0) {
                    return;
                }
            }

            for (size_t i = at; i < values_size; i++) {
                if (simemcmp(name.text, values[i].name.text, name.size) != 0) {
                    break;
                }

                if (values[i].parent != INVALID_INDEX && !(values[values[i].parent].flags & DTF_ERASED) && cb &&
                    !cb(values[i], context)) continue;

                if (i == selected && !move_down() && !move_up()) {
                    selected = INVALID_INDEX;
                }
                values[i].flags |= DTF_ERASED;
                erase_size++;
            }
        }

        void render()
        {
            flush_and_relink();
            refresh_help_box();

            help_box.render(false);
            refresh_visible();
            render_values();
        }

        void update(F32 dt)
        {
            flush_and_relink();
            if (!values_size) return;
            bool edit_pad = zPad::GetDebugPad()->Pressed(0x10000);
            value_type& sv = values[selected];

            if (zPad::GetDebugPad()->Pressed(0x80000)) {
                expand_help = !expand_help;
            }

            if (!editting) {
                if (edit_pad) {
                    switch (sv.type) {
                    case DT_BRANCH:
                        if (zPad::GetDebugPad()->On(0x200)) {
                            if (sv.flags & DTF_EXPANDED)
                                collapse_all((U16)selected);
                            else
                                expand_all((U16)selected);
                        } else {
                            sv.flags ^= DTF_EXPANDED;
                            if (sv.flags & DTF_EXPANDED)
                                sv.flags |= (U16)DTF_CHECKED;
                            else
                                sv.flags &= (U16)~DTF_CHECKED;
                            on_change(sv);
                            if (sv.flags & DTF_EXPANDED) on_expand(sv);
                            else on_collapse(sv);
                        }
                        break;
                    case DT_BOOL:
                        if (sv.flags & DTF_READ_ONLY) break;
                        {
                            bool val = *(bool*)sv.value;
                            convert_mem_to_tweak(sv, &val);
                            val = !val;
                            if (val) sv.flags |= (U16)DTF_CHECKED;
                            else sv.flags &= (U16)~DTF_CHECKED;
                            convert_tweak_to_mem(sv, &val);
                            *(bool*)sv.value = val;
                            on_change(sv);
                            autosave_patch();
                        }
                        break;
                    case DT_FLAG:
                        if (sv.flags & DTF_READ_ONLY) break;
                        {
                            U32 val;
                            get_value(sv, val);
                            if ((val & sv.flag_context.mask) == sv.flag_context.mask) {
                                val &= ~sv.flag_context.mask;
                                sv.flags &= (U16)~DTF_CHECKED;
                            } else {
                                val |= sv.flag_context.mask;
                                sv.flags |= (U16)DTF_CHECKED;
                            }
                            set_value(sv, val);
                            on_change(sv);
                            autosave_patch();
                        }
                        break;
                    case DT_MESSAGE:
                        on_change(sv);
                        break;
                    default:
                        if (sv.flags & DTF_READ_ONLY) break;
                        editting = true;
                        moving = false;
                        edit_pad = false;
                        store_value(sv);
                        on_start_edit(sv);
                        break;
                    }
                }
                else if (zPad::GetDebugPad()->Pressed(0x40000)) {
                    if (reset_value(sv, true))
                        autosave_patch();
                }
                else if (zPad::GetDebugPad()->On(0x40)) {
                    S32 inc = timed_move(dt);
                    for (S32 i = 0; i < inc; i++)
                        move_down();
                }
                else if (zPad::GetDebugPad()->On(0x10)) {
                    S32 inc = timed_move(dt);
                    for (S32 i = 0; i < inc; i++)
                        move_up();
                }
                else moving = false;

                if (zPad::GetDebugPad()->Pressed(0x20)) {
                    if (sv.type == DT_BRANCH) {
                        if (zPad::GetDebugPad()->On(0x200)) {
                            expand_all((U16)selected);
                        }
                        else if (sv.flags & DTF_EXPANDED) {
                            U16 child = begin_child(selected);
                            if (child != INVALID_INDEX) set_selected(child);
                        } else {
                            sv.flags |= (U16)(DTF_EXPANDED | DTF_CHECKED);
                            on_change(sv);
                            on_expand(sv);
                        }
                    }
                }
                else if (zPad::GetDebugPad()->Pressed(0x80)) {
                    if (zPad::GetDebugPad()->On(0x200) && sv.type == DT_BRANCH) {
                        collapse_all((U16)selected);
                    }
                    else if (sv.type == DT_BRANCH && (sv.flags & DTF_EXPANDED)) {
                        sv.flags &= (U16)~(DTF_EXPANDED | DTF_CHECKED);
                        on_change(sv);
                        on_collapse(sv);
                    }
                    else if (sv.parent != INVALID_INDEX) {
                        set_selected(sv.parent);
                    }
                }
            }

            if (editting) {
                if (edit_pad) {
                    editting = false;
                    on_stop_edit(sv);
                    autosave_patch();
                }
                else if (zPad::GetDebugPad()->Pressed(0x20000)) {
                    restore_value(sv);
                    editting = false;
                    on_stop_edit(sv);
                }
                else if (zPad::GetDebugPad()->Pressed(0x40000)) {
                    bool changed = reset_value(sv, true);
                    editting = false;
                    on_stop_edit(sv);
                    if (changed) autosave_patch();
                }
                else if (zPad::GetDebugPad()->On(0x40)) {
                    S32 inc = timed_move(dt);
                    modify_value(sv, -inc);
                }
                else if (zPad::GetDebugPad()->On(0x10)) {
                    S32 inc = timed_move(dt);
                    modify_value(sv, inc);
                }
                else if (zPad::GetDebugPad()->On(0x80)) {
                    S32 inc = timed_move(dt);
                    sv.edit_digit += (S16)inc;
                    clip_edit_digit(sv);
                }
                else if (zPad::GetDebugPad()->On(0x20)) {
                    S32 inc = timed_move(dt);
                    sv.edit_digit -= (S16)inc;
                    clip_edit_digit(sv);
                }
                else moving = false;
            }
        }

        tweak_info* find(const substr& name)
        {
            size_t at;
            if (find(name, at)) {
                if (values[at].flags & DTF_ERASED) return NULL;
                return &values[at];
            }

            for (insert_list::iterator it = inserts.begin(); it != inserts.end(); ++it) {
                if (sicompare(it->name, name) == 0)
                    return &*it;
            }

            return NULL;
        }

        void expand(size_t at)
        {
            for (; at != INVALID_INDEX; at = values[at].parent) {
                value_type& v = values[at];
                if (!(v.flags & DTF_EXPANDED) && v.type == DT_BRANCH) {
                    v.flags |= DTF_EXPANDED;
                    on_expand(v);
                }
            }
        }

        void expand(const char* name)
        {
            flush_and_relink();
            size_t at;
            if (find(substr::create(name, strlen(name)), at))
                expand(at);
        }

        void collapse(size_t at)
        {
            value_type& v = values[at];
            if ((v.flags & DTF_EXPANDED) && v.type == DT_BRANCH) {
                v.flags &= (U16)~DTF_EXPANDED;
                on_collapse(v);
            }
        }

        void collapse(const char* name); // UNUSED

        void select(const char* name)
        {
            flush_and_relink();
            size_t at;
            if (!find(substr::create(name, strlen(name)), at)) return;
            expand(values[at].parent);
            set_selected(at);
            editting = false;
        }

        value_type* get_selected()
        {
            if (selected == INVALID_INDEX) return NULL;
            return &values[selected];
        }

        U8 expanded(const char* name); // UNUSED

        tweak_info* get_parent(const tweak_info& info)
        {
            flush_and_relink();
            const value_type& v = (const value_type&)info;
            U16 i = get_index(v);
            if (i == INVALID_INDEX) return NULL;
            if (v.parent == INVALID_INDEX) return NULL;
            return &values[v.parent];
        }

        tweak_info* get_first_child(const tweak_info& info); // UNUSED
        tweak_info* get_next_sibling(const tweak_info& info); // UNUSED

        size_t count_children(const tweak_info& info)
        {
            flush_and_relink();
            const value_type& v = (const value_type&)info;
            U16 i = get_index(v);
            if (i == INVALID_INDEX) return 0;
            size_t total = 0;
            for (U16 it = begin_child(i); it < values_size; it = values[it].next)
                total++;
            return total;
        }

        void get_value(const tweak_info& v, U32& val, const void* in)
        {
            if (!in) in = v.value;
            switch (v.value_size) {
            case 1: val = *(U8*)in; break;
            case 2: val = *(U16*)in; break;
            case 4: val = *(U32*)in; break;
            default: xASSERT_L(1252, false);
            }
        }

        void set_value(const tweak_info& v, U32 val, void* out)
        {
            if (!out) out = v.value;
            switch (v.value_size) {
            case 1: *(U8*)out = val; break;
            case 2: *(U16*)out = val; break;
            case 4: *(U32*)out = val; break;
            default: xASSERT_L(1264, false);
            }
        }

        void get_value(const tweak_info& v, S32& val, const void* in)
        {
            if (!in) in = v.value;
            switch (v.value_size) {
            case 1: val = *(S8*)in; break;
            case 2: val = *(S16*)in; break;
            case 4: val = *(S32*)in; break;
            default: xASSERT_L(1276, false);
            }
        }

        void set_value(const tweak_info& v, S32 val, void* out)
        {
            if (!out) out = v.value;
            switch (v.value_size) {
            case 1: *(S8*)out = val; break;
            case 2: *(S16*)out = val; break;
            case 4: *(S32*)out = val; break;
            default: xASSERT_L(1288, false);
            }
        }

        void get_value(const tweak_info& v, F32& val, const void* in)
        {
            if (!in) in = v.value;
            val = *(F32*)in;
        }

        void set_value(const tweak_info& v, F32 val, void* out)
        {
            if (!out) out = v.value;
            *(F32*)out = val;
        }

        void autoload()
        {
            U32 old_dump_file_index = dump_file_index;
            dump_file_index = ARRAY_LENGTH(tweak_dump_files) - 1;
            refresh_dump_file();
            load_tweaks();
            automenu_load();
            dump_file_index = old_dump_file_index;
            refresh_dump_file();
        }

        void autosave()
        {
            U32 old_dump_file_index = dump_file_index;
            dump_file_index = ARRAY_LENGTH(tweak_dump_files) - 1;
            refresh_dump_file();
            save_tweaks(FILTER_CHANGED);
            automenu_save();
            dump_file_index = old_dump_file_index;
            refresh_dump_file();
        }

        F32 box_top() { return whole_screen ? 0.02f : 0.175f; }

        bool find(const substr& name, size_t& at)
        {
            size_t lower = 0, upper = values_size;
            while ((S32)lower < (S32)upper) {
                size_t i = (lower + upper) / 2;
                S32 cmp = sicompare(name, values[i].name);

                if (cmp < 0) upper = i;
                else if (cmp > 0) lower = i + 1;
                else { at = i; return true; }
            }
            at = lower;
            return false;
        }

        size_t find_lowest(const char* name)
        {
            substr ss = { name, strlen(name) };
            while (ss.size) {
                size_t at;
                if (find(ss, at))
                    return at;
                ss = parent_name(ss);
            }
            return INVALID_INDEX;
        }

        void free_buffer(tweak_info& v)
        {
            xASSERT_L(1365, v.flags & DTF_BUFFER);
            string_list::iterator it = strings->get_iterator((string_type*)v.name.text);
            strings->erase(it);
            v.flags &= (U16)~DTF_BUFFER;
        }

        void alloc_buffer(tweak_info& v)
        {
            xASSERT_L(1373, !(v.flags & DTF_BUFFER));
            xASSERT_L(1374, v.name.size < MAX_NAME);
            strings->push_back();
            char* buffer = strings->back().text;
            memcpy(buffer, v.name.text, v.name.size);
            buffer[v.name.size] = '\0';
            v.name.text = buffer;
            v.flags |= (U16)DTF_BUFFER;
        }

        bool values_dirty;

        void flush_inserts()
        {
            if (inserts.empty()) return;

            need_relink = true;
            need_sort_stripped = true;
            values_dirty = true;
            S32 src = values_size - 1;
            values_size += inserts.size();
            xASSERT_L(1393, values_size <= MAX_VALUES);
            S32 dst = values_size - 1;
            insert_list::iterator it = inserts.begin();
            while (src < dst) {
                S32 cmp = (src < 0) ? -1 : sicompare(values[src].name, it->name);
                if (cmp < 0) {
                    if (selected != INVALID_INDEX && (S32)selected > src) selected++;
                    (tweak_info&)values[dst] = *it;
                    values[dst].edit_digit = 0;
                    dst--; ++it;
                    if (it == inserts.end()) break;
                }
                else if (cmp == 0) {
                    ++it;
                    if (it == inserts.end()) break;
                } else {
                    if (src != dst) values[dst] = values[src];
                    dst--; src--;
                }
            }
            inserts.clear();
        }

        void flush_erases()
        {
            if (!erase_size) return;
            values_dirty = true;
            need_relink = true;
            need_sort_stripped = true;
            for (size_t src = 0, dst = 0; src < values_size; src++) {
                value_type& sv = values[src];
                value_type& dv = values[dst];
                if (sv.flags & DTF_ERASED) {
                    if (selected != INVALID_INDEX && selected > dst) selected--;
                    if (sv.flags & DTF_BUFFER) free_buffer(sv);
                    continue;
                }
                if (src != dst) dv = sv;
                dst++;
            }
            values_size -= erase_size;
            erase_size = 0;
        }

        void flush_and_relink()
        {
            flush();
            if (!need_relink) return;
            relink();
            validate_stuff();
            if (selected == INVALID_INDEX && values_size)
                set_selected(0);
        }

        void flush()
        {
            flush_erases();
            flush_inserts();
        }

        substr parent_name(const substr& name)
        {
            for (S32 i = name.size - 1; i >= 0; i--) {
                if (name.text[i] == '|')
                    return substr::create(name.text, i);
            }
            return substr::create(NULL, 0);
        }

        substr child_name(const substr& name)
        {
            for (S32 i = name.size - 1; i >= 0; i--) {
                if (name.text[i] == '|')
                    return substr::create(name.text + i + 1, name.size - i - 1);
            }
            return name;
        }

        void add_parent(const substr& name, U16 flags)
        {
            substr pn = parent_name(name);
            if (!pn.size) return;
            bool exists;
            flags &= (U16)~DTF_READ_ONLY;
            tweak_info& t = get(pn, exists, flags);
            if (exists) return;
            t.value = NULL;
            t.cb = NULL;
            t.context = NULL;
            t.type = DT_BRANCH;
            t.value_size = 0;
        }

        tweak_info& get(const substr& name, bool& exists, U16 flags)
        {
            size_t at;

            if (find(name, at)) {
                value_type& v = values[at];
                if (v.flags & DTF_ERASED) {
                    v.flags &= (U16)~DTF_ERASED;
                    xASSERT_L(1505, erase_size > 0);
                    erase_size--;
                    add_parent(name, flags);
                }
                exists = true;
                return values[at];
            }

            for (insert_list::iterator it = inserts.begin(); it != inserts.end(); ++it)
                if (sicompare(name, it->name) == 0) {
                    exists = true; return *it; }

            exists = false;

            add_parent(name, flags);

            if (inserts.size() >= inserts.max_size()) flush();
            insert_list::iterator it = inserts.begin();
            for (; it != inserts.end(); ++it)
                if (sicompare(name, it->name) > 0) break;
            it = inserts.insert(it);
            it->flags = flags & ~DTF_BUFFER;
            it->name = name;
            if (flags & DTF_BUFFER) alloc_buffer(*it);

            return *it;
        }

        void relink()
        {
            need_relink = false;
            substr pname;
            for (U16 i = 0; i < (U16)values_size; i++) {
                value_type& v = values[i];
                v.parent = v.next = INVALID_INDEX;
                if (!i) continue;
                pname = parent_name(v.name);
                U16 parent = i - 1, prev = INVALID_INDEX;
                while (parent != INVALID_INDEX) {
                    if (sicompare(pname, values[parent].name) == 0) break;
                    prev = parent; parent = values[parent].parent;
                }
                v.parent = parent;
                if (prev != INVALID_INDEX) values[prev].next = i;

                if (v.parent == INVALID_INDEX) {
                    U16 parent2 = i - 1, prev2;
                    while (parent2 != INVALID_INDEX) {
                        if (sicompare(pname, values[parent2].name) == 0) break;
                        prev2 = parent2; parent2 = values[prev2].parent;
                    }
                }
            }
            refresh_changed();
        }

        // UNUSED: speculative
        void render_backgrounds()
        {
            basic_rect<F32> r = help_box.bounds;
            r.expand(0.005f);
            render_fill_rect(r, background_color);

            F32 h = 0.01f + help_box.font.clip.h;
            r.x = 0.06f;
            r.y = h + box_top();
            r.w = 0.92f;
            r.h = 0.98f - r.y;
            r.expand(0.005f);
            render_fill_rect(r, background_color);
        }

        void reset_help_box()
        {
            help_box = xtextbox::create();
            help_box.font.id = 1;
            help_box.font.width = 18.0f / 640.0f;
            help_box.font.height = 22.0f / 448.0f;
            help_box.flags = 0x2;
            help_box.font.color = tweak_box_help_color;
            help_box.bounds = help_box.font.clip.set_bounds(0.11f, box_top(), 0.93f, 0.2f);
        }

        void reset_value_box()
        {
            value_box = xtextbox::create();
            value_box.font.id = 1;
            value_box.flags = 0x20;
        }

        void refresh_help_box()
        {
            const char* help_text;
            if (!expand_help) help_text = help_text_short;
            else if (!editting) help_text = help_text_long_browse;
            else help_text = help_text_long_edit;
            help_box.bounds.y = help_box.font.clip.y = box_top();
            help_box.bounds.h = help_box.font.clip.h = 10.0f;
            help_box.set_text(help_text);
            help_box.bounds.h = help_box.font.clip.h = help_box.yextent(false);
        }

        void render_values()
        {
            F32 h = 0.01f + help_box.font.clip.h;
            F32 y = h + box_top();

            for (size_t i = first_visible; i < values_size;) {
                const value_type& v = values[i];
                F32 name_height = set_box_name(v, y);
                value_box.render(false);
                F32 value_height = set_box_value(v, y);
                value_box.render(false);
                y += MAX(name_height, value_height);
                if (y >= 0.98f) break;
                i = get_next_visible(i);
            }
        }

        U16 get_index(const value_type& v)
        {
            S32 diff = (U8*)&v - (U8*)values;
            if (diff % sizeof(value_type)) return INVALID_INDEX;
            diff /= sizeof(value_type);
            if (diff < 0 || diff >= (S32)values_size) return INVALID_INDEX;
            return diff;
        }

        U16 get_next_visible(U16 i)
        {
            xASSERT_L(1648, i != INVALID_INDEX);
            const value_type& v = values[i];
            if (i + 1 >= (U16)values_size) return INVALID_INDEX;
            if (v.flags & DTF_EXPANDED) return i + 1;

            for (; i != INVALID_INDEX; i = values[i].parent)
                if (values[i].next != INVALID_INDEX) return values[i].next;
            return INVALID_INDEX;
        }

        U16 get_prev_visible(U16 i)
        {
            xASSERT_L(1662, i != INVALID_INDEX);
            if (!i) return INVALID_INDEX;

            for (U16 p = values[--i].parent; p != INVALID_INDEX; p = values[p].parent) {
                if (values[p].type == DT_BRANCH && !(values[p].flags & DTF_EXPANDED))
                    i = p;
            }
            return i;
        }

        U16 begin_child(U16 i)
        {
            if (i == INVALID_INDEX) return INVALID_INDEX;
            U16 child = i + 1;
            if (child >= values_size) return INVALID_INDEX;
            if (values[child].parent != i) return INVALID_INDEX;
            return child;
        }

        U16 end_child(U16 i)
        {
            for (; i != INVALID_INDEX; i = values[i].parent) {
                if (values[i].next != INVALID_INDEX) return values[i].next;
            }
            return values_size;
        }

        void set_selected(size_t sel)
        {
            if (sel == selected) return;
            if (selected != INVALID_INDEX) {
                values[selected].flags ^= DTF_SELECTED;
                for (size_t p = selected; p != INVALID_INDEX; p = values[p].parent)
                    on_unselect(values[p]);
            }
            selected = sel;
            if (selected != INVALID_INDEX) {
                values[selected].flags ^= DTF_SELECTED;
                for (size_t p = selected; p != INVALID_INDEX; p = values[p].parent)
                    on_select(values[p]);
            }
        }

        void refresh_visible()
        {
            if (selected == INVALID_INDEX) {
                first_visible = 0;
                return;
            }

            U16 prev = selected;
            if ((prev = get_prev_visible(prev)) == INVALID_INDEX || (prev = get_prev_visible(prev)) == INVALID_INDEX) {
                first_visible = 0;
                return;
            }

            if (prev <= first_visible) {
                first_visible = prev;
                return;
            }

            size_t past_selected = 0;
            F32 h = 0.01f + help_box.font.clip.h;
            F32 y = h + box_top();

            for (size_t i = first_visible; i < values_size;) {
                const value_type& v = values[i];
                if (i > selected) past_selected++;
                F32 name_height = set_box_name(v, y);
                F32 value_height = set_box_value(v, y);
                y += MAX(name_height, value_height);
                if (y >= 0.98f) {
                    if (past_selected > 2) break;
                    first_visible = get_next_visible(first_visible);
                }
                i = get_next_visible(i);
            }

            if (prev <= first_visible) {
                first_visible = prev;
            }
        }

        size_t depth(const value_type& v)
        {
            size_t total = 0;
            for (size_t i = v.parent; i != INVALID_INDEX; i = values[i].parent)
                total++;
            return total;
        }

        void get_font_size(xfont& f, const value_type& v, U32 noScaleFlag)
        {
            f.width = font_size_multiplier * tweak_box_value_font_width;
            f.height = font_size_multiplier * tweak_box_value_font_height;
            if ((v.flags & DTF_SELECTED) && !(v.flags & noScaleFlag)) {
                f.width *= 1.25f;
                f.height *= 1.25f;
            }
        }

        void fit_value(xtextbox& tb, const value_type& v, F32 y, F32 left, F32 right, F32 indent, U32 noScaleFlag)
        {
            get_font_size(tb.font, v, noScaleFlag);

            tb.font.clip.x = left;
            tb.font.clip.y = y;
            tb.font.clip.h = 0.98f - y;
            tb.font.clip.w = right - left;
            tb.bounds = tb.font.clip;
            tb.bounds.contract(indent, 0.0f, 0.0f, 0.0f);
        }

        void convert_underscore(char* out, const substr& in); // UNUSED

        F32 set_box_name(const value_type& v, F32 y)
        {
            fit_value(value_box, v, y, 0.06f, zNPCAnimViewer::IsPresent() ? 0.2f : 0.6f, 0.03f * depth(v), DTF_NO_SCALE_NAME);
            value_box.font.color = tweak_box_name_color[v.type];
            substr cname = child_name(v.name);
            value_box.set_text(cname.text, cname.size);
            return value_box.yextent(false);
        }

        F32 set_box_value(const value_type& v, F32 y)
        {
            fit_value(value_box, v, y, zNPCAnimViewer::IsPresent() ? 0.2f : 0.6f, 0.98f, 0.0f, DTF_NO_SCALE_VALUE);

            if ((v.flags & DTF_SELECTED) && zPad::GetDebugPad()->Pressed(0x10000) &&
                !(v.flags & DTF_READ_ONLY))
                value_box.font.color = tweak_box_value_press_color[v.type];
            else if ((v.flags & DTF_SELECTED) && editting)
                value_box.font.color = tweak_box_value_edit_color[v.type];
            else if (v.flags & DTF_READ_ONLY)
                value_box.font.color = tweak_box_value_read_only_color[v.type];
            else
                value_box.font.color = tweak_box_value_normal_color[v.type];

            static char buffer[128];
            if (editting && (v.flags & DTF_SELECTED))
                write_edit_value(buffer, v, v.edit_digit);
            else
                write_value(buffer, v, 0);

            value_box.set_text(buffer);

            return value_box.yextent(false);
        }

        void get_value(const value_type& v, U32& val)
        {
            switch (v.value_size) {
            case 1: val = *(U8*)v.value; break;
            case 2: val = *(U16*)v.value; break;
            case 4: val = *(U32*)v.value; break;
            default: xFAIL_L(1865);
            }
        }

        void set_value(const value_type& v, U32 val)
        {
            switch (v.value_size) {
            case 1: *(U8*)v.value = val; break;
            case 2: *(U16*)v.value = val; break;
            case 4: *(U32*)v.value = val; break;
            default: xFAIL_L(1876);
            }
        }

        void get_value(const value_type& v, S32& val)
        {
            switch (v.value_size) {
            case 1: val = *(S8*)v.value; break;
            case 2: val = *(S16*)v.value; break;
            case 4: val = *(S32*)v.value; break;
            default: xFAIL_L(1887);
            }
        }

        void set_value(const value_type& v, S32 val)
        {
            switch (v.value_size) {
            case 1: *(S8*)v.value = val; break;
            case 2: *(S16*)v.value = val; break;
            case 4: *(S32*)v.value = val; break;
            default: xFAIL_L(1898);
            }
        }

        U32 get_select_index(const value_type& v)
        {
            U32 val;
            get_value(v, val);
            if (!v.select_context.values) return val;
            switch (v.value_size) {
            case 1:
                {
                    U8* values = (U8*)v.select_context.values;
                    for (U32 i = 0; i < v.select_context.labels_size; i++)
                        if ((U8)val == values[i]) return i;
                }
                break;
            case 2:
                {
                    U16* values = (U16*)v.select_context.values;
                    for (U32 i = 0; i < v.select_context.labels_size; i++)
                        if ((U16)val == values[i]) return i;
                }
                break;
            case 4:
                {
                    U32* values = (U32*)v.select_context.values;
                    for (U32 i = 0; i < v.select_context.labels_size; i++)
                        if (val == values[i]) return i;
                }
                break;
            default: xFAIL_L(1930);
            }
            return v.select_context.labels_size;
        }

        U32 get_select_value(const value_type& v, U32 index)
        {
            if (!v.select_context.values) return index;
            xASSERT_L(1938, (index >= 0) && (index < v.select_context.labels_size));
            switch (v.value_size) {
            case 1: return ((U8*)v.select_context.values)[index];
            case 2: return ((U16*)v.select_context.values)[index];
            case 4: return ((U32*)v.select_context.values)[index];
            default: xFAIL_L(1944); return 0;
            }
        }

        S32 calc_precision(F32 v)
        {
            v *= 100000.0f;
            S32 p = 5;
            while (p > 1) {
                if ((S32)v % 10) return p;
                v *= 0.1f;
                p--;
            }
            return 1;
        }

        void write_value(char* buffer, const value_type& v, S32 width)
        {
            on_update(v);
            switch (v.type) {
            case DT_BRANCH:
                if (v.flags & DTF_SELECTED) {
                    buffer[0] = (v.flags & DTF_EXPANDED) ? '>' : '<';
                    buffer[1] = '\0';
                }
                else buffer[0] = '\0';
                break;
            case DT_INT:
                {
                    S32 val;
                    get_value(v, val);
                    convert_mem_to_tweak(v, &val);
                    if (width < 0) width = 0;

                    width++; if (val < 0) width++;
                    sprintf(buffer, "%0*d", width, val);
                }
                break;
            case DT_UINT:
                {
                    U32 val;
                    get_value(v, val);
                    convert_mem_to_tweak(v, &val);
                    if (width < 0) width = 0;

                    width++;
                    sprintf(buffer, "%0*u", width, val);
                }
                break;
            case DT_FLOAT:
                {
                    F32 val = *(F32*)v.value;
                    convert_mem_to_tweak(v, &val);
                    S32 lwidth = 1, rwidth = calc_precision(val);
                    if (width >= 0) lwidth = width + rwidth + 2 + xsignbit(val);
                    else if (rwidth < -width) rwidth = -width;
                    sprintf(buffer, "%0*.*f", lwidth, rwidth, val);
                }
                break;
            case DT_BOOL:
                {
                    bool val = *(bool*)v.value;
                    convert_mem_to_tweak(v, &val);
                    strcpy(buffer, val ? "true" : "false");
                }
                break;
            case DT_FLAG:
                {
                    U32 val;
                    get_value(v, val);
                    convert_mem_to_tweak(v, &val);
                    if ((val & v.flag_context.mask) == v.flag_context.mask)
                        strcpy(buffer, "true");
                    else strcpy(buffer, "false");
                }
                break;
            case DT_SELECT:
                {
                    U32 index = get_select_index(v);
                    if (index >= v.select_context.labels_size) buffer[0] = '\0';
                    else strcpy(buffer, v.select_context.labels[index]);
                }
                break;
            case DT_MESSAGE:
                {
                    const char* val = (const char*)v.value;
                    convert_mem_to_tweak(v, &val);
                    if (val)
                        strcpy(buffer, val);
                }
                break;
            }
        }

        bool read_value(value_type& v, const char* text)
        {
            switch (v.type) {
            case DT_INT:
                {
                    S32 oldval, val;
                    get_value(v, oldval);
                    convert_mem_to_tweak(v, &oldval);
                    val = atoi(text);
                    convert_tweak_to_mem(v, &val);
                    if (val == oldval) break;
                    set_value(v, val);
                    if (val > oldval) v.flags |= (U16)DTF_CHECKED;
                    else v.flags &= (U16)~DTF_CHECKED;
                    on_change(v);
                    return true;
                }
            case DT_UINT:
                {
                    U32 oldval, val;
                    get_value(v, oldval);
                    convert_mem_to_tweak(v, &oldval);
                    val = atoi(text);
                    convert_tweak_to_mem(v, &val);
                    if (val == oldval) break;
                    set_value(v, val);
                    if (val > oldval) v.flags |= (U16)DTF_CHECKED;
                    else v.flags &= (U16)~DTF_CHECKED;
                    on_change(v);
                    return true;
                }
            case DT_FLOAT:
                {
                    F32 oldval = *(F32*)v.value;
                    convert_mem_to_tweak(v, &oldval);
                    F32 val = atof(text);
                    convert_tweak_to_mem(v, &val);
                    if (xeq(val, oldval, 0.001f)) break;
                    *(F32*)v.value = val;
                    if (val > oldval) v.flags |= (U16)DTF_CHECKED;
                    else v.flags &= (U16)~DTF_CHECKED;
                    on_change(v);
                    return true;
                }
            case DT_BOOL:
                {
                    bool oldval = *(bool*)v.value;
                    bool val;
                    if (stricmp(text, "true") == 0) val = true;
                    else if (stricmp(text, "false") == 0) val = false;
                    else break;
                    if (val == oldval) break;
                    *(bool*)v.value = val;
                    if (val) v.flags |= (U16)DTF_CHECKED;
                    else v.flags &= (U16)~DTF_CHECKED;
                    on_change(v);
                    return true;
                }
            case DT_FLAG:
                {
                    U32 val;
                    get_value(v, val);
                    U32 oldval = val;
                    if (stricmp(text, "true") == 0) val |= v.flag_context.mask;
                    else if (stricmp(text, "false") == 0) val &= ~v.flag_context.mask;
                    else break;
                    if (val == oldval) break;
                    set_value(v, val);
                    if (val & v.flag_context.mask) v.flags |= (U16)DTF_CHECKED;
                    else v.flags &= (U16)~DTF_CHECKED;
                    on_change(v);
                    return true;
                }
            case DT_SELECT:
                {
                    U32 oldval;
                    get_value(v, oldval);
                    char value[128], label[128];
                    strcpy(value, text);
                    strip_ugly(value);
                    for (U32 i = 0; i < v.select_context.labels_size; i++) {
                        strcpy(label, v.select_context.labels[i]);
                        strip_ugly(label);
                        if (stricmp(value, label) == 0) {
                            U32 val = get_select_value(v, i);
                            if (val == oldval) break;
                            set_value(v, val);
                            on_change(v);
                            return true;
                        }
                    }
                }
                break;
            }

            return false;
        }

        void write_edit_value(char* buffer, const value_type& v, S32 digit)
        {
            if (v.type == DT_SELECT) {
                write_value(buffer, v, 0);
                return;
            }

            char temp[64];
            write_value(temp, v, digit);
            S32 len = strlen(temp);
            S32 loc;
            char* dp = std::strchr(temp, '.');
            if (dp == NULL) loc = len;
            else loc = dp - temp;
            if (digit >= 0) loc -= digit + 1;
            else loc -= digit;
            if (loc < 0) loc = 0;

            memcpy(buffer, temp, loc);
            buffer += loc;
            memcpy(buffer, "{c=ffdd3333}", 12);
#if DEBUG
            buffer += 12;
            *buffer++ = temp[loc];

            memcpy(buffer, "{~:c}", 6);
            buffer += 5;
            if (loc < len) memcpy(buffer, temp + loc + 1, len - loc);
#else
            buffer[12] = temp[loc];

            memcpy(buffer + 13, "{~:c}", 6);
            if (loc < len) memcpy(buffer + 18, temp + loc + 1, len - loc);
#endif
        }

        size_t value_index(value_type& v)
        {
            xASSERT_L(2174, (&v >= values) && (&v < (values + values_size)));
            return &v - values;
        }

        bool reset_value(value_type& v, bool recurse)
        {
            if (v.flags & DTF_READ_ONLY) return false;
            if (v.type == DT_BRANCH) {
                if (!recurse) return false;
                size_t i = value_index(v);
                U16 end = end_child(i);
                bool changed = false;
                for (++i; i < end; i++)
                    changed = reset_value(values[i], false) || changed;
                return changed;
            }

            if (v.value_size) {
                U32 val;
                get_value(v, val);
                if (val != v.uint_context.value_def) {
                    set_value(v, v.uint_context.value_def);
                    on_change(v);
                    return true;
                }
            }

            return false;
        }

        void store_value(value_type& v)
        {
            get_value(v, old_uint_value);
        }

        void restore_value(value_type& v)
        {
            U32 val;
            get_value(v, val);
            if (val == old_uint_value) return;
            set_value(v, old_uint_value);
            on_change(v);
        }

        void modify_value(value_type& v, S32 scale)
        {
            switch (v.type) {
            case DT_INT:
                {
                    S32 val;
                    get_value(v, val);
                    convert_mem_to_tweak(v, &val);

                    static const S32 lookup[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
                    S32 mult;
                    if (v.edit_digit < 0) mult = 1;
                    else if (v.edit_digit > 10) mult = lookup[9];
                    else mult = lookup[v.edit_digit];
                    F32 fsize = (F32)val + (F32)mult * scale;
                    if (fsize > 1000000000.0f) val = 1000000000;
                    else if (fsize < -1000000000.0f) val = -1000000000;
                    else val += mult * scale;
                    if (val > v.int_context.value_max) val = v.int_context.value_max;
                    else if (val < v.int_context.value_min) val = v.int_context.value_min;
                    convert_tweak_to_mem(v, &val);
                    set_value(v, val);
                }
                break;
            case DT_UINT:
                {
                    U32 val;
                    get_value(v, val);
                    convert_mem_to_tweak(v, &val);

                    static const U32 lookup[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
                    U32 mult;
                    if (v.edit_digit < 0) mult = 1;
                    else if (v.edit_digit > ARRAY_LENGTH(lookup)) mult = lookup[ARRAY_LENGTH(lookup) - 1];
                    else mult = lookup[v.edit_digit];
                    F32 fsize = (F32)val + (F32)mult * scale;
                    if (fsize > 1000000000.0f) val = 1000000000;
                    else if (fsize < 0.0f) val = 0;
                    else val += mult * scale;
                    if (val > v.uint_context.value_max) val = v.uint_context.value_max;
                    else if (val < v.uint_context.value_min) val = v.uint_context.value_min;
                    convert_tweak_to_mem(v, &val);
                    set_value(v, val);
                }
                break;
            case DT_FLOAT:
                {
                    F32 val = *(F32*)v.value;
                    convert_mem_to_tweak(v, &val);
                    if (v.edit_digit >= 0) {
                        static const F32 lookup[] = { 1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f,
                                                      10000000.0f, 100000000.0f, 1000000000.0f };
                        F32 mult;
                        if (v.edit_digit > 10) mult = lookup[9];
                        else mult = lookup[v.edit_digit];
                        F32 fsize = val + mult * scale;
                        if (fsize > 1000000000.0f) val = 1000000000.0f;
                        else if (fsize < -1000000000.0f) val = -1000000000.0f;
                        else val = fsize;
                        if (val > v.float_context.value_max) val = v.float_context.value_max;
                        else if (val < v.float_context.value_min) val = v.float_context.value_min;
                    } else {
                        static const F32 lookup[] = { 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f };
                        S32 ed = -v.edit_digit - 1;
                        F32 mult;
                        if (ed > 6) mult = lookup[5];
                        else mult = lookup[ed];
                        F32 fsize = val + mult * scale;
                        if (fsize > 1000000000.0f) val = 1000000000.0f;
                        else if (fsize < -1000000000.0f) val = -1000000000.0f;
                        else val = fsize;
                        if (val > v.float_context.value_max) val = v.float_context.value_max;
                        else if (val < v.float_context.value_min) val = v.float_context.value_min;
                    }
                    convert_tweak_to_mem(v, &val);
                    *(F32*)v.value = val;
                }
                break;
            case DT_SELECT:
                {
                    S32 index = get_select_index(v);
                    index += scale;
                    if (scale > 0 && index >= (S32)v.select_context.labels_size)
                        index = v.select_context.labels_size - 1;
                    else if (scale < 0 && index < 0)
                        index = 0;
                    if (index >= 0 && index < (S32)v.select_context.labels_size) {
                        set_value(v, get_select_value(v, index));
                    }
                }
                break;
            }

            if (scale > 0) v.flags |= (U16)DTF_CHECKED;
            else v.flags &= (U16)~DTF_CHECKED;

            on_change(v);
        }

        S32 count_digits(S32 v)
        {
            v = (v < 0) ? -v : v;
            S32 total = 1;
            for (v /= 10; v > 0; v /= 10) total++;
            return total;
        }

        S32 count_digits(U32 v); // UNUSED

        S32 count_digits(F32 v)
        {
            v = xabs(v);
            S32 total = 1;
            for (v *= 0.1f; (S32)v > 0; v *= 0.1f) total++;
            return total;
        }

        void init_edit_digit(value_type& v); // UNUSED

        void clip_edit_digit(value_type& v)
        {
            switch (v.type) {
            case DT_INT:
            case DT_UINT:
                {
                    S32 mindigits = count_digits(v.int_context.value_min);
                    S32 maxdigits = count_digits(v.int_context.value_max);
                    S32 digits = MAX(maxdigits, mindigits);
                    if (v.edit_digit >= digits) v.edit_digit = digits - 1;
                    else if (v.edit_digit < 0) v.edit_digit = 0;
                }
                break;
            case DT_FLOAT:
                {
                    S32 mindigits = count_digits(v.float_context.value_min);
                    S32 maxdigits = count_digits(v.float_context.value_max);
                    S32 digits = MAX(maxdigits, mindigits);
                    if (v.edit_digit >= digits) v.edit_digit = digits - 1;
                    else if (v.edit_digit < -5) v.edit_digit = -5;
                }
                break;
            }
        }

        S32 compare_U16(const void* e1, const void* e2)
        {
            const U16* v1 = (const U16*)e1, *v2 = (const U16*)e2;
            return *v1 - *v2;
        }

        void sort_changed()
        {
            if (!need_sort_changed) return;
            qsort(changed_values, changed_values_size, sizeof(U16), compare_U16);
            need_sort_changed = false;
        }

        void refresh_changed()
        {
            changed_values_size = 0;
            value_type* v = values, *endv = v + values_size;
            for (; v != endv; v++)
                if (v->flags & DTF_CHANGED) {
                    v->flags &= (U16)~DTF_CHANGED;
                    mark_changed(*v);
                }
            need_sort_changed = false;
        }

        void mark_unchanged(value_type& v)
        {
            if (!(v.flags & DTF_CHANGED)) return;
            if (changed_values_size > ARRAY_LENGTH(changed_values)) {
                changed_values_size--;
                return;
            }
            if (changed_values_size == ARRAY_LENGTH(changed_values)) {
                refresh_changed();
                return;
            }
            v.flags &= (U16)~DTF_CHANGED;
            if (v.change_index >= changed_values_size) return;
            need_sort_changed = true;
            U16& ci = changed_values[v.change_index];
            U16 vi = get_index(v);
            if (ci != vi) return;
            changed_values_size--;
            ci = changed_values[changed_values_size];
            values[ci].change_index = v.change_index;

            for (U32 i = 0; i < changed_values_size; i++) {
                for (U32 j = i + 1; j < changed_values_size; j++) {
                    if (changed_values[i] == changed_values[j]) {
                        S32 asdf = 1234;
                    }
                }
            }
        }

        void mark_changed(value_type& v)
        {
            if (v.flags & DTF_CHANGED) return;
            need_sort_changed = true;
            v.flags |= (U16)DTF_CHANGED;
            if (changed_values_size < ARRAY_LENGTH(changed_values))
                changed_values[changed_values_size] = get_index(v);
            v.change_index = changed_values_size;
            changed_values_size++;

            for (U32 i = 0; i < changed_values_size; i++) {
                for (U32 j = i + 1; j < changed_values_size; j++) {
                    if (changed_values[i] == changed_values[j]) {
                        S32 asdf = 1234;
                    }
                }
            }
        }

        void on_change(value_type& v)
        {
            if (v.cb && v.cb->on_change) v.cb->on_change(v);

            if (!v.value_size) return;
            U32 val;
            get_value(v, val);
            if (val == v.uint_context.value_def)
                mark_unchanged(v);
            else
                mark_changed(v);
        }

        void on_select(const value_type& v) { if (v.cb && v.cb->on_select) v.cb->on_select(v); }

        void on_unselect(const value_type& v) { if (v.cb && v.cb->on_unselect) v.cb->on_unselect(v); }

        void on_start_edit(const value_type& v) { if (v.cb && v.cb->on_start_edit) v.cb->on_start_edit(v); }

        void on_stop_edit(const value_type& v) { if (v.cb && v.cb->on_stop_edit) v.cb->on_stop_edit(v); }

        void on_expand(const value_type& v) { if (v.cb && v.cb->on_expand) v.cb->on_expand(v); }

        void on_collapse(const value_type& v) { if (v.cb && v.cb->on_collapse) v.cb->on_collapse(v); }

        void on_update(const value_type& v) { if (v.cb && v.cb->on_update) v.cb->on_update(v); }

        void convert_mem_to_tweak(const value_type& v, void* value)
        {
            if (v.cb && v.cb->convert_mem_to_tweak)
                v.cb->convert_mem_to_tweak(v, value);
        }

        void convert_tweak_to_mem(const value_type& v, void* value)
        {
            if (v.cb && v.cb->convert_tweak_to_mem)
                v.cb->convert_tweak_to_mem(v, value);
        }

        bool move_down()
        {
            if (selected + 1 >= values_size) return false;

            U16 next = get_next_visible(selected);
            if (next == INVALID_INDEX) return false;
            set_selected(next);
            return true;
        }

        U8 is_hidden(const value_type& v); // UNUSED

        bool move_up()
        {
            if (!selected) return false;
            U16 prev = get_prev_visible(selected);
            if (prev == INVALID_INDEX) return false;
            set_selected(prev);
            return true;
        }

        S32 timed_move(F32 dt)
        {
            if (!moving) {
                moving = true;
                move_time = -tweak_move_delay;
                return 1;
            }

            move_time += dt;

            F32 move_speed = zPad::GetDebugPad()->On(0x200) ? tweak_move_super_speed : tweak_move_speed;

            F32 inc_time = 1.0f / move_speed;
            if (move_time < inc_time)
                return 0;
            S32 inc = move_time * move_speed;
            move_time -= inc * inc_time;
            return inc;
        }

        void refresh_dump_file()
        {
            strcpy(dump_file, tweak_dump_files[dump_file_index]);
        }

        void strip_ugly(char* dst, const char* src)
        {
            S32 depth = 0;
            for (; *src; src++) {
                char c = *src;
                if (c < ' ' && c != '\t') continue;

                if (c == '{') depth++;
                if (depth <= 0) *dst++ = c;
                if (c == '}') depth--;
            }
            *dst = '\0';
        }

        void strip_ugly(char* dst, const substr& ssrc)
        {
            S32 depth = 0;
            const char* src = ssrc.text, *end_src = src + ssrc.size;
            for (; src != end_src; src++) {
                char c = *src;
                if (c < ' ' && c != '\t') continue;

                if (c == '{') depth++;
                if (depth <= 0) *dst++ = c;
                if (c == '}') depth--;
            }
            *dst = '\0';
        }

        void strip_ugly(char* text)
        {
            char buffer[256];
            strcpy(buffer, text);

            strip_ugly(text, buffer);
        }

        inline bool filter_value(size_t i, filter_mode filter)
        {
            xASSERT_L(2647, i < values_size);
            const value_type& v = values[i];
            if (v.type == DT_BRANCH) return false;
            switch (filter) {
            case FILTER_OPEN:
                for (size_t pi = v.parent; pi != INVALID_INDEX; pi = values[pi].parent)
                    if (!(values[pi].flags & DTF_EXPANDED)) return false;
                return true;
            case FILTER_CHANGED:
                return (v.flags & DTF_CHANGED) != 0;
            case FILTER_ALL: break;
            }

            return true;
        }

        void save_tweaks(filter_mode filter)
        {
            xFile file;
            S32 flags = 0x8E;
            char buffer[128];

            if (iFileOpen(dump_file, flags, &file)) return;

            refresh_dump_file();

            sprintf(buffer, "# %s\r\n# In-game tweak variables.\r\n", dump_file);
            iFileWrite(&file, buffer, strlen(buffer));

            if (filter == FILTER_CHANGED && changed_values_size < ARRAY_LENGTH(changed_values))
                write_changed_tweaks(file, filter);
            else
                write_all_tweaks(file, filter);

            iFileClose(&file);
        }

        void write_group(xFile& file, const value_type& v)
        {
            char buffer[256];
            xASSERT_L(2689, v.parent != INVALID_INDEX);
            const value_type& pv = values[v.parent];
            iFileWrite(&file, (void*)"\r\n", strlen("\r\n"));
            sprintf(buffer, "# Group \"%.*s\"", pv.name.size, pv.name.text);
            strip_ugly(buffer);
            iFileWrite(&file, buffer, strlen(buffer));
            iFileWrite(&file, (void*)"\r\n", strlen("\r\n"));
        }

        void write_tweak(xFile& file, const value_type& v, size_t min_key_len)
        {
            char buffer[256], value_buffer[128];

            write_value(value_buffer, v, 0);

            sprintf(buffer, "%.*s", v.name.size, v.name.text);
            strip_ugly(buffer);
            size_t name_len = strlen(buffer);
            size_t pad_len = name_len < min_key_len ? min_key_len - name_len : 0;
            sprintf(buffer + name_len, "%.*s= %s", pad_len, whitespace_buffer, value_buffer);
            strip_ugly(buffer + name_len);
            iFileWrite(&file, buffer, strlen(buffer));
            iFileWrite(&file, (void*)"\r\n", strlen("\r\n"));
        }

        void write_all_tweaks(xFile& file, filter_mode filter)
        {
            size_t min_key_len = 0;
            for (size_t i = 0; i < values_size; i++)
                if (filter_value(i, filter)) {
                    const value_type& v = values[i];
                    if (min_key_len < v.name.size) min_key_len = v.name.size;
                }
            min_key_len++;
            if (min_key_len > ARRAY_LENGTH(whitespace_buffer) - 1) {
                min_key_len = ARRAY_LENGTH(whitespace_buffer) - 1;
            }

            U16 parent = INVALID_INDEX;
            for (size_t i = 0; i < values_size; i++)
                if (filter_value(i, filter)) {
                    const value_type& v = values[i];
                    if (v.parent != parent) {
                        parent = v.parent;
                        write_group(file, v);
                    }
                    write_tweak(file, v, min_key_len);
                }
        }

        void write_changed_tweaks(xFile& file, filter_mode)
        {
            sort_changed();
            const U16* cv, *end_cv = changed_values + changed_values_size;
            size_t min_key_len = 0;
            for (cv = changed_values; cv != end_cv; cv++) {
                const value_type& v = values[*cv];
                if (min_key_len < v.name.size) min_key_len = v.name.size;
            }
            min_key_len++;
            if (min_key_len > ARRAY_LENGTH(whitespace_buffer) - 1) {
                min_key_len = ARRAY_LENGTH(whitespace_buffer) - 1;
            }

            U16 parent = INVALID_INDEX;
            for (cv = changed_values; cv != end_cv; cv++) {
                const value_type& v = values[*cv];
                if (v.parent != parent) {
                    parent = v.parent;
                    write_group(file, v);
                }
                write_tweak(file, v, min_key_len);
            }
        }

        S32 compare_stripped(const substr& ss1, const substr& ss2); // UNUSED
        char* skip_ugly(const char* s); // UNUSED

        value_type* find_stripped(const char* key)
        {
            size_t lower = 0, upper = values_size;
            while ((S32)lower < (S32)upper) {
                size_t i = (lower + upper) / 2;
                const stripped_value_type& sv = stripped_values[i];
                S32 cmp = sistrcmp(key, sv.name);
                if (cmp < 0) upper = i;
                else if (cmp > 0) lower = i + 1;
                else return sv.value;
            }
            return NULL;
        }

        value_type* find_stripped(const substr& key)
        {
            size_t lower = 0, upper = values_size;
            while ((S32)lower < (S32)upper) {
                size_t i = (lower + upper) / 2;
                const stripped_value_type& sv = stripped_values[i];
                substr vname = { sv.name, strlen(sv.name) };
                S32 cmp = sicompare(key, vname);
                if (cmp < 0) upper = i;
                else if (cmp > 0) lower = i + 1;
                else return sv.value;
            }
            return NULL;
        }

        void sort_stripped_values()
        {
            if (!need_sort_stripped) return;
            value_type* v = values, *endv = v + values_size;
            stripped_value_type* sv = stripped_values;
            for (; v != endv; ++v, ++sv) {
                strip_ugly(sv->name, v->name);
                sv->value = v;
            }
            qsort(stripped_values, values_size, sizeof(stripped_value_type), sistrcmp);
            need_sort_stripped = false;
        }

        void touch_branch(const char* name)
        {
            if (!name || !*name) return;

            substr path[8];
            S32 total = 1;
            path[0] = substr::create(name, strlen(name));
            for (; total < 8 && path[total - 1].size; total++)
                path[total] = parent_name(path[total - 1]);

            total--;
            for (size_t retries = 0; retries < 10; retries++) {
                values_dirty = false;
                for (S32 i = total; i > 0;) {
                    i--;
                    value_type* v = find_stripped(path[i]);
                    if (!v) break;
                    if (v->type != DT_BRANCH) break;
                    if (v->flags & DTF_EXPANDED) continue;
                    if (!v->cb) continue;
                    if (!v->cb->on_expand && !v->cb->on_collapse) continue;
                    on_expand(*v);
                    on_collapse(*v);
                }
                flush_and_relink();
                sort_stripped_values();
                if (!values_dirty) break;
            }
        }

        void load_tweaks()
        {
            xFile file;
            xIniFile* ini = NULL;
            char* buffer = NULL;
            bool opened;
            U32 fsize;

            opened = iFileOpen(dump_file, 0x81, &file) == 0;
            refresh_dump_file();
            if (!opened) {
                goto done;
            }

            fsize = iFileGetSize(&file);
            if (!fsize) goto done;
            buffer = (char*)xMEMPUSHTEMP_L(fsize, 0, 2886);
            if (!buffer) goto done;
            if (fsize != iFileRead(&file, buffer, fsize)) {
                goto done;
            }

            ini = xIniParse(buffer, fsize);
            if (!ini) goto done;

            flush_and_relink();
            sort_stripped_values();

            values_dirty = false;
            for (xIniValue* it = ini->Values, *end = it + ini->NumValues; it != end; it++)
                touch_branch(it->tok);

            for (size_t retries = 0; retries < 10; retries++) {
                values_dirty = false;
                xIniValue* iv = ini->Values, *end_iv = iv + ini->NumValues;
                for (; iv < end_iv; iv++) {
                    if (!iv->val) continue;
                    value_type* v = find_stripped(iv->tok);
                    if (!v) continue;
                    if (v->flags & DTF_NO_LOAD) continue;
                    if (read_value(*v, iv->val) && v->cb && v->cb->on_change) {
                        flush_and_relink();
                        sort_stripped_values();
                    }
                    iv->val = NULL;
                }
                if (!values_dirty) break;
            }

        done:
            if (opened) iFileClose(&file);
            if (ini) xIniDestroy(ini);
            if (buffer) xMEMPOPTEMP(buffer);
        }

        void badchar_to_hex(char* oname, const substr& iname)
        {
            char* dst = oname;
            const char* src = iname.text, *end_src = src + iname.size;
            for (; src != end_src; src++) {
                char c = *src;
                if ((c < ' ' || c > '127' || c == '#') && c != '\t') {
                    sprintf(dst, "\\x%02x", c);
                    dst += 4;
                } else {
                    *dst = c;
                    dst++;
                }
            }
            *dst = '\0';
        }

        inline S32 convert_hex_digit(S32 c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        }

        void hex_to_badchar(char* oname, const char* iname)
        {
            char* dst = oname;
            const char* src = iname;
            while (true) {
                const char* next_hex = strstr(src, "\\x");
                if (!next_hex || strlen(next_hex) < 4) {
                    strcpy(dst, src);
                    break;
                }
                if (next_hex > src) {
                    memcpy(dst, src, next_hex - src);
                    dst += next_hex - src;
                    src = next_hex;
                }
                S32 hex1 = convert_hex_digit(src[2]);
                S32 hex2 = convert_hex_digit(src[3]);
                if (hex1 < 0 || hex2 < 0) {
                    dst[0] = src[0]; dst[1] = src[1];

                    src += 2; dst += 2; continue;
                }
                S32 val = (hex1 << 4) | hex2;
                *dst = val;
                dst++;

                src += 4;
            }
        }

        void automenu_save()
        {
            if (!automenu_enable.any) {
                return;
            }

            char fname[] = "automenu.ini";
            xFile file;
            S32 flags = 0x8E;
            char buffer[256], write_buffer[512];

            if (iFileOpen(fname, flags, &file)) {
                return;
            }

            sprintf(buffer, "# automenu.ini\r\n# Tweak menu context.\r\n");
            iFileWrite(&file, buffer, strlen(buffer));

            if (automenu_enable.selected) {
                if (selected != INVALID_INDEX) {
                    badchar_to_hex(buffer, values[selected].name);
                    sprintf(write_buffer, "selected=%s\r\n", buffer);
                    iFileWrite(&file, write_buffer, strlen(write_buffer));
                }
                if (first_visible != INVALID_INDEX) {
                    badchar_to_hex(buffer, values[first_visible].name);
                    sprintf(write_buffer, "first_visible=%s\r\n", buffer);
                    iFileWrite(&file, write_buffer, strlen(write_buffer));
                }
            }

            if (automenu_enable.opened) {
                value_type* v = values, *endv = v + values_size;
                for (; v != endv; v++) {
                    if (v->type != DT_BRANCH) continue;
                    if (!(v->flags & DTF_EXPANDED)) continue;
                    badchar_to_hex(buffer, v->name);
                    sprintf(write_buffer, "opened=%s\r\n", buffer);
                    iFileWrite(&file, write_buffer, strlen(write_buffer));
                }
            }

            iFileClose(&file);
        }

        void automenu_load()
        {
            xFile file;
            xIniFile* ini = NULL;
            char* buffer = NULL;
            char fname[] = "automenu.ini";
            bool opened; U32 fsize;
            char name[256];

            opened = iFileOpen(fname, 0x81, &file) == 0;
            if (!opened) {
                goto done;
            }

            fsize = iFileGetSize(&file);
            if (!fsize) goto done;
            buffer = (char*)xMEMPUSHTEMP_L(fsize, 0, 3062);
            if (!buffer) goto done;
            if (fsize != iFileRead(&file, buffer, fsize)) {
                goto done;
            }

            ini = xIniParse(buffer, fsize);
            if (!ini) goto done;

            flush_and_relink();

            if (automenu_enable.opened) {
                xIniValue* it = ini->Values, *end = it + ini->NumValues;
                for (; it != end; it++) {
                    if (strcmp(it->tok, "opened") != 0) continue;

                    hex_to_badchar(name, it->val);
                    size_t at = find_lowest(name);
                    if (at == INVALID_INDEX) continue;

                    value_type& v = values[at];
                    if (!(v.flags & DTF_EXPANDED) && v.type == DT_BRANCH) {
                        v.flags |= DTF_EXPANDED;
                        on_expand(v);
                    }
                }
            }

            if (automenu_enable.selected) {
                const char* selected_text = NULL, *first_visible_text = NULL;
                xIniValue* it = ini->Values, *end = it + ini->NumValues;
                for (; it != end; it++) {
                    if (!selected_text && strcmp(it->tok, "selected") == 0)
                        selected_text = it->val;
                    else if (!first_visible_text && strcmp(it->tok, "first_visible") == 0)
                        first_visible_text = it->val;
                    if (selected_text && first_visible_text) break;
                }
                if (first_visible_text) {
                    hex_to_badchar(name, first_visible_text);
                    size_t at = find_lowest(name);
                    if (at != INVALID_INDEX)
                        first_visible = at;
                }
                if (selected_text) {
                    hex_to_badchar(name, selected_text);
                    select(name);
                }
            }

        done:
            if (opened) iFileClose(&file);
            if (ini) xIniDestroy(ini);
            if (buffer) xMEMPOPTEMP(buffer);
        }

        void on_change_file_index(const tweak_info&)
        {
            refresh_dump_file();
        }

        void on_change_save(const tweak_info&)
        {
            save_tweaks(FILTER_ALL);
        }

        void on_change_save_changed(const tweak_info&)
        {
            save_tweaks(FILTER_CHANGED);
        }

        void on_change_save_opened(const tweak_info&)
        {
            save_tweaks(FILTER_OPEN);
        }

        void on_change_load(const tweak_info&)
        {
            load_tweaks();
        }

        void expand_all()
        {
            flush();

            size_t last_size;
            do {
                last_size = values_size;
                for (size_t i = 0; i < values_size; i++) {
                    value_type& v = values[i];
                    if (v.type == DT_BRANCH) v.flags |= DTF_EXPANDED;
                }
                for (size_t i = 0; i < values_size; i++) {
                    value_type& v = values[i];
                    if (v.type == DT_BRANCH) on_expand(v);
                }
                flush();
            } while (last_size != values_size);
        }

        void collapse_all()
        {
            flush();

            size_t last_size;
            do {
                last_size = values_size;
                for (size_t i = 0; i < values_size; i++) {
                    value_type& v = values[i];
                    if (v.type == DT_BRANCH) v.flags &= (U16)~DTF_EXPANDED;
                }
                for (size_t i = 0; i < values_size; i++) {
                    value_type& v = values[i];
                    if (v.type == DT_BRANCH) on_collapse(v);
                }
                flush();
            } while (last_size != values_size);
            if (selected < values_size) expand(selected);
        }

        void expand_all(size_t at)
        {
            if (at == INVALID_INDEX) {
                return;
            }

            char name_buffer[128];
            substr name = { name_buffer, values[at].name.size };
            memcpy(name_buffer, values[at].name.text, name.size);
            name_buffer[name.size] = '\0';

            expand(at);
            flush();

            size_t last_size;
            do {
                last_size = values_size;

                if (!find(name, at)) {
                    return;
                }

                U16 ci = begin_child(at);
                if (ci == INVALID_INDEX) return;

                U16 end_ci = end_child(at);
                if (end_ci == INVALID_INDEX) {
                    end_ci = values_size;
                }

                value_type* v = values + ci, *endv = values + end_ci;
                for (; v != endv; v++) {
                    if (v->type == DT_BRANCH && !(v->flags & DTF_EXPANDED)) {
                        v->flags |= DTF_EXPANDED;
                        on_expand(*v);
                        if (values_size != last_size) break;
                    }
                }

                flush();
            } while (last_size != values_size);
        }

        void collapse_all(size_t at)
        {
            if (at == INVALID_INDEX) {
                return;
            }

            collapse(at);

            U16 ci = begin_child(at);
            if (ci == INVALID_INDEX) return;

            U16 end_ci = end_child(at);
            if (end_ci == INVALID_INDEX) {
                end_ci = values_size;
            }

            value_type* v = values + ci, *endv = values + end_ci;
            for (; v != endv; v++) {
                if (v->type == DT_BRANCH && (v->flags & DTF_EXPANDED)) {
                    v->flags &= (U16)~DTF_EXPANDED;
                    on_collapse(*v);
                }
            }
        }

        void on_change_expand_all(const tweak_info&)
        {
            expand_all();
        }

        void on_change_collapse_all(const tweak_info&)
        {
            collapse_all();
        }

        void test_callback(const tweak_info& v)
        {
            static S32 count = 0;
            char* s = (char*)v.value;
            size_t len = strlen(s);
            count = (count + 1) % 10;
            s[len - 1] = count + '0';
        }

        void test_add_remove(const tweak_info&)
        {
            static bool above = true;
            if (above) {
                xDebugRemoveTweak("\001{c=ffffff00}Tweak Box|tester|2 Below");
                xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|2 Below|3", "Whatever", NULL, NULL, 0);
                xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|0 Above|1", "Whatever", NULL, NULL, 0);
                xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|0 Above|2", "Whatever", NULL, NULL, 0);
            } else {
                xDebugRemoveTweak("\001{c=ffffff00}Tweak Box|tester|0 Above");
                xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|0 Above|3", "Whatever", NULL, NULL, 0);
                xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|2 Below|1", "Whatever", NULL, NULL, 0);
                xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|2 Below|2", "Whatever", NULL, NULL, 0);
            }

            above = !above;
        }

        void test_add_overflow(const tweak_info& v)
        {
            S32 total = atoi((char*)v.value);
            char buffer[128];
            static F32 some_data = 123.0f;
            for (S32 i = total; i < total + 100; i++) {
                sprintf(buffer, "\001{c=ffffff00}Tweak Box|tester|Overflow Data|%04d", i);
                xDebugAddTweak(buffer, &some_data, -1000000000.0f, 1000000000.0f, NULL, NULL, DTF_BUFFER);
            }
            total += 100;
            sprintf((char*)v.value, "%d", total);
        }

        void test_clear_overflow(const tweak_info& v)
        {
            strcpy((char*)v.value, "0");
            xDebugRemoveTweak("\001{c=ffffff00}Tweak Box|tester|Overflow Data");
        }

        void add_test_tweaks()
        {
            static U32 test_u32 = 11;
            static U16 test_u16 = 12;
            static U8 test_u8 = 13;
            static S32 test_i32 = 14;
            static S16 test_i16 = 15;
            static S8 test_i8 = 16;
            static F32 test_f32 = 17.0f;
            static bool test_bool = false;
            static U32 test_select = 0;
            static char test_callback_buffer[32] = "sploink0";
            static xVec3 test_vec = { -1.0f, 0.0f, 1.0f };
            static const char* test_select_labels[] = { "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine" };
            static const F32 test_select_values[] = { 1.1f, -2.0f, 3.001f, 4.0f, 50.0f, 600.0f, 6.999f, 8.3f, -9.0f };
            static F32 test_select_float = 3.001f;

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test u32", &test_u32, 0, 0, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test u16", &test_u16, 0, 10000, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test group|test u8", &test_u8, 0, 100, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test i32", &test_i32, -100000, 100000, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test group|test i16", &test_i16, -10000, 10000, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test i8", &test_i8, -100, 100, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test group|test f32", &test_f32, -1.0f, -1.0f, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test bool", &test_bool, NULL, NULL, 0);
            xDebugAddSelectTweak("\001{c=ffffff00}Tweak Box|tester|test select", &test_select, test_select_labels, NULL,
                                 ARRAY_LENGTH(test_select_labels), NULL, NULL, 0);
            xDebugAddSelectTweak("\001{c=ffffff00}Tweak Box|tester|test map", &test_select_float, test_select_labels,
                                 test_select_values, ARRAY_LENGTH(test_select_labels), NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test map value", &test_select_float, 0.0f, 0.0f, NULL, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test read-only", "splat", NULL, NULL, 0);

            static const tweak_callback test_cb = { test_callback };

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test callback", test_callback_buffer, &test_cb, NULL, DTF_BUFFER);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test callback", test_callback_buffer, &test_cb, NULL, DTF_BUFFER);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test callback", test_callback_buffer, &test_cb, NULL, 0);
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test callback", test_callback_buffer, &test_cb, NULL, DTF_BUFFER);
            xDebugRemoveTweak("\001{c=ffffff00}Tweak Box|tester|test callback");

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test callback", test_callback_buffer, &test_cb, NULL, 0);

            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|test vector|", &test_vec, NULL, NULL, 0);

            static const tweak_callback test_add_remove_cb = { test_add_remove };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|1 Add/Remove Node", "Toggle", &test_add_remove_cb, NULL, 0);

            static char overflow_total[16] = "0";
            static const tweak_callback test_add_overflow_cb = { test_add_overflow };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|Overflow \001Add 100", overflow_total, &test_add_overflow_cb, NULL, 0);

            static const tweak_callback test_clear_overflow_cb = { test_clear_overflow };
            xDebugAddTweak("\001{c=ffffff00}Tweak Box|tester|Overflow \002Clear", overflow_total, &test_clear_overflow_cb, NULL, 0);
        }

        void validate_stuff()
        {
            static S32 count = 0;
            if (count) return;
            count = (count + 1) % 1000;

            size_t total = 0;
            for (size_t i = 0; i < values_size; i++) {
                if (values[i].flags & DTF_BUFFER) {
                    strings->get_iterator((string_type*)values[i].name.text);
                    total++;
                }
            }
            xASSERT_L(3405, total == strings->size());
            if (total >= strings->size()) return;

            for (string_list::iterator it = strings->begin(); it != strings->end(); ++it) {
                bool found = false;
                substr s = { it->text, strlen(it->text) };
                for (size_t i = 0; i < values_size; i++) {
                    if ((values[i].flags & DTF_BUFFER) && sicompare(values[i].name, s) == 0) {
                        found = true;
                        break;
                    }
                }
                xASSERT_FMT_L(3421, found, "Tweak: String leak: %s", s.text);
            }

            for (string_list::iterator it = strings->begin(); it != strings->end(); ++it) {
                string_list::iterator it2 = it; ++it2;
                for (; it2 != strings->end(); ++it2) {
                    xASSERT_FMT_L(3432, stricmp(it->text, it2->text) != 0, "Tweak: String duplicate: %s", it->text);
                }
            }
        }
    }
}

namespace {
    void xDebugAddTweak(const tweak_info& ti)
    {
        char buffer[16];

        if ((void*)ti.name.text >= buffer && (void*)ti.name.text <= buffer + 4096 && !(ti.flags & DTF_BUFFER)) {
            xFAIL_MSG_L(3451, "tweak most likely on stack");
        }

        if (!debug_tweak_inited) debug_tweak::reset();
        debug_tweak::insert(ti);
    }
}

static bool hide;

void debug_mode_tweak_begin()
{
    if (xprintf_display) xprintf_display = 2;
    debug_tweak::autoload();
}

void debug_mode_tweak_end()
{
    if (xprintf_display == 2) xprintf_display = 1;
    debug_tweak::autosave();
}

void debug_mode_tweak()
{
    static bool was_hidden = false;

    xprintf_display_modes = 3;
    switch (xprintf_display) {
    case 1:
        hide = false;
        debug_tweak::whole_screen = false;
        break;
    case 2:
        hide = false;
        debug_tweak::whole_screen = true;
        break;
    case 0:
    default:
        hide = true;
        debug_tweak::whole_screen = false;
    }

    if (hide && !was_hidden) {
        debug_tweak::autosave();
    }
    else if (!hide && was_hidden) {
        debug_tweak::autoload();
    }

    was_hidden = hide;
}

void xDebugTweakUpdate(F32 dt)
{
    xtextbox::log_layout_stats(false);
    if (!debug_tweak_inited) debug_tweak::reset();
    debug_tweak::update(dt);
    xtextbox::log_layout_stats(true);
}

void xDebugTweakRender()
{
    if (gScreenDump == exSD_MASSIVE || gScreenDump == exSD_MASSIVE_RESET) {
        return;
    }

    zRenderStateBegin();
    iCameraSetFog(false);

    gDebugTweakRendering = true;
    xtextbox::log_layout_stats(false);
    if (!debug_tweak_inited) debug_tweak::reset();
    if (!hide) debug_tweak::render();
    xtextbox::log_layout_stats(true);
    gDebugTweakRendering = false;
}

void xDebugRemoveTweak(const substr& name)
{
    if (!debug_tweak_inited) debug_tweak::reset();
    debug_tweak::erase(name);
}

void xDebugRemoveTweaks(const substr& name, xDebugRemoveTweaksCallback cb, void* context)
{
    if (!debug_tweak_inited) debug_tweak::reset();
    debug_tweak::erase(name, cb, context);
}

void xDebugResetTweaks()
{
    debug_tweak::reset();
}

tweak_info* xDebugGetTweak(const char* name)
{
    xASSERT_L(3582, name != 0);
    return xDebugGetTweak(substr::create(name, strlen(name)));
}

tweak_info* xDebugGetTweak(const substr& name)
{
    return debug_tweak::find(name);
}

tweak_info* xDebugGetParentTweak(const tweak_info& info)
{
    return debug_tweak::get_parent(info);
}

U32 xDebugCountChildTweaks(const tweak_info& info)
{
    return debug_tweak::count_children(info);
}

void xDebugSelectTweak(const char* name)
{
    debug_tweak::select(name);
}

void xDebugExpandTweak(const char* name)
{
    debug_tweak::expand(name);
}

tweak_info* xDebugGetSelectedTweak()
{
    return debug_tweak::get_selected();
}

bool xDebugTweakSelected(const char* name)
{
    const tweak_info* val = debug_tweak::find(substr::create(name, strlen(name)));
    if (!val)
        return false;
    const tweak_info* sel = debug_tweak::get_selected();
    for (; sel; sel = debug_tweak::get_parent(*sel)) {
        if (sel == val)
            return true;
    }
    return false;
}

#define ADD_TWEAK_L(T, dt, ctx, line)                                                                                                               \
    void xDebugAddTweak(const char* name, T* v, T vmin, T vmax, const tweak_callback* cb, void* context, U32 flags)                                 \
    {                                                                                                                                               \
        xASSERT_L(line, v != 0);                                                                                                                    \
        tweak_info ti;                                                                                                                              \
        ti.name.text = name;                                                                                                                        \
        ti.name.size = strlen(name);                                                                                                                \
        ti.type = dt;                                                                                                                               \
        ti.flags = flags;                                                                                                                           \
        if (vmin >= vmax) ti.flags |= DTF_READ_ONLY;                                                                                                \
        ti.value_size = sizeof(T);                                                                                                                  \
        ti.value = v;                                                                                                                               \
        ti.ctx.value_def = *v;                                                                                                                      \
        ti.ctx.value_min = vmin;                                                                                                                    \
        ti.ctx.value_max = vmax;                                                                                                                    \
        ti.cb = cb;                                                                                                                                 \
        ti.context = context;                                                                                                                       \
        xDebugAddTweak(ti);                                                                                                                         \
    }
#define ADD_TWEAK(T, dt, ctx) ADD_TWEAK_L(T, dt, ctx, __LINE__)

ADD_TWEAK_L(S32, DT_INT, int_context, 3673)
ADD_TWEAK_L(S16, DT_INT, int_context, 3674)
ADD_TWEAK_L(S8, DT_INT, int_context, 3675)
ADD_TWEAK_L(U32, DT_UINT, uint_context, 3676)
ADD_TWEAK_L(U16, DT_UINT, uint_context, 3677)
ADD_TWEAK_L(U8, DT_UINT, uint_context, 3678)
ADD_TWEAK_L(F32, DT_FLOAT, float_context, 3679)

#undef ADD_TWEAK
#undef ADD_TWEAK_L

void xDebugAddTweak(const char* name, size_t* v, size_t vmin, size_t vmax, const tweak_callback* cb, void* context, U32 flags)
{
    xASSERT_L(3684, v != 0);
    tweak_info ti;
    ti.name.text = name;
    ti.name.size = strlen(name);
    ti.type = DT_UINT;
    ti.flags = flags;
    if (vmin >= vmax) ti.flags |= DTF_READ_ONLY;

    ti.value_size = sizeof(size_t); ti.value = v;
    ti.uint_context.value_def = *v;
    ti.uint_context.value_min = vmin;
    ti.uint_context.value_max = vmax;
    ti.cb = cb;
    ti.context = context;
    xDebugAddTweak(ti);
}

#define ADD_SELECT_TWEAK_L(T, line)                                                                                                                                                                   \
    void xDebugAddSelectTweak(const char* name, T* v, const char** labels, const T* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags)                                      \
    {                                                                                                                                                                                                 \
        xASSERT_L(line, v != 0);                                                                                                                                                                      \
        xASSERT_L(line, (labels_size <= 0) || (labels != 0));                                                                                                                                         \
        tweak_info ti;                                                                                                                                                                                \
        ti.name.text = name;                                                                                                                                                                          \
        ti.name.size = strlen(name);                                                                                                                                                                  \
        ti.type = DT_SELECT;                                                                                                                                                                          \
        ti.flags = flags;                                                                                                                                                                             \
        if (labels_size <= 1) ti.flags |= DTF_READ_ONLY;                                                                                                                                              \
        ti.value_size = sizeof(T);                                                                                                                                                                    \
        ti.value = v;                                                                                                                                                                                 \
        switch (sizeof(T)) {                                                                                                                                                                          \
        case 1: ti.select_context.value_def = *(U8*)v; break;                                                                                                                                         \
        case 2: ti.select_context.value_def = *(U16*)v; break;                                                                                                                                        \
        case 4: ti.select_context.value_def = *(U32*)v; break;                                                                                                                                        \
        default: xFAIL_L(line);                                                                                                                                                                       \
        }                                                                                                                                                                                             \
        ti.select_context.labels = labels;                                                                                                                                                            \
        ti.select_context.values = (void*)values;                                                                                                                                                     \
        ti.select_context.labels_size = labels_size;                                                                                                                                                  \
        ti.cb = cb;                                                                                                                                                                                   \
        ti.context = context;                                                                                                                                                                         \
        xDebugAddTweak(ti);                                                                                                                                                                           \
    }
#define ADD_SELECT_TWEAK(T) ADD_SELECT_TWEAK_L(T, __LINE__)

ADD_SELECT_TWEAK_L(S32, 3734)
ADD_SELECT_TWEAK_L(S8, 3736)
ADD_SELECT_TWEAK_L(U32, 3737)
ADD_SELECT_TWEAK_L(U16, 3738)
ADD_SELECT_TWEAK_L(U8, 3739)
ADD_SELECT_TWEAK_L(F32, 3740)

#undef ADD_SELECT_TWEAK
#undef ADD_SELECT_TWEAK_L

#define ADD_FLAG_TWEAK_L(T, line)                                                                                                                   \
    void xDebugAddFlagTweak(const char* name, T* v, T mask, const tweak_callback* cb, void* context, U32 flags)                                     \
    {                                                                                                                                               \
        xASSERT_L(line, v != 0);                                                                                                                    \
        tweak_info ti;                                                                                                                              \
        ti.name.text = name;                                                                                                                        \
        ti.name.size = strlen(name);                                                                                                                \
        ti.type = DT_FLAG;                                                                                                                          \
        ti.flags = flags;                                                                                                                           \
        if (!mask) ti.flags |= DTF_READ_ONLY;                                                                                                       \
        ti.value_size = sizeof(T);                                                                                                                  \
        ti.value = v;                                                                                                                               \
        ti.flag_context.value_def = *v;                                                                                                             \
        ti.flag_context.mask = mask;                                                                                                                \
        ti.cb = cb;                                                                                                                                 \
        ti.context = context;                                                                                                                       \
        xDebugAddTweak(ti);                                                                                                                         \
    }
#define ADD_FLAG_TWEAK(T) ADD_FLAG_TWEAK_L(T, __LINE__)

ADD_FLAG_TWEAK_L(U32, 3765)
ADD_FLAG_TWEAK_L(U8, 3767)

#undef ADD_FLAG_TWEAK
#undef ADD_FLAG_TWEAK_L

void xDebugAddTweak(const char* name, xVec3* v, const tweak_callback* cb, void* context, U32 flags)
{
    char buffer[128];
    sprintf(buffer, "%.*sX", ARRAY_LENGTH(buffer) - 2, name);
    xDebugAddTweak(buffer, &v->x, -1000000000.0f, 1000000000.0f, cb, context, flags | DTF_BUFFER);
    sprintf(buffer, "%.*sY", ARRAY_LENGTH(buffer) - 2, name);
    xDebugAddTweak(buffer, &v->y, -1000000000.0f, 1000000000.0f, cb, context, flags | DTF_BUFFER);
    sprintf(buffer, "%.*sZ", ARRAY_LENGTH(buffer) - 2, name);
    xDebugAddTweak(buffer, &v->z, -1000000000.0f, 1000000000.0f, cb, context, flags | DTF_BUFFER);
}

void xDebugAddTweak(const char* name, xColor* v, const tweak_callback* cb, void* context, U32 flags)
{
    char buffer[128];
    sprintf(buffer, "%.*s\001Red", ARRAY_LENGTH(buffer) - 6, name);
    xDebugAddTweak(buffer, &v->r, 0, 255, cb, context, flags | DTF_BUFFER);
    sprintf(buffer, "%.*s\002Green", ARRAY_LENGTH(buffer) - 6, name);
    xDebugAddTweak(buffer, &v->g, 0, 255, cb, context, flags | DTF_BUFFER);
    sprintf(buffer, "%.*s\003Blue", ARRAY_LENGTH(buffer) - 6, name);
    xDebugAddTweak(buffer, &v->b, 0, 255, cb, context, flags | DTF_BUFFER);
    sprintf(buffer, "%.*sAlpha", ARRAY_LENGTH(buffer) - 6, name);
    xDebugAddTweak(buffer, &v->a, 0, 255, cb, context, flags | DTF_BUFFER);
}

void xDebugAddBranchTweak(const char* name, const tweak_callback* cb, void* context, U32 flags)
{
    tweak_info ti;
    ti.name.text = name;
    ti.name.size = strlen(name);
    ti.type = DT_BRANCH;
    ti.flags = flags;
    ti.value = NULL;
    ti.value_size = 0;
    ti.cb = cb;
    ti.context = context;
    xDebugAddTweak(ti);
}

void xDebugAddTweak(const char* name, bool* v, const tweak_callback* cb, void* context, U32 flags)
{
    xASSERT_L(3815, v != 0);
    tweak_info ti;
    ti.name.text = name;
    ti.name.size = strlen(name);
    ti.type = DT_BOOL;
    ti.flags = flags;
    ti.value = v;
    ti.value_size = sizeof(bool);
    ti.bool_context.value_def = *v;
    ti.cb = cb;
    ti.context = context;
    xDebugAddTweak(ti);
}

void xDebugAddTweak(const char* name, const char* message, const tweak_callback* cb, void* context, U32 flags)
{
    tweak_info ti;
    ti.name.text = name;
    ti.name.size = strlen(name);
    ti.type = DT_MESSAGE;
    ti.flags = flags;
    if (!cb || !cb->on_change) ti.flags |= DTF_READ_ONLY;
    ti.value = (void*)message;
    ti.value_size = 0;
    ti.cb = cb;
    ti.context = context;
    xDebugAddTweak(ti);
}

static char* create_auto_name(const char* prefix, const char* name, bool process)
{
    char delim[] = ".:->";
    static char buffer[128], buffer2[128];
    if (!process) {
        sprintf(buffer, "%s%s", prefix, name);
        return buffer;
    }

    char* tail = buffer;
    const char* s = name;
    bool lastrep = false;
    while (*s) {
        size_t offset = strcspn(s, delim);
        if (offset) {
            memcpy(tail, s, offset);
            tail += offset;
            s += offset;
            lastrep = false;
        } else {
            if (!lastrep) {
                *tail = '|'; lastrep = true;
                tail++;
            }
            s++;
        }
    }
    *tail = '\0';

    char* headstr = buffer;
    char* bs = buffer;
    while (*bs) {
        char c = *bs;
        bs++;
        if (c == '|' || c == ')') {
            S32 wsfound = 0;
            char* ws = bs;
            for (;;) {
                c = *ws;
                if (c == ' ' || c == '\t') wsfound++;
                else break;
                ws++;
            }
            if (wsfound > 0)
                headstr = ws;
        }
        else if (!c) break;
    }

    strcpy(buffer2, prefix);
    strcat(buffer2, headstr);

    return buffer2;
}

void _auto_tweak_branch(const char* prefix, const char* name, const tweak_callback* cb, void* context, U32 flags)
{
    char* title = create_auto_name(prefix, name, false);
    size_t len = strlen(title);
    if (len && title[len - 1] == '|')
        title[len - 1] = '\0';
    xDebugAddBranchTweak(title, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, S32* v, S32 vmin, S32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, vmin, vmax, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, S16* v, S16 vmin, S16 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, vmin, vmax, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, U32* v, U32 vmin, U32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, vmin, vmax, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, U16* v, U16 vmin, U16 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, vmin, vmax, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, U8* v, U8 vmin, U8 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, vmin, vmax, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, F32* v, F32 vmin, F32 vmax, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, vmin, vmax, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak_select(const char* prefix, const char* name, U32* v, const char** labels, const U32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddSelectTweak(title, v, labels, values, labels_size, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak_select(const char* prefix, const char* name, U8* v, const char** labels, const U8* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddSelectTweak(title, v, labels, values, labels_size, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak_select(const char* prefix, const char* name, S32* v, const char** labels, const S32* values, U32 labels_size, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddSelectTweak(title, v, labels, values, labels_size, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak_flag(const char* prefix, const char* name, U32* v, U32 mask, const tweak_callback* cb, void* context, U32 flags)
{
    const char* title = create_auto_name(prefix, name, false);
    xDebugAddFlagTweak(title, v, mask, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak_flag(const char* prefix, const char* name, U8* v, U8 mask, const tweak_callback* cb, void* context, U32 flags)
{
    const char* title = create_auto_name(prefix, name, false);
    xDebugAddFlagTweak(title, v, mask, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, xVec3* v, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, xColor* v, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, bool* v, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, v, cb, context, flags | DTF_BUFFER);
}

void _auto_tweak(const char* prefix, const char* name, const char* message, const tweak_callback* cb, void* context, U32 flags, bool process)
{
    const char* title = create_auto_name(prefix, name, process);
    xDebugAddTweak(title, message, cb, context, flags | DTF_BUFFER);
}

void tweak_rtod(const tweak_info&, void* value)
{
    F32& f = *(F32*)value;
    f = (180.0f / PI) * f;
}

void tweak_dtor(const tweak_info&, void* value)
{
    F32& f = *(F32*)value;
    f = (PI / 180.0f) * f;
}

const tweak_callback cb_tweak_convert_degrees = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tweak_rtod, tweak_dtor };

void tweak_acos(const tweak_info&, void* value)
{
    F32& f = *(F32*)value;
    f = (180.0f / PI) * xacos(f);
}

void tweak_cos(const tweak_info&, void* value)
{
    F32& f = *(F32*)value;
    f = icos((PI / 180.0f) * f);
}

const tweak_callback cb_tweak_convert_cos = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tweak_acos, tweak_cos };

namespace {
    template <class T, class CB>
    void force_change(const tweak_info& info, T offset, CB cb)
    {
        T v, cv;
        debug_tweak::get_value(info, v, NULL);
        debug_tweak::get_value(info, cv, info.context);
        cv += offset;
        T nv = cb(v, cv);
        if (nv != v) debug_tweak::set_value(info, nv, NULL);
    }

    template <class T>
    struct cb_get_min
    {
        T operator()(T v, T cv) const { return MIN(v, cv); }
    };

    template <class T>
    struct cb_get_max
    {
        T operator()(T v, T cv) const { return MAX(v, cv); }
    };
}

#define TWEAK_FORCE(cb, ioffset, foffset)                                                         \
    switch (info.type) {                                                                          \
    case DT_INT: force_change(info, (S32)(ioffset), cb<S32>()); break;                            \
    case DT_UINT: force_change(info, (U32)(ioffset), cb<U32>()); break;                           \
    case DT_FLOAT: force_change(info, (F32)(foffset), cb<F32>()); break;                          \
    }

void tweak_force_lt(const tweak_info& info)
{
    TWEAK_FORCE(cb_get_min, 1, -0.001f);
}

void tweak_force_gt(const tweak_info& info)
{
    TWEAK_FORCE(cb_get_max, -1, 0.001f);
}

void tweak_force_lteq(const tweak_info& info)
{
    TWEAK_FORCE(cb_get_min, 0, 0.0f);
}

void tweak_force_gteq(const tweak_info& info)
{
    TWEAK_FORCE(cb_get_max, 0, 0.0f);
}

#undef TWEAK_FORCE

const tweak_callback cb_tweak_force_lt = { tweak_force_lt };
const tweak_callback cb_tweak_force_gt = { tweak_force_gt };
const tweak_callback cb_tweak_force_lteq = { tweak_force_lteq };
const tweak_callback cb_tweak_force_gteq = { tweak_force_gteq };

const char* tweak_blend_mode_labels[11] =
{
    "ZERO",
    "ONE",
    "SRCCOLOR",
    "INVSRCCOLOR",
    "SRCALPHA",
    "INVSRCALPHA",
    "DESTALPHA",
    "INVDESTALPHA",
    "DESTCOLOR",
    "INVDESTCOLOR",
    "SRCALPHASAT"
};

const U32 tweak_blend_mode_values[11] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
