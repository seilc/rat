#include "xCommon.h"
#include "zTaskBox.h"

#include "xDynAsset.h"
#include "xEvent.h"
#include "xGroup.h"
#include "xserializer.h"
#include "xTextAsset.h"
#include "zBase.h"
#include "zEvent.h"
#include "zScene.h"

struct ztaskbox::asset_type : xDynAsset
{
    enum { VERSION = 2 };

    U8 persistent : 8;
    U8 loop : 8;
    U8 enable : 8;
    U8 retry : 8;
    U32 talk_box;
    U32 next_task;
    U32 stages[MAX_STATE];

    static const char* type_name() { return "game_object:task_box"; }
};

ztaskbox::talk_callback* ztaskbox::tcb;

namespace {

struct
{
    ztaskbox* active;
} shared;

} // namespace

class ztaskbox::talk_callback : public ztalkbox::callback
{
public:
    void reset(ztaskbox& task)
    {
        this->task = &task;
        answer = ztalkbox::ANSWER_CONTINUE;
    }

    virtual void on_start()
    {
        xASSERT_L(44, task != 0);
        task->on_talk_start();
    }

    virtual void on_stop()
    {
        xASSERT_L(49, task != 0);
        task->on_talk_stop(answer);
    }

    virtual void on_answer(ztalkbox::answer_enum answer)
    {
        this->answer = answer;
    }

    ztaskbox* task;
    ztalkbox::answer_enum answer;
};

void ztaskbox::load(const asset_type& a)
{
    xBaseInit(this, &a);
    baseType = eBaseTypeTaskBox;
    asset = &a;
    eventFunc = cb_dispatch;
    if (linkCount) link = (const xLinkAsset*)(&a + 1);
    state = STATE_INVALID;
    if (a.enable) {
        flag.enabled = 1;
        set_state(STATE_BEGIN);
        current = this;
    } else {
        flag.enabled = 0;
        current = NULL;
    }
    flag.running = 0;
    if (a.persistent) baseFlags |= k_XBASE_IS_PERSISTENT;
}

void ztaskbox::read(xSerial& s)
{
    U8 state = this->state;
    s.Read(&state);
    set_state((state_enum)state);
}

void ztaskbox::write(xSerial& s)
{
    s.Write((U8)state);
}

void ztaskbox::start_talk(zNPC::zNPCBaseIN2* npc)
{
    if (!current) return;
    if (current != this) {
        current->set_callback(cb);
        current->start_talk(NULL);
        return;
    }

    if (!flag.enabled || state == STATE_INVALID) return;
    if (shared.active && shared.active != this)
        shared.active->stop_talk();

    xASSERT_L(118, asset != 0);

    ztalkbox* talk = (ztalkbox*)zSceneFindObject(asset->talk_box);
    if (!talk) return;
    xASSERT_L(122, talk->baseType == eBaseTypeTalkBox);

    const char* s = get_text(asset->stages[state]);
    if (!s) return;

    shared.active = this;

    tcb->reset(*this);
    talk->start_talk(s, tcb, npc);
}

void ztaskbox::stop_talk()
{
    if (!current) return;
    if (current != this) { current->stop_talk(); return; }
    if (!flag.enabled || state == STATE_INVALID) return;
    if (shared.active != this) return;

    xASSERT_L(139, asset != 0);

    ztalkbox* talk = (ztalkbox*)zSceneFindObject(asset->talk_box);
    if (!talk) return;
    xASSERT_L(143, talk->baseType == eBaseTypeTalkBox);

    talk->stop_talk();

    shared.active = NULL;
}

void ztaskbox::enable()
{
    if (flag.enabled) return;
    flag.enabled = 1;
    set_state(STATE_BEGIN);
}

void ztaskbox::disable()
{
    if (!flag.enabled) return;
    stop_talk();
    flag.enabled = 0;
    set_state(STATE_INVALID);
}

void ztaskbox::reset()
{
    xASSERT_L(174, asset != 0);
    disable();
    if (asset->enable) enable();
}

void ztaskbox::initiate()
{
    if (state == STATE_BEGIN)
        set_state(STATE_DESCRIPTION);
}

