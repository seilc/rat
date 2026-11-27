#ifndef ZTASKBOX_H
#define ZTASKBOX_H

#include "xBase.h"
#include "zTalkBox.h"

struct xDynAsset;
struct xSerial;

struct ztaskbox : xBase
{
    struct asset_type;
    class talk_callback;

    enum state_enum
    {
        STATE_INVALID = -1,
        STATE_BEGIN,
        STATE_DESCRIPTION,
        STATE_REMINDER,
        STATE_SUCCESS,
        STATE_FAILURE,
        STATE_END,
        MAX_STATE
    };

    class callback
    {
    public:
        inline virtual void on_talk_start(); // UNUSED
        inline virtual void on_talk_stop(); // UNUSED
    };

    static talk_callback* tcb;

    void load(const asset_type& a);
    void read(xSerial& s);
    void write(xSerial& s);
    void start_talk(zNPC::zNPCBaseIN2* npc);
    void stop_talk();
    void enable();
    void disable();
    bool enabled(); // UNUSED
    void reset();
    void initiate();
    void succeed();
    void fail();
    void complete();
    void set_callback(callback* cb);
    callback* get_callback(); // UNUSED
    bool exists(state_enum state);
    void set_state(state_enum state);
    bool is_running() const; // UNUSED
    void on_talk_start();
    void on_talk_stop(ztalkbox::answer_enum);

    static void init();
    static void load(xBase& data, xDynAsset& asset, size_t asset_size);
    static const char* get_text(U32 id);
    static void cb_dispatch(xBase*, xBase* to, U32 event, const F32*, xBase*, U32);

private:
    struct
    {
        U8 enabled : 8;
        U8 running : 8;
        U32 dummy : 16;
    } flag;
    const asset_type* asset;
    state_enum state;
    callback* cb;
    ztaskbox* current;
};

#endif
