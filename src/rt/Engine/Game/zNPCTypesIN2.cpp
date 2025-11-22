#include "zNPCTypesIN2.h"

const zNPCTypeInfoIN2* zNPCTypesGetInfo()
{
    static const zNPCTypeInfoIN2 info[] = {
        { 1, NULL, NULL, NULL, NULL, NULL },
        { 2, NULL, NULL, NULL, NULL, NULL },
        {}
    };

    return info;
}