void ztaskbox::succeed()
{
    if (state == STATE_BEGIN || state == STATE_DESCRIPTION ||
        state == STATE_REMINDER)
        set_state(STATE_SUCCESS);
}

void ztaskbox::fail()
{
    if (state == STATE_BEGIN || state == STATE_DESCRIPTION ||
        state == STATE_REMINDER)
        set_state(STATE_FAILURE);
}

void ztaskbox::complete()
{
    if (!flag.enabled) return;
    state = STATE_INVALID;
    flag.enabled = 0;
    zEntEvent(this, this, eEventTaskBox_OnComplete);

    current = (ztaskbox*)zSceneFindObject(asset->next_task);
    if (!current) return;
    xASSERT_L(209, current->baseType == eBaseTypeTaskBox);
}

void ztaskbox::set_callback(callback* cb)
{
    this->cb = cb;
}

void ztaskbox::init()
{
    shared.active = NULL;
    static talk_callback tcb;
    ztaskbox::tcb = &tcb;
}

void ztaskbox::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(231, asset, asset_size, ztaskbox::asset_type, ztaskbox::asset_type::type_name());
    ((ztaskbox&)data).load((const asset_type&)asset);
}

bool ztaskbox::exists(state_enum state)
{
    U32 id = asset->stages[state];
    return id && xSTFindAsset(id, NULL);
}

void ztaskbox::set_state(state_enum state)
{
    xASSERT_L(243, asset != 0);
    this->state = state;
    current = this;

    switch (state) {
    case STATE_BEGIN:
        if (!exists(state))
            set_state(STATE_DESCRIPTION);
        break;
    case STATE_DESCRIPTION:
        if (!exists(state))
            set_state(STATE_REMINDER);
        break;
    case STATE_REMINDER:
    case STATE_SUCCESS:
    case STATE_FAILURE:
        if (!exists(state))
            set_state(STATE_END);
        break;
    case STATE_END:
        if (!exists(state))
            set_state(STATE_INVALID);
        break;
    default:
        complete();
        break;
    }
}

void ztaskbox::on_talk_start()
{
    flag.running = 1;
    if (cb)
        cb->on_talk_start();
}

void ztaskbox::on_talk_stop(ztalkbox::answer_enum)
{
    xASSERT_L(292, asset != 0);
    flag.running = 0;

    switch (state) {
    case STATE_DESCRIPTION:
        set_state(STATE_REMINDER);
        break;
    case STATE_SUCCESS:
        set_state(STATE_END);
        break;
    case STATE_FAILURE:
        if (asset->retry) set_state(STATE_DESCRIPTION);
        else set_state(STATE_END);
        break;
    case STATE_BEGIN:
    case STATE_REMINDER:
    case STATE_END:
        break;
    default:
        if (asset->loop) set_state(STATE_BEGIN);
        break;
    }

    if (cb) cb->on_talk_stop();
}

const char* ztaskbox::get_text(U32 id)
{
    xGroup* text_group = (xGroup*)zSceneFindObject(id);
    if (text_group) {
        if (text_group->baseType != eBaseTypeGroup) return NULL;
        id = xGroupGetAny(text_group);
    }

    if (!id) return NULL;
    const char* text = xTextFindString(id, NULL);
    if (text == NULL) return NULL;
    else return text;
}

void ztaskbox::cb_dispatch(xBase*, xBase* to, U32 event, const F32*, xBase*, U32)
{
    xASSERT_L(337, to != 0);
    ztaskbox& e = *(ztaskbox*)to;
    xASSERT_L(339, e.baseType == eBaseTypeTaskBox);

    switch (event) {
    case eEventReset:
    case eEventDebugReset:
        e.reset();
        break;
    case eEventStartConversation:
        e.start_talk(NULL);
        break;
    case eEventEndConversation:
        e.stop_talk();
        break;
    case eEventTaskBox_Initiate:
        e.initiate();
        break;
    case eEventTaskBox_SetSuccess:
        e.succeed();
        break;
    case eEventTaskBox_SetFailure:
        e.fail();
        break;
    case eEventTaskBox_OnAccept:
    case eEventTaskBox_OnDecline:
    case eEventTaskBox_OnComplete:
        break;
    }
}
