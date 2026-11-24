#ifndef ZMINIME_H
#define ZMINIME_H

#include "xPad.h"

struct xResponseCurve;

struct curve_node
{
    F32 t;
    F32 val;
};

struct zMinigameActionBase
{
    S32 type;
    F32 time;
    S32 flags;
};

struct zMinigameEvent : zMinigameActionBase
{
    U32 toWidgetID;
    U32 event;
    F32 params[4];
};

struct zMinigameInput : zMinigameActionBase
{
    U32 input;
    F32 frequency;
    F32 duration;
    U32 curveID;
};

struct zMinigameText : zMinigameActionBase
{
    U32 hintTextID;
    U32 textBoxID;
};

typedef union _zMinigameActions
{
    zMinigameEvent event;
    zMinigameInput input;
    zMinigameText text;
} zMinigameActions;

class zMinigame
{
    F32 time;
    F32 score;
    zMinigameActions* current_action;
    U32 replyToID;
    bool groupAnd;
    bool groupOr;
    S32 groupType;

public:
    U32 minigameID;
    U32 active : 1;
    U32 type : 7;
    U32 stalled : 1;
    U32 flashy : 1;
    U32 padding : 22;
    U32 cameraID;
    zMinigameActions* actions;
    U32 action_count;
};

struct PadValue
{
    F32 t;
    U32 on;
    xPad::analog_data stick[2];
};

class PadHistory
{
    PadValue value[300];
    PadValue* input;
    bool isStirring;
    F32 stirringFrequency;
    F32 stirringDuration;

public:
    bool IsStirring(F32& frequency, F32& duration);
};

class zMiniME
{
public:
    xResponseCurve* curves;
    U32* curveIDs;
    S32 curve_count;
    curve_node* curvepoints;
    zMinigame* minigames;
    S32 minigame_count;
    zMinigameActions* actions;
    PadHistory history;

    static zMiniME& GetMgr();
};

#endif
