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

void zRenderState(SDRenderState next);

#endif
