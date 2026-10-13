#ifndef _BRVKREND_P_H_
#define _BRVKREND_P_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _NO_PROTOTYPES

struct br_device* BR_EXPORT BrDrv1VkBegin(const char* arguments);

br_token BR_EXPORT BrVkrendCallbacksToken(void);

#endif

#ifdef __cplusplus
};
#endif
#endif
