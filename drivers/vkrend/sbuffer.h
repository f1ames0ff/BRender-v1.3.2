#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BR_BUFFER_STORED_PRIVATE

typedef struct br_buffer_stored {
    struct br_buffer_stored_dispatch* dispatch;

    const char* identifier;

    br_device* device;
    br_renderer* renderer;

    br_pixelmap* source;

    br_uint_16 source_flags;

} br_buffer_stored;

#endif

#ifdef __cplusplus
};
#endif
#endif
