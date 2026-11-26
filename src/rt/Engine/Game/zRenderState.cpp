#include "xCommon.h"
#include "zRenderState.h"

#include "iCamera.h"
#include "xRenderState.h"
#include "xstransvc.h"

static SDRenderState sRS;
static U32 sRSBits = 0xFFFFFFFF;

#if DEBUG
namespace
{
    struct state_list
    {
        RwInt32 texturefilter;
        RwInt32 textureaddress;
        RwBool fogenable;
        RwBool vertexalphaenable;
        RwInt32 cullmode;
        RwBlendFunction srcblend;
        RwBlendFunction destblend;
        RwInt32 shademode;
        RwBool zwriteenable;
        RwBool ztestenable;
    };

    state_list old_states;

    void store_states(state_list& s)
    {
        RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &s.texturefilter);
        RwRenderStateGet(rwRENDERSTATETEXTUREADDRESS, &s.textureaddress);
        RwRenderStateGet(rwRENDERSTATEFOGENABLE, &s.fogenable);
        RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &s.vertexalphaenable);
        RwRenderStateGet(rwRENDERSTATECULLMODE, &s.cullmode);
        xRenderStateBlendModesGet(&s.srcblend, &s.destblend);
        RwRenderStateGet(rwRENDERSTATESHADEMODE, &s.shademode);
        RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &s.zwriteenable);
        RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &s.ztestenable);
    }

    void debug_store_states()
    {
        store_states(old_states);
    }

    void debug_verify_states()
    {
        state_list states;
        store_states(states);
        xASSERT_L(75, states.texturefilter == old_states.texturefilter);
        xASSERT_L(76, states.textureaddress == old_states.textureaddress);
        xASSERT_L(83, states.zwriteenable == old_states.zwriteenable);
        xASSERT_L(84, states.ztestenable == old_states.ztestenable);
    }
} // namespace
#endif

void zRenderStateInit()
{
    sRS = eRenderStateUnknown;
    sRSBits = 0xFFFFFFFF;
}

SDRenderState zRenderStateCurrent()
{
    return sRS;
}

void zRenderStateBegin()
{
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);

    iCameraSetFogRenderStates();

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, TRUE, TRUE);

    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);

    sRS = eRenderStateDefault;
    sRSBits = 0;

    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEARMIPLINEAR);
}

