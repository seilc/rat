#include "xCommon.h"
#include "zNPCLevelSpecific.h"

#include "containers.h"
#include "xFont.h"
#include "xPad.h"
#include "xutil.h"
#include "zGlobals.h"
#include "zScene.h"

ezGameLevel gNPCGameLevel = ezGAMELEVEL_Total;

void zNPCLevelSpecificSetup()
{
    switch (globals.sceneCur->sceneID) {
    case IDTAG('U','T','0','1'): gNPCGameLevel = ezGAMELEVEL_UT01; break;
    case IDTAG('D','D','0','1'): gNPCGameLevel = ezGAMELEVEL_DD01; break;
    case IDTAG('M','T','0','1'): gNPCGameLevel = ezGAMELEVEL_MT01; break;
    case IDTAG('B','B','0','1'): gNPCGameLevel = ezGAMELEVEL_BB01; break;
    case IDTAG('B','1','0','1'): gNPCGameLevel = ezGAMELEVEL_B101; break;
    case IDTAG('M','M','0','1'): gNPCGameLevel = ezGAMELEVEL_MM01; break;
    case IDTAG('R','F','0','1'): gNPCGameLevel = ezGAMELEVEL_RF01; break;
    case IDTAG('B','2','0','1'): gNPCGameLevel = ezGAMELEVEL_B201; break;
    case IDTAG('U','W','0','1'): gNPCGameLevel = ezGAMELEVEL_UW01; break;
    case IDTAG('C','C','0','1'): gNPCGameLevel = ezGAMELEVEL_CC01; break;
    case IDTAG('B','3','0','1'): gNPCGameLevel = ezGAMELEVEL_B301; break;
    case IDTAG('A','1','0','1'):
    case IDTAG('A','1','0','2'):
    case IDTAG('A','1','0','3'):
        gNPCGameLevel = ezGAMELEVEL_A10x; break;
    case IDTAG('A','2','0','1'):
    case IDTAG('A','2','0','2'):
    case IDTAG('A','2','0','3'):
        gNPCGameLevel = ezGAMELEVEL_A20x; break;
    case IDTAG('A','3','0','1'):
    case IDTAG('A','3','0','2'):
    case IDTAG('A','3','0','3'):
        gNPCGameLevel = ezGAMELEVEL_A30x; break;
    default:
        gNPCGameLevel = ezGAMELEVEL_Misc; break;
    }

    xASSERT_L(60, gNPCGameLevel >= 0 && gNPCGameLevel < ezGAMELEVEL_Total);
}

F32 zNPCLevelSpecificGetRoundRobinTimeOut()
{
    F32 timeOutValue[ezGAMELEVEL_Total] =
    {
        10.0f,
        9.0f,
        8.0f,
        7.0f,
        6.0f,
        5.0f,
        4.0f,
        3.0f,
        2.0f,
        1.0f,
        0.5f,
        5.0f,
        3.0f,
        1.0f,
        5.0f
    };

    return timeOutValue[gNPCGameLevel];
}

F32 zNPCLevelSpecificGetUpdateDistance()
{
    F32 updateDistanceValue[ezGAMELEVEL_Total] =
    {
        100.0f,
        140.0f,
        100.0f,
        140.0f,
        100.0f,
        100.0f,
        100.0f,
        100.0f,
        100.0f,
        100.0f,
        100.0f,
        10000.0f,
        10000.0f,
        10000.0f,
        100.0f
    };

    return updateDistanceValue[gNPCGameLevel];
}

F32 zNPCLevelSpecificGetSurrenderDistance()
{
    F32 surrenderDistanceValue[ezGAMELEVEL_Total] =
    {
        200.0f,
        280.0f,
        200.0f,
        280.0f,
        200.0f,
        200.0f,
        200.0f,
        200.0f,
        200.0f,
        200.0f,
        200.0f,
        20000.0f,
        20000.0f,
        20000.0f,
        200.0f
    };

    return surrenderDistanceValue[gNPCGameLevel];
}
