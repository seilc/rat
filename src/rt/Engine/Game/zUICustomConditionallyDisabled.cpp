#include "xCommon.h"
#include "zUICustomConditionallyDisabled.h"

#include "zEvent.h"
#include "zScene.h"
#include "zUI.h"

zUICustomConditionallyDisabled::zUICustomConditionallyDisabled(zUI* ui) : zUICustom(ui), m_pCondition(NULL)
{
    xBase* pCond = zSceneFindObject(ui->asset->customWidget);
    if (pCond) {
        if (pCond && pCond->baseType == eBaseTypeCond)
            m_pCondition = (zConditional*)pCond;
        else
            xFAIL_MSG_L(18, "Custom Widget Parameter doesn't match expected type. Should be a zConditional\n");
    } else {
        xFAIL_MSG_L(23, "Custom Widget Parameter doesn't exist\n");
    }
}

void zUICustomConditionallyDisabled::PreUpdate(F32)
{
    if (m_pCondition) {
        bool new_condition = zConditional_Evaluate(m_pCondition);
        if (m_bCondition != new_condition) {
            m_bCondition = new_condition;
            if (!m_bCondition) {
                GetUI()->AutoMenu(&zUIAsset::autoMenuUp, eEventUIAutoMenuRunUp);
            }
        }
    } else
        m_bCondition = true;
}

bool zUICustomConditionallyDisabled::PreAutoMenu()
{
    return m_bCondition;
}

bool zUICustomConditionallyDisabled::PreRender()
{
    return m_bCondition;
}
