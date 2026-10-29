#ifndef XTRC_H
#define XTRC_H

#include "types.h"
#include "iTRC.h"

#define k_MAX_TRC_PADS 4

typedef enum _tagTRCState
{
    TRC_Unknown,
    TRC_PadMissing,
    TRC_PadInserted,
    TRC_PadActive,
    TRC_PadPreInserted,
    TRC_PadInvalidNoAnalog,
    TRC_PadInvalidType,
    TRC_DiskNotIdentified,
    TRC_DiskIdentified,
    TRC_DiskTrayOpen,
    TRC_DiskTrayClosed,
    TRC_DiskNoDisk,
    TRC_DiskInvalid,
    TRC_DiskRetry,
    TRC_DiskFatal,
    TRC_Total
} TRCState;

typedef enum _tagTRCPadState
{
    TRC_PadStateUnknown,
    TRC_PadStatePresent,
    TRC_PadStateMissingDuringGameplay,
    TRC_PadStateMissingDuringCutscene,
    TRC_PadStateMissingDuringMainMenu,
    TRC_PadStateMissingDuringMovie,
    TRC_PadStateMissingDuringLoadSave
} TRCPadState;

struct _tagTRCPadInfo : _tagiTRCPadInfo
{
    S32 id;
    TRCState state;
    TRCPadState padstate;
};

struct _tagTRCDiskInfo
{
    TRCState state;
    TRCState id;
};

TRCState xTRCDiskGetState();
U32 xTRCDiskIsIDed();
bool xTRCMemCardCheck();
_tagTRCPadInfo& xTRCPadGetNextAvailablePad();
S32 xTRCPadGetArrayIdx(S32 port);
_tagTRCPadInfo& xTRCPadGetPadFromID(S32 id);
void xTRCPadRemove(S32 port);
void xTRCPadRestoreAll();
void xTRCPadSetState(S32 idx, TRCState state);
S32 xTRCPadCount(TRCState state);
S32 xTRCPadCount(TRCPadState padstate);
S32 xTRCPadCountWereActiveTotal();
void xTRCPadSetStatePadPort(S32 port, TRCState state);
void xTRCPadSetPadStatePort(S32 port, TRCPadState state);
bool xTRCPadFindFirst(S32& idx, TRCState state);
bool xTRCPadFindFirstPadState(S32& idx, TRCPadState state);
bool xTRCPadIsIdx(S32 idx, TRCState state);
S32 xTRCPadGetPadPort(S32 idx);
TRCState xTRCPadGetStateIdx(S32 idx);
TRCState xTRCPadGetStatePort(S32 port);
TRCPadState xTRCPadGetPadStatePort(S32 port);
TRCPadState xTRCPadGetPadStateIdx(S32 idx);
bool xTRCPadIsPort(S32 port, TRCState state);
bool xTRCPadGetPadPort(S32& idx, TRCState state);
bool xTRCPadPortWasActive(S32 port);
void xTRCPadReOrder();
S32 xTRCPadGetPort_Player1();
S32 xTRCPadGetPort_Player2();
void xTRCUpdate(S32 i);
void xTRCPadInsert(S32 i);
void xTRCInit();
void xTRCInitMessages();
void xTRCClearPadMsg();
void xTRCRender();
void xTRCReset();
void xTRCPadGetPressed(TRCState padState, bool& A, bool& B, bool& C, bool& D);
void xTRCPadSetPressed(S32 port, S32 value, bool reverse);
void xTRCPadDisplayMessage(S32 pad_id, TRCState);
void xTRCDisk(TRCState state);
void xTRCProcessStateStandalone();
void xTRCProcessState();
#if DEBUG
void xTRCStateToStr(TRCState state, char* txt);
void xTRCDebugCB();
#endif
void render_mem_card_no_space(S32 needed, S32 available, S32 neededFiles, bool enabled);
void RenderText(const char* text, bool enabled);

#endif
