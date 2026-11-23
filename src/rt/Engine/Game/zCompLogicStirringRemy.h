#ifndef ZCOMPLOGICSTIRRINGREMY_H
#define ZCOMPLOGICSTIRRINGREMY_H

#include "zCompStateMachine.h"

class zCompLogicStirringRemy : public zCompStateMachine
{
    xVec3 acc;
    xVec3 oldAcc;
};

#endif
