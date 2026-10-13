#include "drv.h"

br_device* BR_EXPORT BrDrv1VkBegin(const char* arguments) {
    br_device* dev = DeviceVkAllocate("vkrend", arguments);

    if (dev == NULL)
        return NULL;

    return dev;
}

br_token BR_EXPORT BrVkrendCallbacksToken(void) {
    static br_token callbacks_token = BR_NULL_TOKEN;

    if (callbacks_token == BR_NULL_TOKEN)
        callbacks_token = BrTokenCreate(BR_VKREND_CALLBACKS_TOKEN_NAME, BRT_POINTER);

    return callbacks_token;
}

#ifdef DEFINE_BR_ENTRY_POINT
br_device* BR_EXPORT BrDrv1Begin(const char* arguments) {
    return BrDrv1VkBegin(arguments);
}
#endif
