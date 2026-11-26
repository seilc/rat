#ifndef ZRENDERSTATE_H
#define ZRENDERSTATE_H

#include "types.h"

typedef enum _SDRenderState
{
    eRenderStateUnknown,
    eRenderStateDefault,
    eRenderStateOpaqueModels,
    eRenderStateAlphaModels,
    eRenderStateBubble,
    eRenderStateProjectile,
    eRenderStateFont,
    eRenderStateHUD,
    eRenderStateParticles,
    eRenderStateLightning,
    eRenderStateSkidMark,
    eRenderStateStreak,
    eRenderStateTide,
    eRenderStateSkyDomeBack,
    eRenderStateSkyDomeFlat,
    eRenderStateSkyDome3D,
    eRenderStateEnvironment,
    eRenderStateFill,
    eRenderStateGlare,
    eRenderStateFireCards,
    eRenderStateSplashCones,
    eRenderStateWater,
    eRenderStateGrass,
    eRenderStateWaterSkyDome,
    eRenderStateLaserBeam,
    eRenderStateSpotlight,
    eRenderStateLightVolume,
    eRenderStateRibbon,
    eRenderStateScanLaser,
    eRenderStateHDRBeginRender,
    eRenderStateBlurAlpha,
    eRenderStateZParPTank,
    eRenderStateXPTankPool,
    eRenderStateUI,
    eRenderStateForceField,
    eRenderStateFrozone,
    eRenderStateSmashRing,
    eRenderStateLaserBolt,
    eRenderStatePatStunRings,
    eRenderStateDecal,
    eRenderStateUberLaser,
    eRenderStateGCWaterBumpGen,
    eRenderStateScrFX,
    eRenderStateIncrediSlam,
    eRenderStateWaterHose,
    eRenderStatePlayerIdentifier,
    eRenderStateFrostBlast,
    eRenderStateBlastRing,
    eRenderStateFreezingNPCs,
    eRenderStateBlurCards,
    eRenderStateAlphaModelsExplicit,
    eRenderStateBossBrain,
    eRenderStatePlayerEffects,
    eRenderStateTotal = -1
} SDRenderState;

// sRSBits
#define k_ZRENDERSTATE_TEXTUREFILTER 0x1
#define k_ZRENDERSTATE_TEXTUREADDRESS 0x2
#define k_ZRENDERSTATE_FOG 0x4
#define k_ZRENDERSTATE_VERTEXALPHA 0x8
#define k_ZRENDERSTATE_CULLMODE 0x10
#define k_ZRENDERSTATE_SRCBLEND 0x20
#define k_ZRENDERSTATE_DESTBLEND 0x40
#define k_ZRENDERSTATE_SHADEMODE 0x80
#define k_ZRENDERSTATE_ZWRITE 0x100
#define k_ZRENDERSTATE_ZTEST 0x200

void zRenderStateInit();
SDRenderState zRenderStateCurrent();
void zRenderStateBegin();
void zRenderState(SDRenderState next);
void zRenderStateSetDirty(U32 bits);

#endif
