#ifndef ZCAM_H
#define ZCAM_H

#include "zCamPlayer.h"

xVec3& zCamTransitionTarget();
zCamPlayer* zCamGetDefault();
xCam* zCamGetPrimary();
void zCamAdd(xCam& cam, bool force_cut);
void zCamRemove(xCam& cam, bool force_cut);
bool zCamGetDebugMode(xMat4x3** ppDebugCamMtx);

inline void zCamTrackGround(bool);

#endif
