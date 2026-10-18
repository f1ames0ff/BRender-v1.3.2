#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#ifndef VKREND_SBUFFER_TEXTURE_API
#define VKREND_SBUFFER_TEXTURE_API 1
#endif

typedef struct vk_buffer_texture {
    VkImage image;
    VkImageView view;
    br_uint_32 width;
    br_uint_32 height;
    br_uint_32 generation;
} vk_buffer_texture;

br_boolean BufferStoredVkGetTexture(struct br_buffer_stored* stored, br_vk_video* video, vk_buffer_texture* texture);
br_uint_32 BufferStoredVkTextureGeneration(struct br_buffer_stored* stored);
void BufferStoredVkResetAfterRecovery(void);

#ifdef BR_BUFFER_STORED_PRIVATE

typedef struct br_buffer_stored {
    struct br_buffer_stored_dispatch* dispatch;

    const char* identifier;

    br_device* device;
    br_renderer* renderer;

    br_pixelmap* source;

    br_uint_16 source_flags;

    vk_buffer_texture texture;
    VkDeviceMemory texture_memory;
    VkDevice texture_device;
    br_vk_video* video;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    void* staging_mapped;
    br_uint_32 staging_bytes;

    br_uint_32 revision;
    br_uint_32 built_revision;
    br_uint_32 built_generation;
    br_pixelmap* built_source;
    br_pixelmap* built_palette;

    br_device_clut* clut_queried;
    br_boolean clut_query_done;
    br_device_clut* built_clut;
    br_uint_32 built_clut_revision;

    br_boolean unavailable;

} br_buffer_stored;

#endif

#ifdef __cplusplus
};
#endif
#endif
