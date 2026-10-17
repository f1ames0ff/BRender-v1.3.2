#include "drv.h"

#include <stdio.h>
#include <string.h>

#define VKREND_GPU_UBO_SLOTS 8192
#define VKREND_GPU_UBO_STRIDE 256
#define VKREND_GPU_UBO_RANGE 192
#define VKREND_GPU_VERTEX_STRIDE 36
#define VKREND_GPU_VERTEX_FLOATS 9
#define VKREND_GPU_TEXTURE_SET_CAPACITY 512
#define VKREND_GPU_RETIRED_CAPACITY 1024
#define VKREND_MAIN_TARGET_MIN_DRAWS 8

#ifndef VKREND_SBUFFER_TEXTURE_API

typedef struct vk_buffer_texture {
    VkImage image;
    VkImageView view;
    br_uint_32 width;
    br_uint_32 height;
    br_uint_32 generation;
} vk_buffer_texture;

br_boolean BufferStoredVkGetTexture(struct br_buffer_stored* stored, br_vk_video* video, vk_buffer_texture* texture);

#endif

typedef struct vk_gpu_functions {
    PFN_vkGetPhysicalDeviceFormatProperties GetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
    PFN_vkCreateImage CreateImage;
    PFN_vkDestroyImage DestroyImage;
    PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements;
    PFN_vkBindImageMemory BindImageMemory;
    PFN_vkCreateImageView CreateImageView;
    PFN_vkDestroyImageView DestroyImageView;
    PFN_vkCreateRenderPass CreateRenderPass;
    PFN_vkDestroyRenderPass DestroyRenderPass;
    PFN_vkCreateFramebuffer CreateFramebuffer;
    PFN_vkDestroyFramebuffer DestroyFramebuffer;
    PFN_vkCreateShaderModule CreateShaderModule;
    PFN_vkDestroyShaderModule DestroyShaderModule;
    PFN_vkCreateDescriptorSetLayout CreateDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout DestroyDescriptorSetLayout;
    PFN_vkCreateDescriptorPool CreateDescriptorPool;
    PFN_vkDestroyDescriptorPool DestroyDescriptorPool;
    PFN_vkAllocateDescriptorSets AllocateDescriptorSets;
    PFN_vkUpdateDescriptorSets UpdateDescriptorSets;
    PFN_vkCreatePipelineLayout CreatePipelineLayout;
    PFN_vkDestroyPipelineLayout DestroyPipelineLayout;
    PFN_vkCreateGraphicsPipelines CreateGraphicsPipelines;
    PFN_vkDestroyPipeline DestroyPipeline;
    PFN_vkCreateBuffer CreateBuffer;
    PFN_vkDestroyBuffer DestroyBuffer;
    PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements;
    PFN_vkBindBufferMemory BindBufferMemory;
    PFN_vkAllocateMemory AllocateMemory;
    PFN_vkFreeMemory FreeMemory;
    PFN_vkMapMemory MapMemory;
    PFN_vkUnmapMemory UnmapMemory;
    PFN_vkCmdPipelineBarrier CmdPipelineBarrier;
    PFN_vkCmdBeginRenderPass CmdBeginRenderPass;
    PFN_vkCmdEndRenderPass CmdEndRenderPass;
    PFN_vkCmdBindPipeline CmdBindPipeline;
    PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets;
    PFN_vkCmdSetViewport CmdSetViewport;
    PFN_vkCmdSetScissor CmdSetScissor;
    PFN_vkCmdClearColorImage CmdClearColorImage;
    PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage;
    PFN_vkCmdBindVertexBuffers CmdBindVertexBuffers;
    PFN_vkCmdBindIndexBuffer CmdBindIndexBuffer;
    PFN_vkCmdDraw CmdDraw;
    PFN_vkCmdDrawIndexed CmdDrawIndexed;
    PFN_vkBeginCommandBuffer BeginCommandBuffer;
    PFN_vkEndCommandBuffer EndCommandBuffer;
    PFN_vkResetCommandBuffer ResetCommandBuffer;
    PFN_vkQueueSubmit QueueSubmit;
    PFN_vkWaitForFences WaitForFences;
    PFN_vkResetFences ResetFences;
    PFN_vkCreateFence CreateFence;
    PFN_vkDestroyFence DestroyFence;
    PFN_vkDeviceWaitIdle DeviceWaitIdle;
    PFN_vkCreateSampler CreateSampler;
    PFN_vkDestroySampler DestroySampler;
} vk_gpu_functions;

typedef struct vk_gpu_group_range {
    br_uint_32 first_index;
    br_uint_32 index_count;
} vk_gpu_group_range;

typedef struct vk_gpu_material {
    br_colour colour;
    br_scalar opacity;
    br_boolean textured;
    br_boolean colour_key;
    br_boolean use_vertex_colour;
    br_uint_32 texture_width;
    br_uint_32 texture_height;
    struct br_buffer_stored* colour_map;
    br_matrix23 map_transform;
} vk_gpu_material;

typedef struct vk_gpu_texture_set {
    struct br_buffer_stored* stored;
    VkImageView view;
    br_uint_32 generation;
    br_uint_32 used_frame;
    VkDescriptorSet set;
} vk_gpu_texture_set;

typedef struct vk_gpu_retired_buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
} vk_gpu_retired_buffer;

struct vk_gpu_geometry {
    br_vk_video* video;
    br_uint_32 generation;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_memory;

    vk_gpu_group_range* ranges;
    br_uint_16 ngroups;
    br_uint_32 total_vertices;
    br_uint_32 total_faces;
    br_boolean ready;
};

struct vk_gpu_target {
    br_vk_video* video;
    br_uint_32 generation;

    struct br_device_pixelmap* pixelmap;
    br_uint_32 width;
    br_uint_32 height;

    VkImage colour_image;
    VkDeviceMemory colour_memory;
    VkImageView colour_view;
    VkImage depth_image;
    VkDeviceMemory depth_memory;
    VkImageView depth_view;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;

    VkShaderModule vertex_shader;
    VkShaderModule fragment_shader;
    VkDescriptorSetLayout set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    VkSampler sampler;
    VkImage white_image;
    VkDeviceMemory white_memory;
    VkImageView white_view;
    VkBuffer white_staging_buffer;
    VkDeviceMemory white_staging_memory;
    br_boolean white_ready;

    VkPipeline overlay_pipeline;
    VkPipelineLayout overlay_pipeline_layout;
    VkDescriptorSetLayout overlay_set_layout;
    VkDescriptorPool overlay_descriptor_pool;
    VkDescriptorSet overlay_descriptor_set;
    VkSampler overlay_sampler;
    br_uint_32 overlay_generation;

    VkImageLayout colour_layout;
    br_uint_32 frame_epoch;
    br_uint_32 overlay_epoch;
    br_uint_32 frame_draw_count;

    vk_gpu_texture_set texture_sets[VKREND_GPU_TEXTURE_SET_CAPACITY];
    br_uint_32 texture_set_count;
    VkDescriptorSet texture_set_pool[VKREND_GPU_TEXTURE_SET_CAPACITY];
    br_uint_32 texture_set_pool_count;
    br_uint_32 frame_sequence;

    vk_gpu_retired_buffer retired[VKREND_GPU_RETIRED_CAPACITY];
    br_uint_32 retired_count;

    VkBuffer ubo_buffer;
    VkDeviceMemory ubo_memory;
    br_uint_8* ubo_mapped;
    VkDeviceSize ubo_stride;
    br_uint_32 ubo_slot;

    br_boolean in_scene;
};

static vk_gpu_functions gpu;
static VkInstance gpu_instance;
static br_boolean gpu_loaded;
static br_boolean gpu_disabled;
static br_boolean gpu_targets_logged;
static br_boolean gpu_draw_logged;
static br_boolean gpu_texture_failure_logged;
static br_boolean gpu_texture_capacity_logged;
static br_boolean gpu_retired_full_logged;
static br_boolean gpu_frame_logged;

static void GpuVkLogMarker(const char* format, const char* text);

static void GpuVkRecordResult(const char* stage, VkResult result) {
    char text[160];

    BrSprintfN(text, sizeof(text) - 1, "vk failure at %s: VkResult %d", stage, (int)result);
    GpuVkLogMarker("%s", text);
}

static void GpuVkDisable(const char* stage) {
    if (gpu_disabled)
        return;

    gpu_disabled = BR_TRUE;
    BrLogPrintf("VKREND: gpu path failed at %s, falling back to record-only\n", (char*)stage);
    GpuVkLogMarker("VKREND: gpu path failed at %s, falling back to record-only", stage);
}

static br_boolean GpuVkHandleDeviceLost(br_vk_video* video, const char* stage, VkResult result) {
    GpuVkRecordResult(stage, result);

    if (result == VK_ERROR_DEVICE_LOST && VIDEOVkDeviceLost(video) == BRE_OK)
        return BR_TRUE;

    GpuVkDisable(stage);
    return BR_FALSE;
}

