#ifndef ZICOMPPERCEPTION_H
#define ZICOMPPERCEPTION_H

#include "zIComponent.h"
#include "zNPCBase.h"

class zICompPerception : public zIComponent<zNPCBase>
{
#if RELEASE
public:
    virtual bool IsPerceived() = 0;
    virtual bool IsRemembered() = 0;
#endif
};

#endif
