#ifndef _VIDEO_H_
#define _VIDEO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#define VKREND_MAX_INSTANCE_EXTENSIONS 8

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

    VkImage frame_image;
    VkDeviceMemory frame_memory;
    VkImageView frame_view;
    br_uint_32 frame_width;
    br_uint_32 frame_height;
    br_boolean frame_initialised;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkFence fence;
    VkSemaphore acquire_semaphore;

    br_int_32 window_width;
    br_int_32 window_height;
    br_boolean active;
    br_int_32 recovery_attempted;
} br_vk_video;

#ifdef __cplusplus
};
#endif
#endif
