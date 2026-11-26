#ifndef ZGAMEEXTRAS_H
#define ZGAMEEXTRAS_H

#include "types.h"

struct xSerial;

S32 zGameExtras_ExtrasFlags(); // UNUSED
void zGameExtras_MoDay(S32* month, S32* day); // UNUSED
void zGameExtras_SceneInit();
void zGameExtras_SceneReset();
void zGameExtras_SceneExit();
void zGameExtras_SceneUpdate(F32 dt);
S32 zGameExtras_CheatFlags(); // UNUSED
void zGameExtras_NewGameReset();
void zGameExtras_Save(xSerial* xser);
void zGameExtras_Load(xSerial* xser);
void zGameCheatsUpdate(F32 dt);
void zGame_HackGalleryInit(); // UNUSED
U32 zGame_HackIsGallery(); // UNUSED

#endif
