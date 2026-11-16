#ifndef ZUICUSTOMCONDITIONALLYDISABLED_H
#define ZUICUSTOMCONDITIONALLYDISABLED_H

#include "zUICustom.h"
#include "zConditional.h"

class zUICustomConditionallyDisabled : public zUICustom
{
public:
    zUICustomConditionallyDisabled(zUI* ui);

    virtual void PreUpdate(F32);
    virtual bool PreAutoMenu();
    virtual bool PreRender();

private:
    zConditional* m_pCondition;
    bool m_bCondition;
};

#endif
