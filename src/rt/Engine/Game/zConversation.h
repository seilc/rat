#ifndef ZCONVERSATION_H
#define ZCONVERSATION_H

#include "xBase.h"
#include "iSnd.h"

bool zConversationIsConversing();
void zConversationPlay(iSndGroupHandle soundGroup, const xBase* otherSpeakerBase);

#endif
