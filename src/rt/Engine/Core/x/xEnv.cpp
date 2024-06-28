#include "xEnv.h"

#include "xDebug.h"
#include "xDebugTweak.h"
#include "xstransvc.h"

#include <stdio.h>

xEnv* gCurXEnv;

#ifdef DEBUGRELEASE
bool debugNoJspsRendering;
#endif

void xEnvLoadJSPList(xEnv* env, S32 dataType)
{
    U32 bufsize;
    void* buf;
    
    xVERBOSE("... entering xEnvLoadJSPList ...\n");

#ifdef DEBUGRELEASE
    xDebugRemoveTweak("Debugging Options|JSP Visibility");
#endif

    if (dataType == 0) {
        env->geom = &env->ienv;

        S32 jsp_count = xSTAssetCountByType('JSP ');
        xASSERTMSG(54, (jsp_count > 0), "No JSP's found in scene!");

        iEnvLoadBegin(env->geom, dataType, jsp_count / 2);

        S32 index = 0;
        for (S32 i = 0; i < jsp_count; i++) {
            buf = xSTFindAssetByType('JSP ', i, &bufsize);

            PKRAssetTOCInfo info;
            xSTGetAssetInfoByType('JSP ', i, &info);

            S32 rc = iEnvLoadJSP(env->geom, info.aid, buf, bufsize, dataType, index);
            if (rc) {
                index++;
            }
        }

        env->geom->jsp_count = index;
        iEnvLoadEnd(env->geom, dataType);
    }

    xASSERT(77, env->geom);

    gCurXEnv = env;
}

void xEnvSetup(xEnv* env)
{
    xASSERT(106, env->geom);

    iEnvSetup(env->geom);
    iEnvDefaultLighting(env->geom);

    env->lightKit = NULL;
    gCurXEnv = env;
}

void xEnvFree(xEnv* env)
{
    xASSERT(135, env);

    if (env->geom) {
        xASSERT(140, env->geom == &env->ienv);
        iEnvFree(env->geom);
        env->geom = NULL;
    }
}

void xEnvRender(xEnv* env, bool alpha)
{
    xASSERT(155, env);

    if (env->geom) {
        iEnvRender(env->geom, alpha);
    }

    if (env->geom->jsp_count > 0) {
        U32 jspRenderCount = 0;

        for (S32 i = 0; i < env->geom->jsp_count; i++) {
            if (env->geom->jsp_visibilityCount[i] > 0) {
                jspRenderCount++;
            }

#ifdef DEBUGRELEASE
            char text[128];
            if (env->geom->jsp_visibilityCount[i] < 0) {
                sprintf(text, "JSP '%s' had too many Decrements (once only assertion)", xSTAssetName(env->geom->jsp_aid[i]));
                xASSERTONCEFMT(175, "%s", text);
            }
            if (env->geom->jsp_visibilityCount[i] > 4) {
                sprintf(text, "JSP '%s' had suspiciously high Inc's\n (more than 4, missing Dec's?)", xSTAssetName(env->geom->jsp_aid[i]));
                xASSERTONCEFMT(181, "%s", text);
            }
#endif
        }

        if (env->geom->jsp_count == 1) {
            env->geom->jsp_visibilityCount[0] = 1;
        }
    }
}

void xEnvResetJSPVisibilities(xEnv* env)
{
    xASSERT(204, env);

    if (env && env->geom) {
        for (S32 i = 0; i < env->geom->jsp_count; i++) {
            env->geom->jsp_visibilityCount[i] = 0;
        }
    }
}
