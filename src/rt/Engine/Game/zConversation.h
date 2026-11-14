#ifndef ZCONVERSATION_H
#define ZCONVERSATION_H

#include "xBase.h"
#include "xSnd.h"

bool zConversationIsConversing();
void zConversationPlay(xSndGroupHandle soundGroup, const xBase* otherSpeakerBase);

#endif