static br_boolean GpuVkLoadFunctions(VkInstance instance) {
    PFN_vkGetInstanceProcAddr gpa;

    if (gpu_loaded && gpu_instance == instance)
        return BR_TRUE;

    gpu_loaded = BR_FALSE;
    gpu_instance = instance;

    gpa = VIDEOVkGetInstanceProcAddr();
    if (gpa == NULL)
        return BR_FALSE;

#define VKREND_LOAD(name)                                                    \
    do {                                                                     \
        gpu.name = (PFN_vk##name)gpa(instance, "vk" #name);                  \
        if (gpu.name == NULL)                                                \
            return BR_FALSE;                                                 \
    } while (0)

    VKREND_LOAD(GetPhysicalDeviceFormatProperties);
    VKREND_LOAD(GetPhysicalDeviceProperties);
    VKREND_LOAD(GetPhysicalDeviceMemoryProperties);
    VKREND_LOAD(CreateImage);
    VKREND_LOAD(DestroyImage);
    VKREND_LOAD(GetImageMemoryRequirements);
    VKREND_LOAD(BindImageMemory);
    VKREND_LOAD(CreateImageView);
    VKREND_LOAD(DestroyImageView);
    VKREND_LOAD(CreateRenderPass);
    VKREND_LOAD(DestroyRenderPass);
    VKREND_LOAD(CreateFramebuffer);
    VKREND_LOAD(DestroyFramebuffer);
    VKREND_LOAD(CreateShaderModule);
    VKREND_LOAD(DestroyShaderModule);
    VKREND_LOAD(CreateDescriptorSetLayout);
    VKREND_LOAD(DestroyDescriptorSetLayout);
    VKREND_LOAD(CreateDescriptorPool);
    VKREND_LOAD(DestroyDescriptorPool);
    VKREND_LOAD(AllocateDescriptorSets);
    VKREND_LOAD(UpdateDescriptorSets);
    VKREND_LOAD(CreatePipelineLayout);
    VKREND_LOAD(DestroyPipelineLayout);
    VKREND_LOAD(CreateGraphicsPipelines);
    VKREND_LOAD(DestroyPipeline);
    VKREND_LOAD(CreateBuffer);
    VKREND_LOAD(DestroyBuffer);
    VKREND_LOAD(GetBufferMemoryRequirements);
    VKREND_LOAD(BindBufferMemory);
    VKREND_LOAD(AllocateMemory);
    VKREND_LOAD(FreeMemory);
    VKREND_LOAD(MapMemory);
    VKREND_LOAD(UnmapMemory);
    VKREND_LOAD(CmdPipelineBarrier);
    VKREND_LOAD(CmdBeginRenderPass);
    VKREND_LOAD(CmdEndRenderPass);
    VKREND_LOAD(CmdBindPipeline);
    VKREND_LOAD(CmdBindDescriptorSets);
    VKREND_LOAD(CmdSetViewport);
    VKREND_LOAD(CmdSetScissor);
    VKREND_LOAD(CmdClearColorImage);
    VKREND_LOAD(CmdCopyBufferToImage);
    VKREND_LOAD(CmdBindVertexBuffers);
    VKREND_LOAD(CmdBindIndexBuffer);
    VKREND_LOAD(CmdDraw);
    VKREND_LOAD(CmdDrawIndexed);
    VKREND_LOAD(BeginCommandBuffer);
    VKREND_LOAD(EndCommandBuffer);
    VKREND_LOAD(ResetCommandBuffer);
    VKREND_LOAD(QueueSubmit);
    VKREND_LOAD(WaitForFences);
    VKREND_LOAD(ResetFences);
    VKREND_LOAD(CreateFence);
    VKREND_LOAD(DestroyFence);
    VKREND_LOAD(DeviceWaitIdle);
    VKREND_LOAD(CreateSampler);
    VKREND_LOAD(DestroySampler);
#undef VKREND_LOAD

    gpu_loaded = BR_TRUE;
    return BR_TRUE;
}

static br_boolean GpuVkFindMemoryType(br_vk_video* video, br_uint_32 type_bits, VkMemoryPropertyFlags properties,
    br_uint_32* index) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    br_uint_32 i;

    gpu.GetPhysicalDeviceMemoryProperties(video->physical_device, &memory_properties);

    for (i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_bits & (1u << i)) != 0 && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            *index = i;
            return BR_TRUE;
        }
    }

    return BR_FALSE;
}

static void GpuVkDestroyBuffer(VkDevice device, VkBuffer* buffer, VkDeviceMemory* memory) {
    if (*buffer != VK_NULL_HANDLE) {
        gpu.DestroyBuffer(device, *buffer, NULL);
        *buffer = VK_NULL_HANDLE;
    }
    if (*memory != VK_NULL_HANDLE) {
        gpu.FreeMemory(device, *memory, NULL);
        *memory = VK_NULL_HANDLE;
    }
}

static void GpuVkDestroyImage(VkDevice device, VkImage* image, VkDeviceMemory* memory, VkImageView* view) {
    if (*view != VK_NULL_HANDLE) {
        gpu.DestroyImageView(device, *view, NULL);
        *view = VK_NULL_HANDLE;
    }
    if (*image != VK_NULL_HANDLE) {
        gpu.DestroyImage(device, *image, NULL);
        *image = VK_NULL_HANDLE;
    }
    if (*memory != VK_NULL_HANDLE) {
        gpu.FreeMemory(device, *memory, NULL);
        *memory = VK_NULL_HANDLE;
    }
}

static br_boolean GpuVkCreateBuffer(br_vk_video* video, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer,
    VkDeviceMemory* memory, void** mapped) {
    VkBufferCreateInfo buffer_info;
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocate_info;
    br_uint_32 memory_type;
    VkResult result;

    BrMemSet(&buffer_info, 0, sizeof(buffer_info));
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = gpu.CreateBuffer(video->device, &buffer_info, NULL, buffer);
    if (result != VK_SUCCESS)
        return BR_FALSE;

    gpu.GetBufferMemoryRequirements(video->device, *buffer, &requirements);
    if (!GpuVkFindMemoryType(video, requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &memory_type)
        && !GpuVkFindMemoryType(video, requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
            &memory_type)
        && !GpuVkFindMemoryType(video, requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memory_type))
        return BR_FALSE;

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = memory_type;

    result = gpu.AllocateMemory(video->device, &allocate_info, NULL, memory);
    if (result != VK_SUCCESS)
        return BR_FALSE;
    if (gpu.BindBufferMemory(video->device, *buffer, *memory, 0) != VK_SUCCESS)
        return BR_FALSE;

    if (mapped != NULL) {
        result = gpu.MapMemory(video->device, *memory, 0, VK_WHOLE_SIZE, 0, mapped);
        if (result != VK_SUCCESS)
            return BR_FALSE;
    }

    return BR_TRUE;
}

static br_boolean GpuVkCreateImage(br_vk_video* video, br_uint_32 width, br_uint_32 height, VkFormat format,
    VkImageUsageFlags usage, VkImageAspectFlags aspect, VkImage* image, VkDeviceMemory* memory, VkImageView* view) {
    VkImageCreateInfo image_info;
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocate_info;
    VkImageViewCreateInfo view_info;
    br_uint_32 memory_type;
    VkResult result;

    BrMemSet(&image_info, 0, sizeof(image_info));
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    result = gpu.CreateImage(video->device, &image_info, NULL, image);
    if (result != VK_SUCCESS)
        return BR_FALSE;

    gpu.GetImageMemoryRequirements(video->device, *image, &requirements);
    if (!GpuVkFindMemoryType(video, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_type))
        return BR_FALSE;

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = memory_type;

    result = gpu.AllocateMemory(video->device, &allocate_info, NULL, memory);
    if (result != VK_SUCCESS)
        return BR_FALSE;
    if (gpu.BindImageMemory(video->device, *image, *memory, 0) != VK_SUCCESS)
        return BR_FALSE;

    BrMemSet(&view_info, 0, sizeof(view_info));
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = *image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = aspect;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    if (gpu.CreateImageView(video->device, &view_info, NULL, view) != VK_SUCCESS)
        return BR_FALSE;

    return BR_TRUE;
}

static br_boolean GpuVkCreateSampler(vk_gpu_target* target) {
    VkSamplerCreateInfo sampler_info;

    BrMemSet(&sampler_info, 0, sizeof(sampler_info));
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.maxLod = 0.0f;

    return gpu.CreateSampler(target->video->device, &sampler_info, NULL, &target->sampler) == VK_SUCCESS ? BR_TRUE
                                                                                                         : BR_FALSE;
}

static br_boolean GpuVkCreateWhiteTexture(vk_gpu_target* target) {
    br_uint_8* mapped;

    if (!GpuVkCreateImage(target->video, 1, 1, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            &target->white_image, &target->white_memory, &target->white_view))
        return BR_FALSE;

    if (!GpuVkCreateBuffer(target->video, 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &target->white_staging_buffer,
            &target->white_staging_memory, (void**)&mapped))
        return BR_FALSE;

    mapped[0] = 255;
    mapped[1] = 255;
    mapped[2] = 255;
    mapped[3] = 255;

    gpu.UnmapMemory(target->video->device, target->white_staging_memory);

    return BR_TRUE;
}

static br_boolean GpuVkCreateShaderModule(VkDevice device, const uint32_t* code, br_size_t size, VkShaderModule* module) {
    VkShaderModuleCreateInfo module_info;

    BrMemSet(&module_info, 0, sizeof(module_info));
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = size;
    module_info.pCode = code;

    return gpu.CreateShaderModule(device, &module_info, NULL, module) == VK_SUCCESS ? BR_TRUE : BR_FALSE;
}

