#ifndef _VIDEO_H_
#define _VIDEO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#define VKREND_MAX_INSTANCE_EXTENSIONS 8

struct vk_gpu_target;

typedef struct br_vk_video {
    br_device_vkrend_callback_procs callbacks;

    PFN_vkGetInstanceProcAddr gpa;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    br_uint_32 queue_family;

    VkSwapchainKHR swapchain;
    VkFormat format;
    VkColorSpaceKHR color_space;
    VkExtent2D extent;
    br_uint_32 image_count;
    VkImage* images;
    VkImageView* views;
    VkFramebuffer* framebuffers;
    VkSemaphore* render_semaphores;

    VkRenderPass render_pass;
    VkDescriptorSetLayout set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkSampler sampler;

    VkBuffer pixel_buffer;
    VkDeviceMemory pixel_memory;
    void* pixel_mapped;
    VkDeviceSize pixel_buffer_size;
    VkDeviceSize pixel_slot_size;
    br_uint_32 pixel_slot;

    VkImage overlay_image;
    VkDeviceMemory overlay_memory;
    VkImageView overlay_view;
    br_uint_32 overlay_width;
    br_uint_32 overlay_height;
    br_boolean overlay_initialised;
    br_uint_32 overlay_generation;

    VkImage black_image;
    VkDeviceMemory black_memory;
    VkImageView black_view;
    br_boolean black_initialised;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkFence fence;
    VkSemaphore acquire_semaphore;

    VkCommandBuffer scene_command_buffer;
    VkFence scene_fence;
    br_boolean scene_recording;
    br_boolean scene_fence_pending;
    br_uint_32 frame_epoch;

    struct vk_gpu_target* present_target;
    br_boolean frame_has_scene;

    br_int_32 window_width;
    br_int_32 window_height;
    br_boolean active;
    br_int_32 recovery_attempted;
    br_uint_32 recovery_stable_frames;
    br_boolean recovery_exhausted;
    br_uint_32 present_serial;
    br_uint_32 recovery_serial;
    br_uint_32 recovery_cooldown;
    br_uint_32 generation;
} br_vk_video;

PFN_vkGetInstanceProcAddr VIDEOVkGetInstanceProcAddr(void);

br_error VIDEOVkDeviceLost(br_vk_video* video);

br_boolean VIDEOVkOverlayEnsure(br_vk_video* video, br_uint_32 width, br_uint_32 height);

void VIDEOVkOverlayUpload(br_vk_video* video, VkCommandBuffer command_buffer, const br_uint_8* pixels,
    br_uint_32 width, br_uint_32 height, br_int_32 row_bytes, br_size_t base);

#ifdef __cplusplus
};
#endif
#endif
