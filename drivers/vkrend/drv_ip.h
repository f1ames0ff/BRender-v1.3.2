#ifndef _DRV_IP_H_
#define _DRV_IP_H_

#ifndef NO_PROTOTYPES

#ifdef __cplusplus
extern "C" {
#endif

br_device* DeviceVkAllocate(const char* identifier, const char* arguments);

br_output_facility* OutputFacilityVkInit(br_device* dev);

br_device_pixelmap* DevicePixelmapVkAllocateFront(br_device* dev, br_output_facility* outfcty, br_token_value* tv);

br_error VIDEOVkCreate(br_vk_video* video, const br_device_vkrend_callback_procs* callbacks, br_int_32 width,
    br_int_32 height);

void VIDEOVkDestroy(br_vk_video* video);

br_error VIDEOVkResize(br_vk_video* video, br_int_32 width, br_int_32 height);

br_error VIDEOVkPresent(br_vk_video* video, const void* pixels, br_int_32 width, br_int_32 height, br_int_32 row_bytes);

#ifdef __cplusplus
};
#endif

#endif
#endif