static br_boolean GpuVkCreateRenderPass(vk_gpu_target* target) {
    VkAttachmentDescription attachments[2];
    VkAttachmentReference colour_reference;
    VkAttachmentReference depth_reference;
    VkSubpassDescription subpass;
    VkSubpassDependency dependency;
    VkRenderPassCreateInfo render_pass_info;
    VkResult result;

    BrMemSet(attachments, 0, sizeof(attachments));
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = VK_FORMAT_D16_UNORM;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    BrMemSet(&colour_reference, 0, sizeof(colour_reference));
    colour_reference.attachment = 0;
    colour_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    BrMemSet(&depth_reference, 0, sizeof(depth_reference));
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    BrMemSet(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_reference;
    subpass.pDepthStencilAttachment = &depth_reference;

    BrMemSet(&dependency, 0, sizeof(dependency));
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    BrMemSet(&render_pass_info, 0, sizeof(render_pass_info));
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    result = gpu.CreateRenderPass(target->video->device, &render_pass_info, NULL, &target->render_pass);
    if (result != VK_SUCCESS)
        return BR_FALSE;

    {
        VkImageView views[2];
        VkFramebufferCreateInfo framebuffer_info;

        views[0] = target->colour_view;
        views[1] = target->depth_view;

        BrMemSet(&framebuffer_info, 0, sizeof(framebuffer_info));
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = target->render_pass;
        framebuffer_info.attachmentCount = 2;
        framebuffer_info.pAttachments = views;
        framebuffer_info.width = target->width;
        framebuffer_info.height = target->height;
        framebuffer_info.layers = 1;

        if (gpu.CreateFramebuffer(target->video->device, &framebuffer_info, NULL, &target->framebuffer) != VK_SUCCESS)
            return BR_FALSE;
    }

    return BR_TRUE;
}

static void GpuVkWriteSetDescriptors(vk_gpu_target* target, VkDescriptorSet set, VkImageView view) {
    VkDescriptorBufferInfo buffer_info;
    VkDescriptorImageInfo image_info;
    VkWriteDescriptorSet writes[2];

    BrMemSet(&buffer_info, 0, sizeof(buffer_info));
    buffer_info.buffer = target->ubo_buffer;
    buffer_info.offset = 0;
    buffer_info.range = VKREND_GPU_UBO_RANGE;

    BrMemSet(&image_info, 0, sizeof(image_info));
    image_info.sampler = target->sampler;
    image_info.imageView = view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    BrMemSet(writes, 0, sizeof(writes));
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = set;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[0].pBufferInfo = &buffer_info;
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = set;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &image_info;

    gpu.UpdateDescriptorSets(target->video->device, 2, writes, 0, NULL);
}

static br_boolean GpuVkCreateDescriptors(vk_gpu_target* target) {
    VkDevice device = target->video->device;
    VkDescriptorSetLayoutBinding bindings[2];
    VkDescriptorSetLayoutCreateInfo layout_info;
    VkDescriptorPoolSize pool_sizes[2];
    VkDescriptorPoolCreateInfo pool_info;
    VkDescriptorSetAllocateInfo allocate_info;
    br_uint_32 i;
    VkResult result;

    BrMemSet(bindings, 0, sizeof(bindings));
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    BrMemSet(&layout_info, 0, sizeof(layout_info));
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    if (gpu.CreateDescriptorSetLayout(device, &layout_info, NULL, &target->set_layout) != VK_SUCCESS)
        return BR_FALSE;

    BrMemSet(pool_sizes, 0, sizeof(pool_sizes));
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pool_sizes[0].descriptorCount = VKREND_GPU_TEXTURE_SET_CAPACITY + 1;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = VKREND_GPU_TEXTURE_SET_CAPACITY + 1;

    BrMemSet(&pool_info, 0, sizeof(pool_info));
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = VKREND_GPU_TEXTURE_SET_CAPACITY + 1;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;

    if (gpu.CreateDescriptorPool(device, &pool_info, NULL, &target->descriptor_pool) != VK_SUCCESS)
        return BR_FALSE;

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = target->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &target->set_layout;

    for (i = 0; i < VKREND_GPU_TEXTURE_SET_CAPACITY + 1; i++) {
        VkDescriptorSet set;

        result = gpu.AllocateDescriptorSets(device, &allocate_info, &set);
        if (result != VK_SUCCESS)
            return BR_FALSE;

        if (i == 0) {
            target->descriptor_set = set;
        } else {
            target->texture_set_pool[i - 1] = set;
        }
    }

    target->texture_set_pool_count = VKREND_GPU_TEXTURE_SET_CAPACITY;
    target->texture_set_count = 0;

    GpuVkWriteSetDescriptors(target, target->descriptor_set, target->white_view);

    return BR_TRUE;
}

static br_boolean GpuVkCreatePipeline(vk_gpu_target* target) {
    VkDevice device = target->video->device;
    VkPipelineShaderStageCreateInfo stages[2];
    VkPipelineVertexInputStateCreateInfo vertex_input;
    VkVertexInputBindingDescription vertex_binding;
    VkVertexInputAttributeDescription vertex_attributes[3];
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineMultisampleStateCreateInfo multisample;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    VkPipelineColorBlendAttachmentState blend_attachment;
    VkPipelineColorBlendStateCreateInfo blend;
    VkDynamicState dynamic_states[2];
    VkPipelineDynamicStateCreateInfo dynamic;
    VkPipelineLayoutCreateInfo layout_info;
    VkGraphicsPipelineCreateInfo pipeline_info;
    VkResult result;

    if (!GpuVkCreateShaderModule(device, VKREND_MODEL_VERT_SPV, sizeof(VKREND_MODEL_VERT_SPV), &target->vertex_shader))
        return BR_FALSE;
    if (!GpuVkCreateShaderModule(device, VKREND_MODEL_FRAG_SPV, sizeof(VKREND_MODEL_FRAG_SPV), &target->fragment_shader))
        return BR_FALSE;

    BrMemSet(stages, 0, sizeof(stages));
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = target->vertex_shader;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = target->fragment_shader;
    stages[1].pName = "main";

    BrMemSet(&vertex_binding, 0, sizeof(vertex_binding));
    vertex_binding.binding = 0;
    vertex_binding.stride = VKREND_GPU_VERTEX_STRIDE;
    vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    BrMemSet(vertex_attributes, 0, sizeof(vertex_attributes));
    vertex_attributes[0].location = 0;
    vertex_attributes[0].binding = 0;
    vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attributes[0].offset = 0;
    vertex_attributes[1].location = 1;
    vertex_attributes[1].binding = 0;
    vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attributes[1].offset = 12;
    vertex_attributes[2].location = 2;
    vertex_attributes[2].binding = 0;
    vertex_attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_attributes[2].offset = 20;

    BrMemSet(&vertex_input, 0, sizeof(vertex_input));
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &vertex_binding;
    vertex_input.vertexAttributeDescriptionCount = 3;
    vertex_input.pVertexAttributeDescriptions = vertex_attributes;

    BrMemSet(&input_assembly, 0, sizeof(input_assembly));
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
    BrMemSet(&dynamic, 0, sizeof(dynamic));
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;

    BrMemSet(&viewport_state, 0, sizeof(viewport_state));
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    BrMemSet(&rasterization, 0, sizeof(rasterization));
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0f;

    BrMemSet(&multisample, 0, sizeof(multisample));
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    BrMemSet(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    BrMemSet(&blend_attachment, 0, sizeof(blend_attachment));
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    BrMemSet(&blend, 0, sizeof(blend));
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attachment;

    BrMemSet(&layout_info, 0, sizeof(layout_info));
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &target->set_layout;

    result = gpu.CreatePipelineLayout(device, &layout_info, NULL, &target->pipeline_layout);
    if (result != VK_SUCCESS)
        return BR_FALSE;

    BrMemSet(&pipeline_info, 0, sizeof(pipeline_info));
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &blend;
    pipeline_info.pDynamicState = &dynamic;
    pipeline_info.layout = target->pipeline_layout;
    pipeline_info.renderPass = target->render_pass;
    pipeline_info.subpass = 0;

    result = gpu.CreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &target->pipeline);
    if (result != VK_SUCCESS)
        return BR_FALSE;

    return BR_TRUE;
}

static void GpuVkWriteOverlayDescriptor(vk_gpu_target* target, VkImageView view) {
    VkDescriptorImageInfo image_info;
    VkWriteDescriptorSet write;

    if (target->overlay_descriptor_set == VK_NULL_HANDLE || view == VK_NULL_HANDLE)
        return;

    BrMemSet(&image_info, 0, sizeof(image_info));
    image_info.sampler = target->overlay_sampler;
    image_info.imageView = view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    BrMemSet(&write, 0, sizeof(write));
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = target->overlay_descriptor_set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image_info;

    gpu.UpdateDescriptorSets(target->video->device, 1, &write, 0, NULL);

    target->overlay_generation = target->video->overlay_generation;
}

static br_boolean GpuVkCreateOverlayPipeline(vk_gpu_target* target) {
    VkDevice device = target->video->device;
    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    VkPipelineShaderStageCreateInfo stages[2];
    VkPipelineVertexInputStateCreateInfo vertex_input;
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineMultisampleStateCreateInfo multisample;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    VkPipelineColorBlendAttachmentState blend_attachment;
    VkPipelineColorBlendStateCreateInfo blend;
    VkDynamicState dynamic_states[2];
    VkPipelineDynamicStateCreateInfo dynamic;
    VkDescriptorSetLayoutBinding binding;
    VkDescriptorSetLayoutCreateInfo set_info;
    VkDescriptorPoolSize pool_size;
    VkDescriptorPoolCreateInfo pool_info;
    VkDescriptorSetAllocateInfo allocate_info;
    VkPipelineLayoutCreateInfo layout_info;
    VkGraphicsPipelineCreateInfo pipeline_info;
    VkSamplerCreateInfo sampler_info;
    VkResult result;

    vertex_module = VK_NULL_HANDLE;
    fragment_module = VK_NULL_HANDLE;

    if (!GpuVkCreateShaderModule(device, VKREND_PRESENT_VERT_SPV, sizeof(VKREND_PRESENT_VERT_SPV), &vertex_module))
        return BR_FALSE;
    if (!GpuVkCreateShaderModule(device, VKREND_OVERLAY_FRAG_SPV, sizeof(VKREND_OVERLAY_FRAG_SPV), &fragment_module)) {
        gpu.DestroyShaderModule(device, vertex_module, NULL);
        return BR_FALSE;
    }

    BrMemSet(&binding, 0, sizeof(binding));
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    BrMemSet(&set_info, 0, sizeof(set_info));
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_info.bindingCount = 1;
    set_info.pBindings = &binding;

    result = gpu.CreateDescriptorSetLayout(device, &set_info, NULL, &target->overlay_set_layout);
    if (result != VK_SUCCESS)
        goto fail_modules;

    BrMemSet(&pool_size, 0, sizeof(pool_size));
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = 1;

    BrMemSet(&pool_info, 0, sizeof(pool_info));
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    result = gpu.CreateDescriptorPool(device, &pool_info, NULL, &target->overlay_descriptor_pool);
    if (result != VK_SUCCESS)
        goto fail_set_layout;

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = target->overlay_descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &target->overlay_set_layout;

    result = gpu.AllocateDescriptorSets(device, &allocate_info, &target->overlay_descriptor_set);
    if (result != VK_SUCCESS)
        goto fail_pool;

    BrMemSet(stages, 0, sizeof(stages));
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex_module;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment_module;
    stages[1].pName = "main";

    BrMemSet(&vertex_input, 0, sizeof(vertex_input));
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    BrMemSet(&input_assembly, 0, sizeof(input_assembly));
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
    BrMemSet(&dynamic, 0, sizeof(dynamic));
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;

    BrMemSet(&viewport_state, 0, sizeof(viewport_state));
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    BrMemSet(&rasterization, 0, sizeof(rasterization));
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0f;

    BrMemSet(&multisample, 0, sizeof(multisample));
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    BrMemSet(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_FALSE;
    depth_stencil.depthWriteEnable = VK_FALSE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    BrMemSet(&blend_attachment, 0, sizeof(blend_attachment));
    blend_attachment.blendEnable = VK_FALSE;
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    BrMemSet(&blend, 0, sizeof(blend));
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attachment;

    BrMemSet(&layout_info, 0, sizeof(layout_info));
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &target->overlay_set_layout;

    result = gpu.CreatePipelineLayout(device, &layout_info, NULL, &target->overlay_pipeline_layout);
    if (result != VK_SUCCESS)
        goto fail_pool;

    BrMemSet(&pipeline_info, 0, sizeof(pipeline_info));
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &blend;
    pipeline_info.pDynamicState = &dynamic;
    pipeline_info.layout = target->overlay_pipeline_layout;
    pipeline_info.renderPass = target->render_pass;
    pipeline_info.subpass = 0;

    result = gpu.CreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &target->overlay_pipeline);
    if (result != VK_SUCCESS)
        goto fail_pipeline_layout;

    BrMemSet(&sampler_info, 0, sizeof(sampler_info));
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.maxLod = 0.0f;

    result = gpu.CreateSampler(device, &sampler_info, NULL, &target->overlay_sampler);
    if (result != VK_SUCCESS)
        goto fail_pipeline;

    gpu.DestroyShaderModule(device, fragment_module, NULL);
    gpu.DestroyShaderModule(device, vertex_module, NULL);

    GpuVkWriteOverlayDescriptor(target, target->video->overlay_view);

    return BR_TRUE;

fail_pipeline:
    if (target->overlay_pipeline != VK_NULL_HANDLE) {
        gpu.DestroyPipeline(device, target->overlay_pipeline, NULL);
        target->overlay_pipeline = VK_NULL_HANDLE;
    }
fail_pipeline_layout:
    if (target->overlay_pipeline_layout != VK_NULL_HANDLE) {
        gpu.DestroyPipelineLayout(device, target->overlay_pipeline_layout, NULL);
        target->overlay_pipeline_layout = VK_NULL_HANDLE;
    }
fail_pool:
    if (target->overlay_descriptor_pool != VK_NULL_HANDLE) {
        gpu.DestroyDescriptorPool(device, target->overlay_descriptor_pool, NULL);
        target->overlay_descriptor_pool = VK_NULL_HANDLE;
        target->overlay_descriptor_set = VK_NULL_HANDLE;
    }
fail_set_layout:
    if (target->overlay_set_layout != VK_NULL_HANDLE) {
        gpu.DestroyDescriptorSetLayout(device, target->overlay_set_layout, NULL);
        target->overlay_set_layout = VK_NULL_HANDLE;
    }
fail_modules:
    gpu.DestroyShaderModule(device, fragment_module, NULL);
    gpu.DestroyShaderModule(device, vertex_module, NULL);
    return BR_FALSE;
}

static void GpuVkRetireFlush(vk_gpu_target* target) {
    VkDevice device = target->video->device;
    br_uint_32 i;

    for (i = 0; i < target->retired_count; i++)
        GpuVkDestroyBuffer(device, &target->retired[i].buffer, &target->retired[i].memory);

    target->retired_count = 0;
}

static void GpuVkRetireGeometry(vk_gpu_target* target, vk_gpu_geometry* geometry) {
    target->retired[target->retired_count].buffer = geometry->vertex_buffer;
    target->retired[target->retired_count].memory = geometry->vertex_memory;
    target->retired_count++;
    target->retired[target->retired_count].buffer = geometry->index_buffer;
    target->retired[target->retired_count].memory = geometry->index_memory;
    target->retired_count++;

    geometry->vertex_buffer = VK_NULL_HANDLE;
    geometry->vertex_memory = VK_NULL_HANDLE;
    geometry->index_buffer = VK_NULL_HANDLE;
    geometry->index_memory = VK_NULL_HANDLE;
}

typedef struct vk_deferred_release {
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkImage image;
    VkImageView view;
} vk_deferred_release;

static vk_deferred_release gpu_deferred[VKREND_GPU_RETIRED_CAPACITY];
static br_uint_32 gpu_deferred_count;

void GpuVkDeferBuffer(VkDevice device, VkBuffer buffer, VkDeviceMemory memory) {
    if (buffer == VK_NULL_HANDLE && memory == VK_NULL_HANDLE)
        return;

    if (gpu_deferred_count >= VKREND_GPU_RETIRED_CAPACITY)
        return;

    BrMemSet(&gpu_deferred[gpu_deferred_count], 0, sizeof(gpu_deferred[gpu_deferred_count]));
    gpu_deferred[gpu_deferred_count].device = device;
    gpu_deferred[gpu_deferred_count].buffer = buffer;
    gpu_deferred[gpu_deferred_count].memory = memory;
    gpu_deferred_count++;
}

void GpuVkDeferImage(VkDevice device, VkImage image, VkDeviceMemory memory, VkImageView view) {
    if (image == VK_NULL_HANDLE && memory == VK_NULL_HANDLE && view == VK_NULL_HANDLE)
        return;

    if (gpu_deferred_count >= VKREND_GPU_RETIRED_CAPACITY)
        return;

    BrMemSet(&gpu_deferred[gpu_deferred_count], 0, sizeof(gpu_deferred[gpu_deferred_count]));
    gpu_deferred[gpu_deferred_count].device = device;
    gpu_deferred[gpu_deferred_count].image = image;
    gpu_deferred[gpu_deferred_count].memory = memory;
    gpu_deferred[gpu_deferred_count].view = view;
    gpu_deferred_count++;
}

static void GpuVkFlushDeferred(void) {
    br_uint_32 i;

    for (i = 0; i < gpu_deferred_count; i++) {
        VkDevice device = gpu_deferred[i].device;
        VkBuffer buffer = gpu_deferred[i].buffer;
        VkDeviceMemory memory = gpu_deferred[i].memory;

        if (gpu_deferred[i].image != VK_NULL_HANDLE || gpu_deferred[i].view != VK_NULL_HANDLE) {
            if (gpu_deferred[i].view != VK_NULL_HANDLE)
                gpu.DestroyImageView(device, gpu_deferred[i].view, NULL);
            if (gpu_deferred[i].image != VK_NULL_HANDLE)
                gpu.DestroyImage(device, gpu_deferred[i].image, NULL);
            if (memory != VK_NULL_HANDLE)
                gpu.FreeMemory(device, memory, NULL);
        } else {
            GpuVkDestroyBuffer(device, &buffer, &memory);
        }
    }

    gpu_deferred_count = 0;
}

void GpuVkResetDeferred(void) {
    gpu_deferred_count = 0;
}

void GpuVkResetAfterRecovery(void) {
    gpu_disabled = BR_FALSE;
    gpu_loaded = BR_FALSE;
    gpu_instance = VK_NULL_HANDLE;
    gpu_targets_logged = BR_FALSE;
    gpu_draw_logged = BR_FALSE;
    gpu_texture_failure_logged = BR_FALSE;
    gpu_texture_capacity_logged = BR_FALSE;
    gpu_retired_full_logged = BR_FALSE;
    gpu_frame_logged = BR_FALSE;
}

static void GpuVkTargetCleanup(vk_gpu_target* target) {
    VkDevice device = target->video->device;

    GpuVkRetireFlush(target);

    if (target->overlay_pipeline != VK_NULL_HANDLE) {
        gpu.DestroyPipeline(device, target->overlay_pipeline, NULL);
        target->overlay_pipeline = VK_NULL_HANDLE;
    }
    if (target->overlay_pipeline_layout != VK_NULL_HANDLE) {
        gpu.DestroyPipelineLayout(device, target->overlay_pipeline_layout, NULL);
        target->overlay_pipeline_layout = VK_NULL_HANDLE;
    }
    if (target->overlay_descriptor_pool != VK_NULL_HANDLE) {
        gpu.DestroyDescriptorPool(device, target->overlay_descriptor_pool, NULL);
        target->overlay_descriptor_pool = VK_NULL_HANDLE;
        target->overlay_descriptor_set = VK_NULL_HANDLE;
    }
    if (target->overlay_set_layout != VK_NULL_HANDLE) {
        gpu.DestroyDescriptorSetLayout(device, target->overlay_set_layout, NULL);
        target->overlay_set_layout = VK_NULL_HANDLE;
    }
    if (target->overlay_sampler != VK_NULL_HANDLE) {
        gpu.DestroySampler(device, target->overlay_sampler, NULL);
        target->overlay_sampler = VK_NULL_HANDLE;
    }
    if (target->pipeline != VK_NULL_HANDLE) {
        gpu.DestroyPipeline(device, target->pipeline, NULL);
        target->pipeline = VK_NULL_HANDLE;
    }
    if (target->pipeline_layout != VK_NULL_HANDLE) {
        gpu.DestroyPipelineLayout(device, target->pipeline_layout, NULL);
        target->pipeline_layout = VK_NULL_HANDLE;
    }
    if (target->descriptor_pool != VK_NULL_HANDLE) {
        gpu.DestroyDescriptorPool(device, target->descriptor_pool, NULL);
        target->descriptor_pool = VK_NULL_HANDLE;
        target->descriptor_set = VK_NULL_HANDLE;
    }
    if (target->set_layout != VK_NULL_HANDLE) {
        gpu.DestroyDescriptorSetLayout(device, target->set_layout, NULL);
        target->set_layout = VK_NULL_HANDLE;
    }
    if (target->sampler != VK_NULL_HANDLE) {
        gpu.DestroySampler(device, target->sampler, NULL);
        target->sampler = VK_NULL_HANDLE;
    }
    if (target->fragment_shader != VK_NULL_HANDLE) {
        gpu.DestroyShaderModule(device, target->fragment_shader, NULL);
        target->fragment_shader = VK_NULL_HANDLE;
    }
    if (target->vertex_shader != VK_NULL_HANDLE) {
        gpu.DestroyShaderModule(device, target->vertex_shader, NULL);
        target->vertex_shader = VK_NULL_HANDLE;
    }

    GpuVkDestroyBuffer(device, &target->white_staging_buffer, &target->white_staging_memory);
    GpuVkDestroyBuffer(device, &target->ubo_buffer, &target->ubo_memory);
    target->ubo_mapped = NULL;

    if (target->framebuffer != VK_NULL_HANDLE) {
        gpu.DestroyFramebuffer(device, target->framebuffer, NULL);
        target->framebuffer = VK_NULL_HANDLE;
    }
    if (target->render_pass != VK_NULL_HANDLE) {
        gpu.DestroyRenderPass(device, target->render_pass, NULL);
        target->render_pass = VK_NULL_HANDLE;
    }

    GpuVkDestroyImage(device, &target->white_image, &target->white_memory, &target->white_view);
    GpuVkDestroyImage(device, &target->depth_image, &target->depth_memory, &target->depth_view);
    GpuVkDestroyImage(device, &target->colour_image, &target->colour_memory, &target->colour_view);
}

void GpuVkTargetDestroy(vk_gpu_target** ptarget) {
    vk_gpu_target* target = *ptarget;

    if (target == NULL)
        return;

    if (target->video != NULL && target->video->present_target == target) {
        target->video->present_target = NULL;
        target->video->frame_has_scene = BR_FALSE;
    }

    if (target->video != NULL && target->video->scene_recording) {
        if (target->in_scene) {
            target->video->scene_recording = BR_FALSE;
            GpuVkDisable("target destroyed during scene");
        } else {
            GpuVkFrameEnd(target->video);
        }
    }

    if (target->video != NULL && target->video->active && target->video->generation == target->generation) {
        if (gpu.DeviceWaitIdle != NULL)
            gpu.DeviceWaitIdle(target->video->device);
        GpuVkTargetCleanup(target);
    }

    BrMemFree(target);
    *ptarget = NULL;
}

static br_boolean GpuVkTargetInit(vk_gpu_target* target) {
    br_vk_video* video = target->video;
    VkPhysicalDeviceProperties properties;
    VkFormatProperties format_properties;
    VkFormatFeatureFlags colour_required = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
        | VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    VkFormatFeatureFlags depth_required = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkDeviceSize ubo_size;

    target->colour_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    gpu.GetPhysicalDeviceFormatProperties(video->physical_device, VK_FORMAT_R8G8B8A8_UNORM, &format_properties);
    if ((format_properties.optimalTilingFeatures & colour_required) != colour_required)
        return BR_FALSE;

    gpu.GetPhysicalDeviceFormatProperties(video->physical_device, VK_FORMAT_D16_UNORM, &format_properties);
    if ((format_properties.optimalTilingFeatures & depth_required) != depth_required)
        return BR_FALSE;

    if (!GpuVkCreateImage(video, target->width, target->height, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, &target->colour_image, &target->colour_memory, &target->colour_view))
        return BR_FALSE;

    if (!GpuVkCreateImage(video, target->width, target->height, VK_FORMAT_D16_UNORM,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, &target->depth_image,
            &target->depth_memory, &target->depth_view))
        return BR_FALSE;

    if (!GpuVkCreateSampler(target))
        return BR_FALSE;

    if (!GpuVkCreateWhiteTexture(target))
        return BR_FALSE;

    gpu.GetPhysicalDeviceProperties(video->physical_device, &properties);
    target->ubo_stride = VKREND_GPU_UBO_STRIDE;
    if (properties.limits.minUniformBufferOffsetAlignment > target->ubo_stride)
        target->ubo_stride = properties.limits.minUniformBufferOffsetAlignment;

    ubo_size = target->ubo_stride * VKREND_GPU_UBO_SLOTS;
    if (!GpuVkCreateBuffer(video, ubo_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &target->ubo_buffer,
            &target->ubo_memory, (void**)&target->ubo_mapped))
        return BR_FALSE;

    if (!GpuVkCreateRenderPass(target))
        return BR_FALSE;

    if (!GpuVkCreateDescriptors(target))
        return BR_FALSE;

    if (!GpuVkCreatePipeline(target))
        return BR_FALSE;

    if (!GpuVkCreateOverlayPipeline(target))
        return BR_FALSE;

    return BR_TRUE;
}

static br_boolean GpuVkTargetUsable(vk_gpu_target* target, br_vk_video* video, br_uint_32 width, br_uint_32 height) {
    return target != NULL && target->video == video && target->generation == video->generation
            && target->width == width && target->height == height;
}

static void GpuVkProjection(const br_matrix4* source, float* destination) {
    int row;

    for (row = 0; row < 4; row++) {
        float x = source->m[row][0];
        float y = source->m[row][1];
        float z = source->m[row][2];
        float w = source->m[row][3];

        destination[row * 4 + 0] = x;
        destination[row * 4 + 1] = -y;
        destination[row * 4 + 2] = 0.5f * w - 0.5f * z;
        destination[row * 4 + 3] = w;
    }
}

static void GpuVkWriteUbo(vk_gpu_target* target, br_uint_32 slot, br_renderer* renderer,
    const vk_gpu_material* material) {
    const vk_state_matrix* matrix = &renderer->state.current->matrix;
    br_matrix4 model_to_view;
    float* destination = (float*)(target->ubo_mapped + (br_size_t)slot * target->ubo_stride);

    GpuVkProjection(&matrix->view_to_screen, destination);
    BrMatrix4Copy34(&model_to_view, (br_matrix34*)&matrix->model_to_view);
    BrMemCpy(destination + 16, &model_to_view, sizeof(model_to_view));

    destination[32] = (float)BR_RED(material->colour) / 255.0f;
    destination[33] = (float)BR_GRN(material->colour) / 255.0f;
    destination[34] = (float)BR_BLU(material->colour) / 255.0f;
    destination[35] = 1.0f;

    destination[36] = material->textured ? 1.0f : 0.0f;
    destination[37] = (float)material->opacity;
    destination[38] = material->colour_key ? 1.0f : 0.0f;
    destination[39] = material->use_vertex_colour ? 1.0f : 0.0f;

    destination[40] = (float)material->map_transform.m[0][0];
    destination[41] = (float)material->map_transform.m[1][0];
    destination[42] = (float)material->map_transform.m[2][0];
    destination[43] = 0.0f;
    destination[44] = (float)material->map_transform.m[0][1];
    destination[45] = (float)material->map_transform.m[1][1];
    destination[46] = (float)material->map_transform.m[2][1];
    destination[47] = 0.0f;
}

static void GpuVkLogMarker(const char* format, const char* text) {
    FILE* file = fopen("vkrend_markers.txt", "a");

    if (file != NULL) {
        fprintf(file, format, text);
        fprintf(file, "\n");
        fclose(file);
    }
}

static void GpuVkMaterialResolve(const struct v11model* model, br_uint_16 group_index,
    struct br_renderer_state_stored* default_state, vk_gpu_material* material) {
    struct br_renderer_state_stored* group_stored = NULL;
    struct br_renderer_state_stored* surface_state = NULL;
    struct br_renderer_state_stored* primitive_state = NULL;

    if (model != NULL && model->groups != NULL && group_index < model->ngroups)
        group_stored = (struct br_renderer_state_stored*)model->groups[group_index].stored;

    if (group_stored != NULL && (group_stored->state.valid & VK_MASK_STATE_SURFACE) != 0)
        surface_state = group_stored;
    else if (default_state != NULL && (default_state->state.valid & VK_MASK_STATE_SURFACE) != 0)
        surface_state = default_state;
    else if (group_stored != NULL)
        surface_state = group_stored;
    else
        surface_state = default_state;

    if (group_stored != NULL && (group_stored->state.valid & VK_MASK_STATE_PRIMITIVE) != 0)
        primitive_state = group_stored;
    else if (default_state != NULL && (default_state->state.valid & VK_MASK_STATE_PRIMITIVE) != 0)
        primitive_state = default_state;
    else if (group_stored != NULL)
        primitive_state = group_stored;
    else
        primitive_state = default_state;

    if (surface_state != NULL) {
        material->colour = surface_state->state.surface.colour;
        material->opacity = surface_state->state.surface.opacity;
        material->map_transform = surface_state->state.surface.map_transform;
        material->use_vertex_colour =
            surface_state->state.surface.lighting && surface_state->state.surface.prelighting;
    } else {
        material->colour = BR_COLOUR_RGB(255, 255, 255);
        material->opacity = BR_SCALAR(1.0);
        material->use_vertex_colour = BR_FALSE;
        material->map_transform.m[0][0] = BR_SCALAR(1.0);
        material->map_transform.m[0][1] = BR_SCALAR(0.0);
        material->map_transform.m[0][2] = BR_SCALAR(0.0);
        material->map_transform.m[1][0] = BR_SCALAR(0.0);
        material->map_transform.m[1][1] = BR_SCALAR(1.0);
        material->map_transform.m[1][2] = BR_SCALAR(0.0);
    }

    if (material->opacity < BR_SCALAR(0.0))
        material->opacity = BR_SCALAR(0.0);
    if (material->opacity > BR_SCALAR(1.0))
        material->opacity = BR_SCALAR(1.0);

    material->colour_map = primitive_state != NULL ? primitive_state->state.prim.colour_map : NULL;
    material->colour_key = primitive_state != NULL && (primitive_state->state.prim.flags & VK_PRIMF_COLOUR_KEY) != 0;
    material->textured = BR_FALSE;
    material->texture_width = 0;
    material->texture_height = 0;
}

static VkDescriptorSet GpuVkMaterialDescriptor(vk_gpu_target* target, vk_gpu_material* material) {
    vk_buffer_texture texture;
    br_uint_32 i;

    if (material->colour_map == NULL)
        return target->descriptor_set;

    BrMemSet(&texture, 0, sizeof(texture));
    if (!BufferStoredVkGetTexture(material->colour_map, target->video, &texture) || texture.view == VK_NULL_HANDLE) {
        if (!gpu_texture_failure_logged) {
            gpu_texture_failure_logged = BR_TRUE;
            GpuVkLogMarker("%s", "VKREND: colour map has no gpu texture, using white");
        }
        return target->descriptor_set;
    }

    for (i = 0; i < target->texture_set_count; i++) {
        if (target->texture_sets[i].stored == material->colour_map
            && target->texture_sets[i].view == texture.view
            && target->texture_sets[i].generation == texture.generation) {
            target->texture_sets[i].used_frame = target->frame_sequence;
            target->texture_sets[i].generation = texture.generation;

            material->textured = BR_TRUE;
            material->texture_width = texture.width;
            material->texture_height = texture.height;
            return target->texture_sets[i].set;
        }
    }

    if (target->texture_set_pool_count == 0) {
        for (i = 0; i < target->texture_set_count;) {
            if (target->texture_sets[i].used_frame == target->frame_sequence) {
                i++;
                continue;
            }

            target->texture_set_pool[target->texture_set_pool_count++] = target->texture_sets[i].set;
            target->texture_sets[i] = target->texture_sets[target->texture_set_count - 1];
            target->texture_set_count--;
        }
    }

    if (target->texture_set_pool_count == 0) {
        if (!gpu_texture_capacity_logged) {
            gpu_texture_capacity_logged = BR_TRUE;
            GpuVkLogMarker("%s", "VKREND: texture descriptor cache full, using white");
        }
        return target->descriptor_set;
    }

    {
        vk_gpu_texture_set* entry = &target->texture_sets[target->texture_set_count];

        entry->stored = material->colour_map;
        entry->view = texture.view;
        entry->generation = texture.generation;
        entry->used_frame = target->frame_sequence;
        entry->set = target->texture_set_pool[--target->texture_set_pool_count];

        GpuVkWriteSetDescriptors(target, entry->set, texture.view);
        target->texture_set_count++;
    }

    material->textured = BR_TRUE;
    material->texture_width = texture.width;
    material->texture_height = texture.height;
    return target->texture_sets[target->texture_set_count - 1].set;
}

static void GpuVkUploadWhiteTexture(vk_gpu_target* target, VkCommandBuffer command_buffer) {
    VkBufferMemoryBarrier buffer_barrier;
    VkImageMemoryBarrier image_barrier;
    VkBufferImageCopy region;

    if (target->white_ready)
        return;

    BrMemSet(&buffer_barrier, 0, sizeof(buffer_barrier));
    buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buffer_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    buffer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buffer_barrier.buffer = target->white_staging_buffer;
    buffer_barrier.offset = 0;
    buffer_barrier.size = VK_WHOLE_SIZE;

    gpu.CmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1,
        &buffer_barrier, 0, NULL);

    BrMemSet(&image_barrier, 0, sizeof(image_barrier));
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = target->white_image;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.layerCount = 1;

    gpu.CmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL,
        0, NULL, 1, &image_barrier);

    BrMemSet(&region, 0, sizeof(region));
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = 1;
    region.imageExtent.height = 1;
    region.imageExtent.depth = 1;

    gpu.CmdCopyBufferToImage(command_buffer, target->white_staging_buffer, target->white_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    BrMemSet(&image_barrier, 0, sizeof(image_barrier));
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = target->white_image;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.layerCount = 1;

    gpu.CmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
        NULL, 0, NULL, 1, &image_barrier);

    target->white_ready = BR_TRUE;
}

