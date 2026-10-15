#ifndef _DEVPIXMP_H_
#define _DEVPIXMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BR_DEVICE_PIXELMAP_PRIVATE

typedef struct br_device_pixelmap {
    const struct br_device_pixelmap_dispatch* dispatch;

    const char* pm_identifier;

    BR_PIXELMAP_MEMBERS

    struct br_device* device;
    struct br_output_facility* output_facility;

    br_token use_type;

    struct br_device_pixelmap* screen;

    br_boolean sub_pixelmap;
    br_uint_16 parent_height;

    void* shadow;

    br_device_vkrend_callback_procs callbacks;

    br_vk_video video;
} br_device_pixelmap;

#endif

#ifdef __cplusplus
};
#endif
#endif
