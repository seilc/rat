#include "xCommon.h"
#include "zCineFXTableTest.h"

#include "zCineFX.h"
#include "zCineFXShared.h"
#include "zCutsceneMgr.h"
#include "zParEmitter.h"

#define DEFINE_CINFX_PAR_UPD_L(name, emitname, line)                                                                                                \
    static void zCinFX_Par_##name##_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)                                                          \
    {                                                                                                                                               \
        if (killit) {                                                                                                                               \
            fxrec->flg_stat |= 0x2;                                                                                                                 \
            return;                                                                                                                                 \
        }                                                                                                                                           \
        if (fxrec->flg_stat & 0x4) {                                                                                                                \
            zParEmitter*& emitter = fxrec->fxdata.pardata.emitter;                                                                                  \
            emitter = zParEmitterFind(emitname);                                                                                                    \
            xParEmitterAsset* a = emitter->tasset;                                                                                                  \
            if (!a || a->emit_type != eParRegionEntBone) {                                                                                          \
                emitter = NULL;                                                                                                                     \
            }                                                                                                                                       \
            if (!emitter) {                                                                                                                         \
                xASSERT_FMT_L(line, false, "Could'nay find the entity bone emitter! '%s'", emitname);                                               \
                fxrec->flg_stat |= 0x2;                                                                                                             \
            } else {                                                                                                                                \
                if (fxrec->basic.pos_A[1].y < 0.0f) {                                                                                               \
                    fxrec->basic.pos_A[1].y = 0.1f + a->e_entbone.bone;                                                                             \
                }                                                                                                                                   \
                fxrec->basic.pos_B[0] = 1e9f;                                                                                                       \
            }                                                                                                                                       \
        }                                                                                                                                           \
    }
#define DEFINE_CINFX_PAR_UPD(name, emitname) DEFINE_CINFX_PAR_UPD_L(name, emitname, __LINE__)

namespace CineFXTableTest {

DEFINE_CINFX_PAR_UPD_L(CINFX_TEST_PAR_SMOKE, "PAREMIT_SMOKE", 85)

static void NCIN_upd_test(const zCutsceneMgr* csnmgr, NCINEntry* fxrec, S32 killit)
{
    iprintf("NCIN: %8.4f Update  - 0x%08X 0x%08X %8d\n", csnmgr->csn->Time, csnmgr, fxrec, killit);

    if (killit) {
        fxrec->flg_stat |= 0x2;
    }

    if (fxrec->flg_stat & 0x4) {
        PosInput* pi = &fxrec->basic;
        iprintf("NCIN: %8.4f Animate -\n"
                "            A0 { %8.3f, %8.3f, %8.3f }\n"
                "            A1 { %8.3f, %8.3f, %8.3f }\n"
                "            B0 { %8.3f, %8.3f, %8.3f }\n"
                "            B1 { %8.3f, %8.3f, %8.3f }\n",
                csnmgr->csn->Time,
                pi->pos_A[0].x, pi->pos_A[0].y, pi->pos_A[0].z,
                pi->pos_A[1].x, pi->pos_A[1].y, pi->pos_A[1].z,
                pi->pos_B[0].x, pi->pos_B[0].y, pi->pos_B[0].z,
                pi->pos_B[1].x, pi->pos_B[1].y, pi->pos_B[1].z);
    }
}

static void NCIN_anim_test(const zCutsceneMgr* csnmgr, NCINEntry* fxrec, RpAtomic* model, RwMatrix* animMat,
                           U32 animIndex, U32 dataIndex, U32& visflags)
{
    iprintf("NCIN: %8.4f Animate - 0x%08X 0x%08X 0x%08X 0x%08X %3d %3d 0x%08X\n", csnmgr->csn->Time, csnmgr, fxrec,
            model, animMat, animIndex, dataIndex, visflags);
}

static void NCIN_rend_test(const zCutsceneMgr* csnmgr, NCINEntry* fxrec)
{
    iprintf("NCIN: %8.4f Render  - 0x%08X 0x%08X\n", csnmgr->csn->Time, csnmgr, fxrec);
}

static void NCIN_tweak_test(const char* prefix, NCINEntry*)
{
    iprintf("NCIN: %8.4f Tweaks  - %s\n", -1.0, prefix);
}

static NCINEntry g_TR02_cin_test[] = {
    { { 1, CINFX_FRAMES(23.95f), CINFX_FRAMES(27.27f), "CinFX Test" },
      { NCIN_upd_test, NCIN_anim_test, NCIN_rend_test, NCIN_tweak_test },
      { 0 },
      { { { 8.64f, 9.411f, 1.659f }, { 99.0f, 101.0f, 108.0f } },
        { { -1.0f, -2.0f, -3.0f }, { 2.42f, -3.23f, 4.04f } } } },
    { { 0x8002, CINFX_FRAMES(10), CINFX_FRAMES(9), "Smoke1" },
      { zCinFX_Par_CINFX_TEST_PAR_SMOKE_Upd, zCinFX_EntityBonePar_AR } },
#if DEBUG || RELEASE
    { { 0x8001, CINFX_FRAMES(0), CINFX_FRAMES(10000), "BoneProbe" },
      { zCinFX_BoneProbe_Upd, zCinFX_BoneProbe_AR, NULL, zCinFX_BoneProbe_Twk } },
#endif
    { 0 }
};

} // namespace CineFXTableTest

void zCineFX_Register_TestTable()
{
    static NCINMap cutmap[] = {
        { "test UT_O", CineFXTableTest::g_TR02_cin_test, 0 },
        { NULL, NULL, 0 }
    };

    zCineFXRegisterTable(cutmap);
}