static void GpuVkClearTargetColour(vk_gpu_target* target, VkCommandBuffer command_buffer) {
    VkImageMemoryBarrier barrier;
    VkClearColorValue clear;
    VkImageSubresourceRange range;
    VkPipelineStageFlags source_stage;
    VkAccessFlags source_access;

    if (target->colour_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        source_access = 0;
    } else if (target->colour_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        source_access = VK_ACCESS_SHADER_READ_BIT;
    } else if (target->colour_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        source_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else {
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        source_access = VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    BrMemSet(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = target->colour_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = source_access;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target->colour_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    gpu.CmdPipelineBarrier(command_buffer, source_stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
        &barrier);

    BrMemSet(&clear, 0, sizeof(clear));
    clear.float32[3] = 1.0f;

    BrMemSet(&range, 0, sizeof(range));
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.levelCount = 1;
    range.layerCount = 1;

    gpu.CmdClearColorImage(command_buffer, target->colour_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1,
        &range);

    BrMemSet(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target->colour_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    gpu.CmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, NULL, 0, NULL, 1, &barrier);

    target->colour_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

static br_boolean GpuVkUploadOverlayUnder(vk_gpu_target* target, VkCommandBuffer command_buffer) {
    br_vk_video* video = target->video;
    struct br_device_pixelmap* pixelmap = target->pixelmap;
    br_int_32 row_bytes;
    br_size_t base;

    if (pixelmap == NULL || pixelmap->shadow == NULL)
        return BR_FALSE;
    if (target->width != video->overlay_width || target->height != video->overlay_height)
        return BR_FALSE;

    row_bytes = pixelmap->pm_row_bytes;
    if (row_bytes < 0)
        row_bytes = -row_bytes;

    base = (br_size_t)pixelmap->pm_base_y * (br_size_t)row_bytes + (br_size_t)pixelmap->pm_base_x * 2;

    VIDEOVkOverlayUpload(video, command_buffer, (const br_uint_8*)pixelmap->shadow, target->width, target->height,
        row_bytes, base);

    if (target->overlay_generation != video->overlay_generation)
        GpuVkWriteOverlayDescriptor(target, video->overlay_view);

    GpuVkShadowResetKey(pixelmap);

    return BR_TRUE;
}

static br_boolean GpuVkBeginScene(vk_gpu_target* target) {
    br_vk_video* video = target->video;
    VkCommandBuffer command_buffer = video->scene_command_buffer;
    VkCommandBufferBeginInfo begin_info;
    VkRenderPassBeginInfo render_pass_info;
    VkClearValue clear_values[2];
    VkViewport viewport;
    VkRect2D scissor;
    br_boolean overlay_drawn;
    VkResult result;

    if (command_buffer == VK_NULL_HANDLE)
        return BR_FALSE;

    if (!video->scene_recording) {
        if (video->scene_fence_pending) {
            result = gpu.WaitForFences(video->device, 1, &video->scene_fence, VK_TRUE, UINT64_MAX);
            if (result != VK_SUCCESS) {
                GpuVkHandleDeviceLost(video, "scene fence wait", result);
                return BR_FALSE;
            }

            result = gpu.ResetFences(video->device, 1, &video->scene_fence);
            if (result != VK_SUCCESS) {
                GpuVkHandleDeviceLost(video, "scene fence reset", result);
                return BR_FALSE;
            }

            video->scene_fence_pending = BR_FALSE;
        }

        result = gpu.WaitForFences(video->device, 1, &video->fence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS) {
            GpuVkHandleDeviceLost(video, "present fence wait", result);
            return BR_FALSE;
        }

        GpuVkFlushDeferred();
        video->pixel_slot = 0;

        result = gpu.ResetCommandBuffer(command_buffer, 0);
        if (result != VK_SUCCESS) {
            GpuVkHandleDeviceLost(video, "scene command buffer reset", result);
            return BR_FALSE;
        }

        GpuVkRetireFlush(target);

        BrMemSet(&begin_info, 0, sizeof(begin_info));
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = gpu.BeginCommandBuffer(command_buffer, &begin_info);
        if (result != VK_SUCCESS) {
            GpuVkHandleDeviceLost(video, "scene command buffer begin", result);
            return BR_FALSE;
        }

        video->scene_recording = BR_TRUE;
        video->frame_epoch++;
        video->frame_has_scene = BR_FALSE;
        video->present_target = NULL;
    }

    if (target->frame_epoch != video->frame_epoch) {
        target->frame_epoch = video->frame_epoch;
        target->ubo_slot = 0;
        target->frame_draw_count = 0;
        GpuVkClearTargetColour(target, command_buffer);
    }

    overlay_drawn = BR_FALSE;
    if (target->overlay_epoch != video->frame_epoch) {
        target->overlay_epoch = video->frame_epoch;
        overlay_drawn = GpuVkUploadOverlayUnder(target, command_buffer);
    }

    GpuVkUploadWhiteTexture(target, command_buffer);

    BrMemSet(clear_values, 0, sizeof(clear_values));
    clear_values[1].depthStencil.depth = 1.0f;

    BrMemSet(&render_pass_info, 0, sizeof(render_pass_info));
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = target->render_pass;
    render_pass_info.framebuffer = target->framebuffer;
    render_pass_info.renderArea.extent.width = target->width;
    render_pass_info.renderArea.extent.height = target->height;
    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = clear_values;

    gpu.CmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)target->width;
    viewport.height = (float)target->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    gpu.CmdSetViewport(command_buffer, 0, 1, &viewport);

    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = target->width;
    scissor.extent.height = target->height;
    gpu.CmdSetScissor(command_buffer, 0, 1, &scissor);

    if (overlay_drawn && target->overlay_pipeline != VK_NULL_HANDLE
        && target->overlay_descriptor_set != VK_NULL_HANDLE) {
        gpu.CmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, target->overlay_pipeline);
        gpu.CmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, target->overlay_pipeline_layout, 0, 1,
            &target->overlay_descriptor_set, 0, NULL);
        gpu.CmdDraw(command_buffer, 3, 1, 0, 0);
    }

    gpu.CmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, target->pipeline);

    target->frame_sequence++;
    target->in_scene = BR_TRUE;
    return BR_TRUE;
}

