#ifndef ZCONVERSATION_H
#define ZCONVERSATION_H

#include "xBase.h"
#include "xJaw.h"
#include "xSnd.h"
#include "zBrainTypes.h"
#include "zConversationAsset.h"

class zConversation : public xBase
{
public:
    zConversation(const zConversationAsset& myAsset);

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    void HandleEvent(xBase*, U32 toEvent, const F32*, xBase* toParamWidget, U32);
    xSndGroupHandle GetSoundGroup() { return soundGroup; }

protected:
    const zConversationAsset& asset;
    xSndGroupHandle soundGroup;
};

void zConversationInit();
xJawSpeaker zConversationGetSpeaker(eBrainType brainType); // UNUSED
F32 zConversationGetJaw(xJawSpeaker speaker);
bool zConversationIsConversing();
void zConversationPlay(xSndGroupHandle soundGroup, const xBase* otherSpeakerBase);
void zConversationStop(xJawSpeaker speaker); // UNUSED

#endif
