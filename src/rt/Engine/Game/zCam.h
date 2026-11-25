#ifndef ZCAM_H
#define ZCAM_H

#include "zCamPlayer.h"

void zCamSetCamera(U32 cameraID, bool forceRestart);
xVec3& zCamTransitionTarget();
zCamPlayer* zCamGetDefault();
xCam* zCamGetPrimary();
void zCamAdd(xCam& cam, bool force_cut);
void zCamRemove(xCam& cam, bool force_cut);
bool zCamGetDebugMode(xMat4x3** ppDebugCamMtx);
xMat4x3& zCamGetMatrix();
void zCamExitDebugCam();

inline void zCamTrackGround(bool);

#endif