static br_boolean GpuVkEndScene(vk_gpu_target* target) {
    br_vk_video* video = target->video;

    if (video->scene_command_buffer == VK_NULL_HANDLE || !video->scene_recording)
        return BR_FALSE;

    gpu.CmdEndRenderPass(video->scene_command_buffer);

    return BR_TRUE;
}

void GpuVkFrameEnd(br_vk_video* video) {
    VkSubmitInfo submit_info;
    VkResult result;

    if (video == NULL || !video->scene_recording)
        return;

    video->scene_recording = BR_FALSE;

    if (gpu_disabled)
        return;

    result = gpu.EndCommandBuffer(video->scene_command_buffer);
    if (result != VK_SUCCESS) {
        GpuVkHandleDeviceLost(video, "scene command buffer end", result);
        return;
    }

    BrMemSet(&submit_info, 0, sizeof(submit_info));
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &video->scene_command_buffer;

    result = gpu.QueueSubmit(video->queue, 1, &submit_info, video->scene_fence);
    if (result != VK_SUCCESS) {
        GpuVkHandleDeviceLost(video, "scene queue submit", result);
        return;
    }

    video->scene_fence_pending = BR_TRUE;

    result = gpu.WaitForFences(video->device, 1, &video->scene_fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        GpuVkHandleDeviceLost(video, "scene fence wait", result);
        return;
    }

    result = gpu.ResetFences(video->device, 1, &video->scene_fence);
    if (result != VK_SUCCESS) {
        GpuVkHandleDeviceLost(video, "scene fence reset", result);
        return;
    }

    video->scene_fence_pending = BR_FALSE;

    if (!gpu_frame_logged) {
        gpu_frame_logged = BR_TRUE;
        BrLogPrintf("VKREND: gpu frame compositor active\n");
        GpuVkLogMarker("%s", "VKREND: gpu frame compositor active");
    }
}