void zRenderState(SDRenderState next)
{
    if (next == sRS && sRSBits == 0) {
#if DEBUG
        static bool enable_verify = false;
        static S32 recur_depth = 0;
        if (enable_verify) {
            if (recur_depth == 0) {
                debug_store_states();
                recur_depth++;
                zRenderState(sRS);
                recur_depth--;
                debug_verify_states();
                return;
            }
        } else {
            return;
        }
#else
        return;
#endif
    }

    sRS = next; U32 customBits = 0;
    switch (sRS) {
    case eRenderStateDefault:
    case eRenderStateAlphaModels:
    case eRenderStateProjectile:
    case eRenderStateSkyDome3D:
        break;
    case eRenderStateOpaqueModels:
        customBits = k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDZERO, TRUE, TRUE);
        break;
    case eRenderStateBlastRing:
        customBits = k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        break;
    case eRenderStateUI:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        iCameraSetFog(false);
        xRenderStateZModesSet(FALSE, FALSE);
        break;
    case eRenderStateTide:
    case eRenderStateEnvironment:
        customBits = k_ZRENDERSTATE_CULLMODE;
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
        break;
    case eRenderStateParticles:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SHADEMODE | k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        iCameraSetFog(false);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
        break;
    case eRenderStateLightning:
        customBits = k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, TRUE);
        break;
    case eRenderStateSkidMark:
    case eRenderStateStreak:
        customBits = k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        break;
    case eRenderStateGlare:
    case eRenderStateIncrediSlam:
    case eRenderStateWaterHose:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        iCameraSetFog(false);
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, TRUE);
        break;
    case eRenderStatePlayerEffects:
        customBits = k_ZRENDERSTATE_TEXTUREADDRESS | k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_DESTBLEND |
                     k_ZRENDERSTATE_ZWRITE;
        iCameraSetFog(false);
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, TRUE);
        break;
    case eRenderStatePlayerIdentifier:
        customBits = k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_SHADEMODE |
                     k_ZRENDERSTATE_ZWRITE;
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, TRUE);
        break;
    case eRenderStateFont:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SHADEMODE | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        iCameraSetFog(false);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
        xRenderStateZModesSet(FALSE, FALSE);
        break;
    case eRenderStateHUD:
        customBits = k_ZRENDERSTATE_FOG;
        iCameraSetFog(false);
        break;
    case eRenderStateBubble:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_CULLMODE;
        iCameraSetFog(false);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
        break;
    case eRenderStateSkyDomeBack:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZTEST;
        xRenderStateZModesSet(TRUE, FALSE);
        iCameraSetFog(false);
        break;
    case eRenderStateSkyDomeFlat:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        xRenderStateZModesSet(FALSE, FALSE);
        break;
    case eRenderStateFill:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_VERTEXALPHA | k_ZRENDERSTATE_SRCBLEND |
                     k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_SHADEMODE | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        iCameraSetFog(false);
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDZERO, FALSE, FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
        break;
    case eRenderStateFireCards:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        iCameraSetFog(false);
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDONE, FALSE, TRUE);
        break;
    case eRenderStateSplashCones:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_VERTEXALPHA | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        iCameraSetFog(false);
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, TRUE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
        break;
    case eRenderStateWater:
        customBits = k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND;
        xRenderStateBlendModesSet(rwBLENDONE, rwBLENDZERO);
        break;
    case eRenderStateGrass:
        customBits = k_ZRENDERSTATE_TEXTUREADDRESS | k_ZRENDERSTATE_FOG;
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
        iCameraSetFog(false);
        break;
    case eRenderStateWaterSkyDome:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_VERTEXALPHA | k_ZRENDERSTATE_SRCBLEND |
                     k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        iCameraSetFog(false);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDZERO, FALSE, FALSE);
        break;
    case eRenderStateLaserBeam:
        customBits = k_ZRENDERSTATE_TEXTUREADDRESS | k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SRCBLEND |
                     k_ZRENDERSTATE_DESTBLEND;
        xRenderStateBlendModesSet(rwBLENDONE, rwBLENDONE);
        iCameraSetFog(false);
        break;
    case eRenderStateSpotlight:
        customBits = k_ZRENDERSTATE_TEXTUREADDRESS | k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SRCBLEND |
                     k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDONE, FALSE, TRUE);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
        iCameraSetFog(false);
        break;
    case eRenderStateLightVolume:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDONE, FALSE, TRUE);
        iCameraSetFog(false);
        break;
    case eRenderStateRibbon:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        iCameraSetFog(false);
        break;
    case eRenderStateScanLaser:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_CULLMODE | k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND |
                     k_ZRENDERSTATE_ZWRITE;
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDONE, FALSE, TRUE);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
        iCameraSetFog(false);
        break;
    case eRenderStateHDRBeginRender:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        iCameraSetFog(false);
        break;
    case eRenderStateBlurAlpha:
        customBits = k_ZRENDERSTATE_TEXTUREADDRESS | k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SRCBLEND |
                     k_ZRENDERSTATE_DESTBLEND;
        xRenderStateBlendModesSet(rwBLENDONE, rwBLENDZERO);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
        iCameraSetFog(false);
        break;
    case eRenderStateZParPTank:
        customBits = k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, TRUE);
        break;
    case eRenderStateXPTankPool:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE;
        iCameraSetFog(false);
        xRenderStateZModesSet(FALSE, TRUE);
        break;
    case eRenderStateForceField:
        customBits = k_ZRENDERSTATE_TEXTUREADDRESS | k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_CULLMODE |
                     k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        iCameraSetFog(false);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
        break;
    case eRenderStateFrozone:
        customBits = k_ZRENDERSTATE_FOG;
        iCameraSetFog(false);
        break;
    case eRenderStateSmashRing:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE;
        iCameraSetFog(false);
        xRenderStateZModesSet(FALSE, TRUE);
        break;
    case eRenderStateLaserBolt:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        iCameraSetFog(false);
        break;
    case eRenderStateFrostBlast:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        iCameraSetFog(false);
        break;
    case eRenderStateDecal:
        customBits = k_ZRENDERSTATE_TEXTUREADDRESS | k_ZRENDERSTATE_CULLMODE | k_ZRENDERSTATE_ZWRITE;
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
        xRenderStateZModesSet(FALSE, TRUE);
        break;
    case eRenderStateUberLaser:
        customBits = k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND;
        xRenderStateBlendModesSet(rwBLENDONE, rwBLENDONE);
        break;
    case eRenderStateGCWaterBumpGen:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, FALSE);
        iCameraSetFog(false);
        break;
    case eRenderStateScrFX:
        customBits = k_ZRENDERSTATE_TEXTUREFILTER | k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_ZWRITE | k_ZRENDERSTATE_ZTEST;
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
        iCameraSetFog(false);
        xRenderStateZModesSet(FALSE, FALSE);
        break;
    case eRenderStateFreezingNPCs:
        customBits = k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDONE, FALSE, TRUE);
        break;
    case eRenderStateBlurCards:
        customBits = k_ZRENDERSTATE_FOG | k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND | k_ZRENDERSTATE_ZWRITE;
        xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDONE, FALSE, TRUE);
        iCameraSetFog(false);
        break;
    case eRenderStateBossBrain:
        customBits = k_ZRENDERSTATE_ZWRITE;
        xRenderStateZModesSet(FALSE, TRUE);
        break;
    case eRenderStateAlphaModelsExplicit:
        customBits = k_ZRENDERSTATE_SRCBLEND | k_ZRENDERSTATE_DESTBLEND;
        xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
        break;
    case eRenderStateUnknown:
    default:
        xFAIL_L(529);
        break;
    }

    U32 diffBits = sRSBits & ~customBits;
    if (diffBits) {
        if (diffBits & k_ZRENDERSTATE_TEXTUREFILTER) {
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
        }

        if (diffBits & k_ZRENDERSTATE_TEXTUREADDRESS) {
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
        }

        if (diffBits & k_ZRENDERSTATE_FOG) {
            iCameraSetFog(true);
        }

        if (diffBits & k_ZRENDERSTATE_VERTEXALPHA) {
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        }

        if (diffBits & k_ZRENDERSTATE_CULLMODE) {
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
        }

        if ((diffBits & k_ZRENDERSTATE_SRCBLEND) || (diffBits & k_ZRENDERSTATE_DESTBLEND)) {
            if (diffBits & k_ZRENDERSTATE_ZWRITE) {
                xRenderStateBlendAndZModesSet(
                    rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, TRUE, (customBits & k_ZRENDERSTATE_ZTEST) ? FALSE : TRUE);
            }
            else if (diffBits & k_ZRENDERSTATE_ZTEST) {
                xRenderStateBlendAndZModesSet(
                    rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, (customBits & k_ZRENDERSTATE_ZWRITE) ? FALSE : TRUE, TRUE);
            } else {
                xRenderStateBlendModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
            }
        } else {
            if (diffBits & k_ZRENDERSTATE_ZWRITE) {
                xRenderStateZModesSet(TRUE, (customBits & k_ZRENDERSTATE_ZTEST) ? FALSE : TRUE);
            }
            else if (diffBits & k_ZRENDERSTATE_ZTEST) {
                xRenderStateZModesSet((customBits & k_ZRENDERSTATE_ZWRITE) ? FALSE : TRUE, TRUE);
            }
        }

        if (diffBits & k_ZRENDERSTATE_SHADEMODE) {
            RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
        }
    }

    sRSBits = customBits;
}

void zRenderStateSetDirty(U32 bits)
{
    sRSBits |= bits;
}
