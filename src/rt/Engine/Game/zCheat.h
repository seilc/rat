#ifndef ZCHEAT_H
#define ZCHEAT_H

enum zCheatStatus
{
    zCheatStatus_Bad,
    zCheatStatus_Ok,
    zCheatStatus_CreditCode,
    zCheatStatus_HILogo
};

zCheatStatus zCheatProcess(const char* name);

#endif
