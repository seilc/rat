#ifndef ZTALKBOX_H
#define ZTALKBOX_H

#include "xBase.h"

namespace zNPC {
class zNPCBaseIN2;
}

struct ztextbox;

struct ztalkbox : xBase
{
    struct asset_type;

    enum answer_enum
    {
        ANSWER_CONTINUE,
        ANSWER_YES,
        ANSWER_NO
    };

    struct callback
    {
        virtual void on_signal(U32) {}
        virtual void on_start() {}
        virtual void on_stop() {}
        virtual void on_answer(answer_enum) {}
    };

    struct
    {
        bool visible : 1;
    } flag;
    const asset_type* asset;
    ztextbox* dialog_box;
    ztextbox* prompt_box;
    ztextbox* quit_box;
    struct
    {
        const char* skip;
        const char* noskip;
        const char* quit;
        const char* noquit;
        const char* yesno;
    } prompt;
    U32 triggerPads;
    zNPC::zNPCBaseIN2* npc;

    void start_talk(const char* s, callback* cb, zNPC::zNPCBaseIN2* npc);
    void stop_talk();
};

#endif
