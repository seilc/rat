#include "xCommon.h"
#include "zAnimCustom.h"

#include "xAnim.h"
#include "xModel.h"

#include <string.h>

struct zAnimCustomFile
{
    zAnimCustomFile* next;
    U32 id;
    xAnimFile* data;
};

static S32 sAnimCustomCount;
static S32 sMaxTempTransCount;
static zAnimCustomFile* sAnimCustomList;
static zAnimCustomFile* sAnimCustomEndOfList;
static xMemPool szAnimCustomTempTranListPool;
static xAnimTransitionList** zAnimCustomActiveTempTrans;
static S32 sLastIndex;
static char* sCustomStateName = "AnimCustomState";
static xAnimState sDummyState;
static xAnimTransition sDummyTran;
static xAnimTransitionList sDummyList;

static U32 TempTransCB(xAnimTransition* tran, xAnimSingle*, void*)
{
    tran->UserFlags = 2;
    return 1;
}

static inline void AddToActiveArray(xAnimTransitionList* l)
{
    xASSERT_L(43, sLastIndex < (sMaxTempTransCount - 1));

    sLastIndex++;
    zAnimCustomActiveTempTrans[sLastIndex] = l;
}

static inline void DeleteFromActiveArray(S32 index)
{
    xASSERT_L(51, index >= 0 && index <= sLastIndex);
    xAnimTempTransitionFree(zAnimCustomActiveTempTrans[index]->T);
    xMemPoolFree(&szAnimCustomTempTranListPool, zAnimCustomActiveTempTrans[index]);
    zAnimCustomActiveTempTrans[index] = zAnimCustomActiveTempTrans[sLastIndex--];
}

static inline void ClearActiveArray()
{
    // BUG: leaves entry 0
    while (sLastIndex > 0)
        DeleteFromActiveArray(0);
}

void zAnimCustomInit(S32 count)
{
    sAnimCustomList = NULL;
    sAnimCustomEndOfList = NULL;
    sAnimCustomCount = 0;
    sMaxTempTransCount = count;

    memset(&szAnimCustomTempTranListPool, 0, sizeof(xMemPool));
    xMemPoolSetup(&szAnimCustomTempTranListPool, xMEMALLOC_L(sMaxTempTransCount * sizeof(xAnimTransitionList), 0, eMemMgrTag_AnimCustom, 0, 71), 0, 0, NULL, sizeof(xAnimTransitionList), sMaxTempTransCount, 0);

    zAnimCustomActiveTempTrans = (xAnimTransitionList**)xMEMALLOC_L(sMaxTempTransCount * sizeof(xAnimTransitionList*), 0, eMemMgrTag_AnimCustom, 0, 73);
    sLastIndex = -1;

    memset(&sDummyState, 0, sizeof(xAnimState));
    sDummyState.Name = "DUMMY";
    sDummyState.Speed = 1.0f;

    memset(&sDummyTran, 0, sizeof(xAnimTransition));
    sDummyTran.Dest = &sDummyState;
    sDummyTran.Flags = AnimTransition_Default;

    memset(&sDummyList, 0, sizeof(xAnimTransitionList));
    sDummyList.T = &sDummyTran;
}

void zAnimCustomClear()
{
    sAnimCustomCount = 0;
    sMaxTempTransCount = 0;

    sAnimCustomList = NULL;

    ClearActiveArray();
}

void zAnimCustomReset()
{
    ClearActiveArray();
}

void zAnimCustomAdd(U32 id, xAnimFile* data, bool hasPhysics)
{
    if (!data) {
        return;
    }

    if (hasPhysics) {
        data->FileFlags |= 0x50000;
        xAnimCalcPhysicsData(data, data);
    }

    zAnimCustomFile* entry = (zAnimCustomFile*)xMEMALLOC_L(sizeof(zAnimCustomFile), 0, eMemMgrTag_AnimCustom, 0, 115);
    entry->id = id;
    entry->data = data;
    entry->next = NULL;

    if (sAnimCustomEndOfList) {
        sAnimCustomEndOfList->next = entry;
        sAnimCustomEndOfList = entry;
    } else {
        sAnimCustomList = entry;
        sAnimCustomEndOfList = entry;
    }

    sAnimCustomCount++;
}

void zAnimCustomPlay(U32 id, xModelInstance* model, U32 outStateID, bool loopAnim, F32 inBlend, F32 outBlend,
                     F32 speed)
{
    zAnimCustomFile* file = sAnimCustomList;
    while (file) {
        if (file->id == id) {
            if (strcmp(model->Anim->Single->State->Name, sCustomStateName) == 0) {
                for (S32 i = 0; i <= sLastIndex; i++) {
                    if (zAnimCustomActiveTempTrans[i] == model->Anim->Single->State->Default) {
                        DeleteFromActiveArray(i);
                        break;
                    }
                }
            }

            xAnimState state;
            state.Next = NULL;
            state.Name = sCustomStateName;
            state.ID = id;
            state.Flags = AnimState_CopyOnWrite;
            if (loopAnim)
                state.Flags |= AnimState_EndLoop;
            else
                state.Flags |= outStateID ? AnimState_EndTransition : AnimState_EndStop;

            state.UserFlags = 0;
            state.Speed = speed;
            state.Data = file->data;
            state.Effects = NULL;
            state.Default = NULL;
            state.List = NULL;
            state.BoneBlend = NULL;
            state.TimeSnap = NULL;
            state.FadeRecip = 0.0f;
            state.FadeOffset = NULL;
            state.MultiFile = NULL;
            state.CallbackData = NULL;
            state.BeforeEnter = NULL;
            state.StateCallback = NULL;
            state.BeforeAnimMatrices = NULL;

            if (outStateID) {
                xAnimTransition* tran = xAnimTempTransitionAlloc(NULL);
                tran->Dest = xAnimTableGetStateID(model->Anim->Table, outStateID);
                xASSERT_L(207, tran->Dest);

                tran->Conditional = NULL;
                tran->Callback = TempTransCB;
                tran->Flags = AnimTransition_Default;
                tran->UserFlags = 1;
                tran->SrcTime = 0.0f;
                tran->DestTime = 0.0f;
                tran->Priority = 1;
                tran->QueuePriority = 0;
                tran->BlendRecip = outBlend == 0.0f ? 0.0f : 1.0f / outBlend;
                tran->BlendOffset = NULL;

                xAnimTransitionList* tempList = (xAnimTransitionList*)xMemPoolAlloc(&szAnimCustomTempTranListPool, eMemMgrTag_AnimCustom);
                AddToActiveArray(tempList);

                tempList->T = tran;
                tempList->Next = NULL;

                state.Default = tempList;
            } else {
                state.Default = &sDummyList;
            }

            xAnimTransition* transition = xAnimTempTransitionAlloc(NULL);
            transition->BlendRecip = inBlend == 0.0f ? 0.0f : 1.0f / inBlend;
            transition->Dest = &state;
            xAnimPlayStartTransition(model->Anim, transition);
            return;
        }

        file = file->next;
    }

    xASSERT_FMT_L(241, 0, "Requested custom animation not found.");
}

void zAnimCustomUpdate()
{
    S32 currIndex = 0;
    while (currIndex <= sLastIndex) {
        if (zAnimCustomActiveTempTrans[currIndex]->T->UserFlags == 2) {
            DeleteFromActiveArray(currIndex);
        } else
            currIndex++;
    }
}
