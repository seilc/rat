#include "xSkyDome.h"

#include "zBase.h"
#include "zEvent.h"
#include "iCamera.h"
#include "zRenderState.h"

struct SkyDomeInfo
{
    xEnt* ent;
    S32 sortorder;
    S32 lockXZ;
    S32 lockY;
    S32 zReadWrite;
};

#define MAX_SKYDOME_OBJECTS 16

static SkyDomeInfo sSkyList[MAX_SKYDOME_OBJECTS];
static S32 sSkyCount;

void xSkyDome_EmptyRender(xEnt*)
{
}

void xSkyDome_Setup()
{
    sSkyCount = 0;
}

void xSkyDome_AddEntity(xEnt* ent, S32 sortorder, S32 lockY, S32 lockXZ, S32 ZReadWrite)
{
    if (xSkyDome_EntityIsSkyDome(ent)) {
        S32 index = (S32)ent->user_data;
        xASSERT(64, (index >= 0) && (index < sSkyCount));
        SkyDomeInfo& info = sSkyList[index];
        xASSERT(66, info.ent == ent);
        return;
    }
    
    bool inList = false;
    S32 i, j;
    for (i = 0; i < sSkyCount; i++) {
        if (sSkyList[i].ent == ent) {
            S32 index = (S32)ent->user_data;
            xASSERT(78, index == i);
            inList = true;
            break;
        }
    }
    xASSERT(83, sSkyCount < MAX_SKYDOME_OBJECTS);
    if (!inList) {
        for (i = 0; i < sSkyCount; i++) {
            if (sortorder < sSkyList[i].sortorder) {
                break;
            }
        }
        for (j = sSkyCount-1; j >= i; j--) {
            sSkyList[j+1] = sSkyList[j];
            sSkyList[j+1].ent->user_data = (void*)(j+1);
        }
        sSkyList[i].ent = ent;
        sSkyList[i].sortorder = sortorder;
        sSkyList[i].lockY = lockY;
        sSkyList[i].lockXZ = lockXZ;
        sSkyList[i].zReadWrite = ZReadWrite;
        sSkyCount++;
        ent->user_data = (void*)i;
        ent->model->Flags &= (U16)~0x1;
    }

    ent->render = xSkyDome_EmptyRender;
    ent->baseFlags &= (U16)~k_XBASE_RECEIVES_SHADOWS;
    if (ent->baseType == eBaseTypePlatform) {
        ent->baseFlags |= k_XBASE_UNK_0x80;
    }

    zEntEvent(ent, eEventCollisionOff);
    zEntEvent(ent, eEventCameraCollideOff);
}

void xSkyDome_Render()
{
    RwCamera* cam = RwCameraGetCurrentCamera();
    RwFrame* camframe = RwCameraGetFrame(cam);
    RwMatrix* cammat = RwFrameGetMatrix(camframe);

    RwCameraEndUpdate(cam);

    if (sSkyCount > 0) {
        RwReal far_clip = RwCameraGetFarClipPlane(cam);
        RwCameraSetFarClipPlane(cam, 10000.0f);
        RwCameraBeginUpdateWrapper(cam);
        zRenderState(eRenderStateSkyDomeBack);

        S32 useZBuf = FALSE;
        for (S32 i = 0; i < sSkyCount; i++) {
            xEnt* ent = sSkyList[i].ent;
            ent->model->Flags &= (U16)~0x1;

            if (!ent->model) continue;
            if (!xEntIsVisible(ent)) continue;

            RwV3d oldpos;
            oldpos = ent->model->Mat->pos;

            if (sSkyList[i].lockXZ) {
                ent->model->Mat->pos.x = cammat->pos.x;
                ent->model->Mat->pos.z = cammat->pos.z;
            }
            if (sSkyList[i].lockY) {
                ent->model->Mat->pos.y = cammat->pos.y;
            }

            if (!xModelCullSingle(ent->model)) {
#ifdef DEBUGRELEASE
                if (xModelRenderLogEnabled()) {
                    xModelRenderLogSetEntity(ent);
                }
#endif

                if (useZBuf != sSkyList[i].zReadWrite) {
                    xASSERT_DESIGN_M(202, i != 0 || !sSkyList[i].zReadWrite,
                                     "ART/DESIGN: Outermost SkyDome layer must have \"Set SkyDome\" parameter 4 set to 0!");
                    xASSERT_DESIGN_M(204, !useZBuf,
                                     "ART/DESIGN: Got a flat layer (parameter 4 = 0) layer after a non-flat layer");
                    useZBuf = sSkyList[i].zReadWrite;
                    if (useZBuf) {
                        zRenderState(eRenderStateSkyDome3D);
                    }
                } else if (i == 1 && !sSkyList[i].zReadWrite) {
                    zRenderState(eRenderStateSkyDomeFlat);
                }

                iModelRender(ent->model->Data, ent->model->Mat);
            }

            ent->model->Mat->pos = oldpos;
        }

        RwCameraEndUpdate(cam);
        RwCameraSetFarClipPlane(cam, far_clip);
    }
    
    RwRGBA color = { 0, 0, 0, 0 };
    RwCameraClear(cam, &color, (rwCAMERACLEARZ|rwCAMERACLEARSTENCIL));
    RwCameraBeginUpdateWrapper(cam);
}

#ifndef NON_MATCHING
static void __unused(xEnt* ent)
{
    xASSERT(0, xSkyDome_EntityIsSkyDome(ent));
}
#endif