VkImageView GpuVkPresentView(br_vk_video* video, struct vk_gpu_target* target) {
    if (video == NULL || target == NULL || !gpu_loaded)
        return VK_NULL_HANDLE;
    if (target->video != video || target->generation != video->generation)
        return VK_NULL_HANDLE;

    return target->colour_view;
}

void GpuVkPresentPrepare(br_vk_video* video, struct vk_gpu_target* target, VkCommandBuffer command_buffer) {
    VkImageMemoryBarrier barrier;
    VkPipelineStageFlags source_stage;
    VkAccessFlags source_access;

    if (GpuVkPresentView(video, target) == VK_NULL_HANDLE)
        return;
    if (target->colour_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        return;

    if (target->colour_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        source_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (target->colour_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        source_access = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else {
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        source_access = 0;
    }

    BrMemSet(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = target->colour_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = source_access;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target->colour_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    gpu.CmdPipelineBarrier(command_buffer, source_stage, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL,
        1, &barrier);

    target->colour_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void GpuVkShadowResetKey(struct br_device_pixelmap* pixelmap) {
    br_int_32 row_bytes;
    br_size_t base;
    br_uint_32 x;
    br_uint_32 y;

    if (pixelmap == NULL || pixelmap->shadow == NULL || pixelmap->pm_width <= 0 || pixelmap->pm_height <= 0)
        return;

    row_bytes = pixelmap->pm_row_bytes;
    if (row_bytes < 0)
        row_bytes = -row_bytes;

    base = (br_size_t)pixelmap->pm_base_y * (br_size_t)row_bytes + (br_size_t)pixelmap->pm_base_x * 2;

    for (y = 0; y < (br_uint_32)pixelmap->pm_height; y++) {
        br_uint_16* row =
            (br_uint_16*)((br_uint_8*)pixelmap->shadow + base + (br_size_t)y * (br_size_t)row_bytes);

        for (x = 0; x < (br_uint_32)pixelmap->pm_width; x++)
            row[x] = 0xF81F;
    }
}

void GpuVkSceneBeginRenderer(br_renderer* renderer) {
    br_device_pixelmap* colour;
    br_vk_video* video;
    vk_gpu_target* target;

    if (gpu_disabled)
        return;

    colour = renderer->state.current->output.colour;
    if (colour == NULL || colour->device != renderer->device || colour->shadow == NULL)
        return;
    if (colour->use_type != BRT_OFFSCREEN || colour->pm_width <= 0 || colour->pm_height <= 0)
        return;

    video = colour->screen != NULL ? &colour->screen->video : NULL;
    if (video == NULL || !video->active || video->device == VK_NULL_HANDLE)
        return;

    if (!GpuVkLoadFunctions(video->instance)) {
        GpuVkDisable("function loading");
        return;
    }

    if (!video->scene_recording) {
        if (!VIDEOVkOverlayEnsure(video, (br_uint_32)colour->pm_width, (br_uint_32)colour->pm_height)) {
            GpuVkDisable("overlay creation");
            return;
        }
    }

    target = colour->gpu_target;
    if (!GpuVkTargetUsable(target, video, (br_uint_32)colour->pm_width, (br_uint_32)colour->pm_height)) {
        GpuVkTargetDestroy(&colour->gpu_target);

        target = BrMemAllocate(sizeof(*target), BR_MEMORY_APPLICATION);
        if (target != NULL) {
            BrMemSet(target, 0, sizeof(*target));
            target->video = video;
            target->generation = video->generation;
            target->pixelmap = colour;
            target->width = (br_uint_32)colour->pm_width;
            target->height = (br_uint_32)colour->pm_height;

            if (!GpuVkTargetInit(target))
                GpuVkTargetDestroy(&target);
            else
                colour->gpu_target = target;
        }

        if (colour->gpu_target == NULL) {
            GpuVkDisable("render target creation");
            return;
        }

        if (!gpu_targets_logged) {
            gpu_targets_logged = BR_TRUE;
            BrLogPrintf("VKREND: gpu targets active\n");
            GpuVkLogMarker("%s", "VKREND: gpu targets active");
        }

        target = colour->gpu_target;
    }

    {
        static int scene_log_count;

        if (scene_log_count < 48) {
            char text[192];

            scene_log_count++;
            BrSprintfN(text, sizeof(text) - 1, "scene %d: pm=%s %dx%d use=%d draws=%u", scene_log_count,
                colour->pm_identifier != NULL ? colour->pm_identifier : "?", (int)colour->pm_width,
                (int)colour->pm_height, (int)colour->use_type, (unsigned)GeometryStoredVkDrawCount());
            GpuVkLogMarker("%s", text);
        }
    }

    {
        br_uint_32 generation = video->generation;

        if (!GpuVkBeginScene(target)) {
            GpuVkTargetDestroy(&colour->gpu_target);
            if (video->generation == generation)
                GpuVkDisable("scene begin");
            return;
        }
    }

    renderer->gpu_scene = target;
}

void GpuVkSceneEndRenderer(br_renderer* renderer) {
    vk_gpu_target* target = renderer->gpu_scene;
    br_device_pixelmap* colour;

    renderer->gpu_scene = NULL;

    if (target == NULL)
        return;

    target->in_scene = BR_FALSE;
    colour = target->pixelmap;

    if (!GpuVkEndScene(target)) {
        if (colour != NULL && colour->gpu_target == target)
            GpuVkTargetDestroy(&colour->gpu_target);
        GpuVkDisable("scene end");
        return;
    }

    if (target->video != NULL) {
        br_vk_video* video = target->video;

        if (video->present_target == NULL
            || (video->present_target->frame_draw_count < VKREND_MAIN_TARGET_MIN_DRAWS
                && target->frame_draw_count > video->present_target->frame_draw_count)) {
            video->present_target = target;
        }

        video->frame_has_scene = BR_TRUE;
    }
}

void GpuVkGeometryRelease(vk_gpu_geometry* geometry) {
    if (geometry == NULL)
        return;

    if (geometry->video != NULL && geometry->video->active && geometry->video->generation == geometry->generation) {
        GpuVkDeferBuffer(geometry->video->device, geometry->vertex_buffer, geometry->vertex_memory);
        GpuVkDeferBuffer(geometry->video->device, geometry->index_buffer, geometry->index_memory);
    }

    geometry->vertex_buffer = VK_NULL_HANDLE;
    geometry->vertex_memory = VK_NULL_HANDLE;
    geometry->index_buffer = VK_NULL_HANDLE;
    geometry->index_memory = VK_NULL_HANDLE;

    geometry->video = NULL;
    geometry->generation = 0;
    geometry->ready = BR_FALSE;
}

static br_boolean GpuVkGeometryUpload(vk_gpu_geometry* geometry, const struct v11model* model) {
    br_uint_32 total_vertices = 0;
    br_uint_32 total_faces = 0;
    br_uint_32 index_offset = 0;
    br_uint_16 vertex_offset = 0;
    br_uint_16 group;
    VkDeviceSize vertex_bytes;
    VkDeviceSize index_bytes;
    float* vertices;
    br_uint_16* indices;
    void* mapped;

    if (model == NULL || model->groups == NULL)
        return BR_FALSE;

    for (group = 0; group < model->ngroups; group++) {
        total_vertices += model->groups[group].nvertices;
        total_faces += model->groups[group].nfaces;
    }

    if (total_vertices == 0 || total_faces == 0 || total_vertices > 65535)
        return BR_FALSE;

    vertex_bytes = (VkDeviceSize)total_vertices * VKREND_GPU_VERTEX_STRIDE;
    index_bytes = (VkDeviceSize)total_faces * 3 * sizeof(br_uint_16);

    vertices = BrMemAllocate((br_size_t)(vertex_bytes + index_bytes), BR_MEMORY_APPLICATION);
    if (vertices == NULL)
        return BR_FALSE;
    indices = (br_uint_16*)((br_uint_8*)vertices + vertex_bytes);

    {
        float* next = vertices;

        for (group = 0; group < model->ngroups; group++) {
            const struct v11group* gp = &model->groups[group];

            for (br_uint_16 v = 0; v < gp->nvertices; v++) {
                if (gp->position != NULL) {
                    next[0] = (float)gp->position[v].v[0];
                    next[1] = (float)gp->position[v].v[1];
                    next[2] = (float)gp->position[v].v[2];
                } else {
                    next[0] = 0.0f;
                    next[1] = 0.0f;
                    next[2] = 0.0f;
                }

                if (gp->map != NULL) {
                    next[3] = (float)gp->map[v].v[0];
                    next[4] = (float)gp->map[v].v[1];
                } else {
                    next[3] = 0.0f;
                    next[4] = 0.0f;
                }

                if (gp->vertex_colours != NULL) {
                    br_colour colour = gp->vertex_colours[v];

                    next[5] = (float)BR_RED(colour) / 255.0f;
                    next[6] = (float)BR_GRN(colour) / 255.0f;
                    next[7] = (float)BR_BLU(colour) / 255.0f;
                    next[8] = (float)BR_ALPHA(colour) / 255.0f;
                } else {
                    next[5] = 1.0f;
                    next[6] = 1.0f;
                    next[7] = 1.0f;
                    next[8] = 1.0f;
                }

                next += VKREND_GPU_VERTEX_FLOATS;
            }
        }
    }

    {
        br_uint_16* next = indices;
        br_uint_32 skipped_faces = 0;

        for (group = 0; group < model->ngroups; group++) {
            const struct v11group* gp = &model->groups[group];
            br_uint_32 index_count = 0;

            geometry->ranges[group].first_index = index_offset;

            for (br_uint_16 f = 0; f < gp->nfaces; f++) {
                const br_vector3_u16* fp = &gp->vertex_numbers[f];

                if (fp->v[0] >= gp->nvertices || fp->v[1] >= gp->nvertices || fp->v[2] >= gp->nvertices) {
                    skipped_faces++;
                    continue;
                }

                next[0] = (br_uint_16)(fp->v[0] + vertex_offset);
                next[1] = (br_uint_16)(fp->v[1] + vertex_offset);
                next[2] = (br_uint_16)(fp->v[2] + vertex_offset);
                next += 3;
                index_count += 3;
            }

            geometry->ranges[group].index_count = index_count;
            index_offset += index_count;
            vertex_offset = (br_uint_16)(vertex_offset + gp->nvertices);
        }

        if (skipped_faces > 0)
            BrLogPrintf("VKREND: skipped %u faces with out-of-range vertex indices\n", (unsigned)skipped_faces);
    }

    if (!GpuVkCreateBuffer(geometry->video, vertex_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &geometry->vertex_buffer,
            &geometry->vertex_memory, &mapped)) {
        BrMemFree(vertices);
        return BR_FALSE;
    }
    BrMemCpy(mapped, vertices, vertex_bytes);
    gpu.UnmapMemory(geometry->video->device, geometry->vertex_memory);

    if (!GpuVkCreateBuffer(geometry->video, index_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &geometry->index_buffer,
            &geometry->index_memory, &mapped)) {
        BrMemFree(vertices);
        return BR_FALSE;
    }
    BrMemCpy(mapped, indices, index_bytes);
    gpu.UnmapMemory(geometry->video->device, geometry->index_memory);

    BrMemFree(vertices);

    geometry->ngroups = model->ngroups;
    geometry->total_vertices = total_vertices;
    geometry->total_faces = total_faces;
    return BR_TRUE;
}

static vk_gpu_geometry* GpuVkGeometryEnsure(br_geometry_stored* stored, br_vk_video* video) {
    vk_gpu_geometry* geometry = stored->gpu;

    if (geometry != NULL && geometry->video == video && geometry->generation == video->generation)
        return geometry->ready ? geometry : NULL;

    if (geometry == NULL) {
        geometry = BrResAllocate(stored, sizeof(*geometry), BR_MEMORY_OBJECT_DATA);
        if (geometry == NULL)
            return NULL;

        BrMemSet(geometry, 0, sizeof(*geometry));
        stored->gpu = geometry;
    } else {
        GpuVkGeometryRelease(geometry);
    }

    if (geometry->ranges == NULL) {
        geometry->ranges =
            BrResAllocate(geometry, sizeof(*geometry->ranges) * stored->model->ngroups, BR_MEMORY_OBJECT_DATA);
        if (geometry->ranges == NULL)
            return NULL;
    }

    geometry->video = video;
    geometry->generation = video->generation;

    if (!GpuVkGeometryUpload(geometry, stored->model))
        return NULL;

    geometry->ready = BR_TRUE;
    return geometry;
}

static void GpuVkDrawGeometry(vk_gpu_target* target, br_renderer* renderer, vk_gpu_geometry* geometry,
    const struct v11model* model, struct br_renderer_state_stored* default_state) {
    VkCommandBuffer command_buffer = target->video->scene_command_buffer;
    VkDeviceSize vertex_buffer_offset = 0;
    br_uint_16 group;

    gpu.CmdBindVertexBuffers(command_buffer, 0, 1, &geometry->vertex_buffer, &vertex_buffer_offset);
    gpu.CmdBindIndexBuffer(command_buffer, geometry->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    for (group = 0; group < geometry->ngroups; group++) {
        vk_gpu_material material;
        VkDescriptorSet descriptor_set;
        br_uint_32 ubo_offset;
        br_uint_32 slot;

        if (geometry->ranges[group].index_count == 0)
            continue;
        if (target->ubo_slot >= VKREND_GPU_UBO_SLOTS)
            break;

        slot = target->ubo_slot++;

        BrMemSet(&material, 0, sizeof(material));
        GpuVkMaterialResolve(model, group, default_state, &material);
        descriptor_set = GpuVkMaterialDescriptor(target, &material);

        GpuVkWriteUbo(target, slot, renderer, &material);
        ubo_offset = (br_uint_32)((VkDeviceSize)slot * target->ubo_stride);


        gpu.CmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, target->pipeline_layout, 0, 1,
            &descriptor_set, 1, &ubo_offset);

        target->frame_draw_count++;

        gpu.CmdDrawIndexed(command_buffer, geometry->ranges[group].index_count, 1,
            geometry->ranges[group].first_index, 0, 0);
    }
}

void GpuVkRenderStored(br_geometry_stored* stored, br_renderer* renderer,
    struct br_renderer_state_stored* default_state) {
    vk_gpu_target* target = renderer->gpu_scene;
    vk_gpu_geometry* geometry;
    br_vk_video* video;

    if (gpu_disabled || target == NULL || !target->in_scene)
        return;
    if (stored->model == NULL)
        return;
    if (stored->total_faces == 0 || stored->total_vertices == 0 || stored->total_vertices > 65535)
        return;

    video = target->video;
    if (video == NULL || !video->active || video->generation != target->generation)
        return;

    geometry = GpuVkGeometryEnsure(stored, video);
    if (geometry == NULL) {
        GpuVkDisable("geometry upload");
        return;
    }

    GpuVkDrawGeometry(target, renderer, geometry, stored->model, default_state);

    if (!gpu_draw_logged) {
        gpu_draw_logged = BR_TRUE;
        BrLogPrintf("VKREND: gpu geometry draw\n");
        GpuVkLogMarker("%s", "VKREND: gpu geometry draw");
    }
}

br_boolean GpuVkRenderV1Model(struct v11model* model, struct br_renderer* renderer,
    struct br_renderer_state_stored* default_state) {
    vk_gpu_target* target;
    br_vk_video* video;
    vk_gpu_geometry geometry;

    if (gpu_disabled || renderer == NULL || model == NULL)
        return BR_FALSE;
    if (model->groups == NULL || model->ngroups == 0)
        return BR_FALSE;

    target = renderer->gpu_scene;
    if (target == NULL || !target->in_scene)
        return BR_FALSE;

    video = target->video;
    if (video == NULL || !video->active || video->generation != target->generation)
        return BR_FALSE;

    if (target->retired_count + 2 > VKREND_GPU_RETIRED_CAPACITY) {
        if (!gpu_retired_full_logged) {
            gpu_retired_full_logged = BR_TRUE;
            GpuVkLogMarker("%s", "VKREND: temporary geometry budget exhausted, immediate model skipped");
        }
        return BR_FALSE;
    }

    BrMemSet(&geometry, 0, sizeof(geometry));
    geometry.video = video;
    geometry.generation = target->generation;
    geometry.ranges = BrMemAllocate(sizeof(*geometry.ranges) * (br_size_t)model->ngroups, BR_MEMORY_APPLICATION);
    if (geometry.ranges == NULL)
        return BR_FALSE;

    if (!GpuVkGeometryUpload(&geometry, model)) {
        BrMemFree(geometry.ranges);
        GpuVkRetireGeometry(target, &geometry);
        return BR_FALSE;
    }

    GpuVkDrawGeometry(target, renderer, &geometry, model, default_state);

    GpuVkRetireGeometry(target, &geometry);
    BrMemFree(geometry.ranges);

    return BR_TRUE;
}
