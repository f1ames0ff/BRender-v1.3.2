#include "drv.h"

#include <string.h>

#include "vkshaders.h"

static PFN_vkGetInstanceProcAddr gpa;

static PFN_vkCreateInstance pCreateInstance;
static PFN_vkEnumerateInstanceExtensionProperties pEnumerateInstanceExtensionProperties;
static PFN_vkDestroyInstance pDestroyInstance;
static PFN_vkEnumeratePhysicalDevices pEnumeratePhysicalDevices;
static PFN_vkEnumerateDeviceExtensionProperties pEnumerateDeviceExtensionProperties;
static PFN_vkGetPhysicalDeviceFormatProperties pGetPhysicalDeviceFormatProperties;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties pGetPhysicalDeviceQueueFamilyProperties;
static PFN_vkGetPhysicalDeviceMemoryProperties pGetPhysicalDeviceMemoryProperties;
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR pGetPhysicalDeviceSurfaceSupportKHR;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pGetPhysicalDeviceSurfaceCapabilitiesKHR;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pGetPhysicalDeviceSurfaceFormatsKHR;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pGetPhysicalDeviceSurfacePresentModesKHR;
static PFN_vkCreateDevice pCreateDevice;
static PFN_vkDestroyDevice pDestroyDevice;
static PFN_vkGetDeviceQueue pGetDeviceQueue;
static PFN_vkDeviceWaitIdle pDeviceWaitIdle;
static PFN_vkDestroySurfaceKHR pDestroySurfaceKHR;
static PFN_vkCreateSwapchainKHR pCreateSwapchainKHR;
static PFN_vkDestroySwapchainKHR pDestroySwapchainKHR;
static PFN_vkGetSwapchainImagesKHR pGetSwapchainImagesKHR;
static PFN_vkAcquireNextImageKHR pAcquireNextImageKHR;
static PFN_vkQueuePresentKHR pQueuePresentKHR;
static PFN_vkQueueSubmit pQueueSubmit;
static PFN_vkCreateCommandPool pCreateCommandPool;
static PFN_vkDestroyCommandPool pDestroyCommandPool;
static PFN_vkAllocateCommandBuffers pAllocateCommandBuffers;
static PFN_vkBeginCommandBuffer pBeginCommandBuffer;
static PFN_vkEndCommandBuffer pEndCommandBuffer;
static PFN_vkResetCommandBuffer pResetCommandBuffer;
static PFN_vkCmdPipelineBarrier pCmdPipelineBarrier;
static PFN_vkCmdCopyBufferToImage pCmdCopyBufferToImage;
static PFN_vkCmdBeginRenderPass pCmdBeginRenderPass;
static PFN_vkCmdEndRenderPass pCmdEndRenderPass;
static PFN_vkCmdBindPipeline pCmdBindPipeline;
static PFN_vkCmdBindDescriptorSets pCmdBindDescriptorSets;
static PFN_vkCmdSetViewport pCmdSetViewport;
static PFN_vkCmdSetScissor pCmdSetScissor;
static PFN_vkCmdDraw pCmdDraw;
static PFN_vkCreateImageView pCreateImageView;
static PFN_vkDestroyImageView pDestroyImageView;
static PFN_vkCreateImage pCreateImage;
static PFN_vkDestroyImage pDestroyImage;
static PFN_vkGetImageMemoryRequirements pGetImageMemoryRequirements;
static PFN_vkCreateBuffer pCreateBuffer;
static PFN_vkDestroyBuffer pDestroyBuffer;
static PFN_vkGetBufferMemoryRequirements pGetBufferMemoryRequirements;
static PFN_vkAllocateMemory pAllocateMemory;
static PFN_vkFreeMemory pFreeMemory;
static PFN_vkBindImageMemory pBindImageMemory;
static PFN_vkBindBufferMemory pBindBufferMemory;
static PFN_vkMapMemory pMapMemory;
static PFN_vkUnmapMemory pUnmapMemory;
static PFN_vkCreateRenderPass pCreateRenderPass;
static PFN_vkDestroyRenderPass pDestroyRenderPass;
static PFN_vkCreateFramebuffer pCreateFramebuffer;
static PFN_vkDestroyFramebuffer pDestroyFramebuffer;
static PFN_vkCreateShaderModule pCreateShaderModule;
static PFN_vkDestroyShaderModule pDestroyShaderModule;
static PFN_vkCreateDescriptorSetLayout pCreateDescriptorSetLayout;
static PFN_vkDestroyDescriptorSetLayout pDestroyDescriptorSetLayout;
static PFN_vkCreateDescriptorPool pCreateDescriptorPool;
static PFN_vkDestroyDescriptorPool pDestroyDescriptorPool;
static PFN_vkAllocateDescriptorSets pAllocateDescriptorSets;
static PFN_vkUpdateDescriptorSets pUpdateDescriptorSets;
static PFN_vkCreatePipelineLayout pCreatePipelineLayout;
static PFN_vkDestroyPipelineLayout pDestroyPipelineLayout;
static PFN_vkCreateGraphicsPipelines pCreateGraphicsPipelines;
static PFN_vkDestroyPipeline pDestroyPipeline;
static PFN_vkCreateSampler pCreateSampler;
static PFN_vkDestroySampler pDestroySampler;
static PFN_vkCreateFence pCreateFence;
static PFN_vkDestroyFence pDestroyFence;
static PFN_vkWaitForFences pWaitForFences;
static PFN_vkResetFences pResetFences;
static PFN_vkCreateSemaphore pCreateSemaphore;
static PFN_vkDestroySemaphore pDestroySemaphore;

static br_error video_fail(const char* stage, VkResult result) {
    BrFailure("VKREND: %s failed (VkResult %d)", (char*)stage, (int)result);
    return BRE_FAIL;
}

static br_boolean video_host_failed(br_boolean result) {
    return (result != 0) ? BR_TRUE : BR_FALSE;
}

static br_boolean video_copy_callbacks(br_vk_video* video, const br_device_vkrend_callback_procs* callbacks) {
    br_size_t size;

    if (callbacks == NULL)
        return BR_FALSE;

    size = callbacks->size;
    if (size == 0 || size > sizeof(video->callbacks))
        size = sizeof(video->callbacks);

    BrMemCpy(&video->callbacks, callbacks, size);
    return BR_TRUE;
}

static br_error video_load_instance_functions(br_vk_video* video) {
#define VKREND_LOAD(type, name)                                         \
    do {                                                               \
        p##name = (type)gpa(video->instance, "vk" #name);              \
        if (p##name == NULL) {                                         \
            BrFailure("VKREND: missing vk%s", (char*)#name);           \
            return BRE_FAIL;                                           \
        }                                                              \
    } while (0)

    VKREND_LOAD(PFN_vkDestroyInstance, DestroyInstance);
    VKREND_LOAD(PFN_vkEnumeratePhysicalDevices, EnumeratePhysicalDevices);
    VKREND_LOAD(PFN_vkEnumerateDeviceExtensionProperties, EnumerateDeviceExtensionProperties);
    VKREND_LOAD(PFN_vkGetPhysicalDeviceFormatProperties, GetPhysicalDeviceFormatProperties);
    VKREND_LOAD(PFN_vkGetPhysicalDeviceQueueFamilyProperties, GetPhysicalDeviceQueueFamilyProperties);
    VKREND_LOAD(PFN_vkGetPhysicalDeviceMemoryProperties, GetPhysicalDeviceMemoryProperties);
    VKREND_LOAD(PFN_vkGetPhysicalDeviceSurfaceSupportKHR, GetPhysicalDeviceSurfaceSupportKHR);
    VKREND_LOAD(PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    VKREND_LOAD(PFN_vkGetPhysicalDeviceSurfaceFormatsKHR, GetPhysicalDeviceSurfaceFormatsKHR);
    VKREND_LOAD(PFN_vkGetPhysicalDeviceSurfacePresentModesKHR, GetPhysicalDeviceSurfacePresentModesKHR);
    VKREND_LOAD(PFN_vkCreateDevice, CreateDevice);
    VKREND_LOAD(PFN_vkDestroyDevice, DestroyDevice);
    VKREND_LOAD(PFN_vkGetDeviceQueue, GetDeviceQueue);
    VKREND_LOAD(PFN_vkDeviceWaitIdle, DeviceWaitIdle);
    VKREND_LOAD(PFN_vkDestroySurfaceKHR, DestroySurfaceKHR);
    VKREND_LOAD(PFN_vkCreateSwapchainKHR, CreateSwapchainKHR);
    VKREND_LOAD(PFN_vkDestroySwapchainKHR, DestroySwapchainKHR);
    VKREND_LOAD(PFN_vkGetSwapchainImagesKHR, GetSwapchainImagesKHR);
    VKREND_LOAD(PFN_vkAcquireNextImageKHR, AcquireNextImageKHR);
    VKREND_LOAD(PFN_vkQueuePresentKHR, QueuePresentKHR);
    VKREND_LOAD(PFN_vkQueueSubmit, QueueSubmit);
    VKREND_LOAD(PFN_vkCreateCommandPool, CreateCommandPool);
    VKREND_LOAD(PFN_vkDestroyCommandPool, DestroyCommandPool);
    VKREND_LOAD(PFN_vkAllocateCommandBuffers, AllocateCommandBuffers);
    VKREND_LOAD(PFN_vkBeginCommandBuffer, BeginCommandBuffer);
    VKREND_LOAD(PFN_vkEndCommandBuffer, EndCommandBuffer);
    VKREND_LOAD(PFN_vkResetCommandBuffer, ResetCommandBuffer);
    VKREND_LOAD(PFN_vkCmdPipelineBarrier, CmdPipelineBarrier);
    VKREND_LOAD(PFN_vkCmdCopyBufferToImage, CmdCopyBufferToImage);
    VKREND_LOAD(PFN_vkCmdBeginRenderPass, CmdBeginRenderPass);
    VKREND_LOAD(PFN_vkCmdEndRenderPass, CmdEndRenderPass);
    VKREND_LOAD(PFN_vkCmdBindPipeline, CmdBindPipeline);
    VKREND_LOAD(PFN_vkCmdBindDescriptorSets, CmdBindDescriptorSets);
    VKREND_LOAD(PFN_vkCmdSetViewport, CmdSetViewport);
    VKREND_LOAD(PFN_vkCmdSetScissor, CmdSetScissor);
    VKREND_LOAD(PFN_vkCmdDraw, CmdDraw);
    VKREND_LOAD(PFN_vkCreateImageView, CreateImageView);
    VKREND_LOAD(PFN_vkDestroyImageView, DestroyImageView);
    VKREND_LOAD(PFN_vkCreateImage, CreateImage);
    VKREND_LOAD(PFN_vkDestroyImage, DestroyImage);
    VKREND_LOAD(PFN_vkGetImageMemoryRequirements, GetImageMemoryRequirements);
    VKREND_LOAD(PFN_vkCreateBuffer, CreateBuffer);
    VKREND_LOAD(PFN_vkDestroyBuffer, DestroyBuffer);
    VKREND_LOAD(PFN_vkGetBufferMemoryRequirements, GetBufferMemoryRequirements);
    VKREND_LOAD(PFN_vkAllocateMemory, AllocateMemory);
    VKREND_LOAD(PFN_vkFreeMemory, FreeMemory);
    VKREND_LOAD(PFN_vkBindImageMemory, BindImageMemory);
    VKREND_LOAD(PFN_vkBindBufferMemory, BindBufferMemory);
    VKREND_LOAD(PFN_vkMapMemory, MapMemory);
    VKREND_LOAD(PFN_vkUnmapMemory, UnmapMemory);
    VKREND_LOAD(PFN_vkCreateRenderPass, CreateRenderPass);
    VKREND_LOAD(PFN_vkDestroyRenderPass, DestroyRenderPass);
    VKREND_LOAD(PFN_vkCreateFramebuffer, CreateFramebuffer);
    VKREND_LOAD(PFN_vkDestroyFramebuffer, DestroyFramebuffer);
    VKREND_LOAD(PFN_vkCreateShaderModule, CreateShaderModule);
    VKREND_LOAD(PFN_vkDestroyShaderModule, DestroyShaderModule);
    VKREND_LOAD(PFN_vkCreateDescriptorSetLayout, CreateDescriptorSetLayout);
    VKREND_LOAD(PFN_vkDestroyDescriptorSetLayout, DestroyDescriptorSetLayout);
    VKREND_LOAD(PFN_vkCreateDescriptorPool, CreateDescriptorPool);
    VKREND_LOAD(PFN_vkDestroyDescriptorPool, DestroyDescriptorPool);
    VKREND_LOAD(PFN_vkAllocateDescriptorSets, AllocateDescriptorSets);
    VKREND_LOAD(PFN_vkUpdateDescriptorSets, UpdateDescriptorSets);
    VKREND_LOAD(PFN_vkCreatePipelineLayout, CreatePipelineLayout);
    VKREND_LOAD(PFN_vkDestroyPipelineLayout, DestroyPipelineLayout);
    VKREND_LOAD(PFN_vkCreateGraphicsPipelines, CreateGraphicsPipelines);
    VKREND_LOAD(PFN_vkDestroyPipeline, DestroyPipeline);
    VKREND_LOAD(PFN_vkCreateSampler, CreateSampler);
    VKREND_LOAD(PFN_vkDestroySampler, DestroySampler);
    VKREND_LOAD(PFN_vkCreateFence, CreateFence);
    VKREND_LOAD(PFN_vkDestroyFence, DestroyFence);
    VKREND_LOAD(PFN_vkWaitForFences, WaitForFences);
    VKREND_LOAD(PFN_vkResetFences, ResetFences);
    VKREND_LOAD(PFN_vkCreateSemaphore, CreateSemaphore);
    VKREND_LOAD(PFN_vkDestroySemaphore, DestroySemaphore);
#undef VKREND_LOAD
    return BRE_OK;
}

static br_error video_find_memory_type(br_vk_video* video, br_uint_32 type_bits, VkMemoryPropertyFlags properties,
    br_uint_32* index) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    br_uint_32 i;

    pGetPhysicalDeviceMemoryProperties(video->physical_device, &memory_properties);

    for (i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_bits & (1u << i)) != 0 && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            *index = i;
            return BRE_OK;
        }
    }

    BrFailure("VKREND: no matching memory type");
    return BRE_FAIL;
}

static br_error video_create_buffer(br_vk_video* video, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer,
    VkDeviceMemory* memory, void** mapped) {
    VkBufferCreateInfo buffer_info;
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocate_info;
    VkResult result;
    br_uint_32 memory_type;

    BrMemSet(&buffer_info, 0, sizeof(buffer_info));
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = pCreateBuffer(video->device, &buffer_info, NULL, buffer);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateBuffer", result);

    pGetBufferMemoryRequirements(video->device, *buffer, &requirements);
    if (video_find_memory_type(video, requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memory_type) != BRE_OK)
        return BRE_FAIL;

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = memory_type;

    result = pAllocateMemory(video->device, &allocate_info, NULL, memory);
    if (result != VK_SUCCESS)
        return video_fail("vkAllocateMemory(buffer)", result);
    if (pBindBufferMemory(video->device, *buffer, *memory, 0) != VK_SUCCESS) {
        BrFailure("VKREND: vkBindBufferMemory failed");
        return BRE_FAIL;
    }
    if (mapped != NULL) {
        result = pMapMemory(video->device, *memory, 0, VK_WHOLE_SIZE, 0, mapped);
        if (result != VK_SUCCESS)
            return video_fail("vkMapMemory", result);
    }

    return BRE_OK;
}

static br_error video_create_image(br_vk_video* video, br_uint_32 width, br_uint_32 height, VkFormat format,
    VkImageUsageFlags usage, VkImage* image, VkDeviceMemory* memory, VkImageView* view, br_boolean* initialised) {
    VkImageCreateInfo image_info;
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocate_info;
    VkImageViewCreateInfo view_info;
    VkResult result;
    br_uint_32 memory_type;

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

    result = pCreateImage(video->device, &image_info, NULL, image);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateImage", result);

    pGetImageMemoryRequirements(video->device, *image, &requirements);
    if (video_find_memory_type(video, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_type) != BRE_OK)
        return BRE_FAIL;

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = memory_type;

    result = pAllocateMemory(video->device, &allocate_info, NULL, memory);
    if (result != VK_SUCCESS)
        return video_fail("vkAllocateMemory(image)", result);
    if (pBindImageMemory(video->device, *image, *memory, 0) != VK_SUCCESS) {
        BrFailure("VKREND: vkBindImageMemory failed");
        return BRE_FAIL;
    }

    BrMemSet(&view_info, 0, sizeof(view_info));
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = *image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    result = pCreateImageView(video->device, &view_info, NULL, view);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateImageView", result);
    if (initialised != NULL)
        *initialised = BR_FALSE;

    return BRE_OK;
}

static void video_destroy_buffer(br_vk_video* video, VkBuffer* buffer, VkDeviceMemory* memory) {
    if (*buffer != VK_NULL_HANDLE) {
        pDestroyBuffer(video->device, *buffer, NULL);
        *buffer = VK_NULL_HANDLE;
    }
    if (*memory != VK_NULL_HANDLE) {
        pFreeMemory(video->device, *memory, NULL);
        *memory = VK_NULL_HANDLE;
    }
}

static void video_destroy_image(br_vk_video* video, VkImage* image, VkDeviceMemory* memory, VkImageView* view) {
    if (*view != VK_NULL_HANDLE) {
        pDestroyImageView(video->device, *view, NULL);
        *view = VK_NULL_HANDLE;
    }
    if (*image != VK_NULL_HANDLE) {
        pDestroyImage(video->device, *image, NULL);
        *image = VK_NULL_HANDLE;
    }
    if (*memory != VK_NULL_HANDLE) {
        pFreeMemory(video->device, *memory, NULL);
        *memory = VK_NULL_HANDLE;
    }
}

static br_error video_create_instance(br_vk_video* video) {
    const char* extensions[VKREND_MAX_INSTANCE_EXTENSIONS];
    br_uint_32 extension_count = 0;
    br_uint_32 i;
    VkApplicationInfo app_info;
    VkInstanceCreateInfo create_info;
    VkResult result;
    VkExtensionProperties* available = NULL;
    br_uint_32 available_count = 0;
    br_boolean portability = BR_FALSE;

    pCreateInstance = (PFN_vkCreateInstance)gpa(NULL, "vkCreateInstance");
    pEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)gpa(NULL, "vkEnumerateInstanceExtensionProperties");
    if (pCreateInstance == NULL || pEnumerateInstanceExtensionProperties == NULL) {
        BrFailure("VKREND: loader is missing core instance entry points");
        return BRE_FAIL;
    }

    if (video_host_failed(video->callbacks.get_instance_extensions(video->callbacks.window, &extension_count, extensions,
            VKREND_MAX_INSTANCE_EXTENSIONS))) {
        BrFailure("VKREND: host could not provide the instance extensions");
        return BRE_FAIL;
    }

    result = pEnumerateInstanceExtensionProperties(NULL, &available_count, NULL);
    if (result == VK_SUCCESS && available_count > 0) {
        available = BrMemAllocate(sizeof(VkExtensionProperties) * available_count, BR_MEMORY_APPLICATION);
        if (available != NULL
            && pEnumerateInstanceExtensionProperties(NULL, &available_count, available) == VK_SUCCESS) {
            for (i = 0; i < available_count; i++) {
                if (strcmp(available[i].extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
                    portability = BR_TRUE;
                    break;
                }
            }
        }
        BrMemFree(available);
    }

    if (portability && extension_count < VKREND_MAX_INSTANCE_EXTENSIONS)
        extensions[extension_count++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;

    BrMemSet(&app_info, 0, sizeof(app_info));
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vkrend";
    app_info.pEngineName = "BRender";
    app_info.apiVersion = VK_API_VERSION_1_0;

    BrMemSet(&create_info, 0, sizeof(create_info));
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extension_count > 0 ? extensions : NULL;
    if (portability)
        create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    result = pCreateInstance(&create_info, NULL, &video->instance);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateInstance", result);
    if (video_load_instance_functions(video) != BRE_OK)
        return BRE_FAIL;
    if (video_host_failed(video->callbacks.create_surface(video->callbacks.window, video->instance, (void**)&video->surface))) {
        BrFailure("VKREND: host could not create a Vulkan surface");
        return BRE_FAIL;
    }

    return BRE_OK;
}

static br_error video_pick_physical_device(br_vk_video* video) {
    br_uint_32 device_count = 0;
    VkPhysicalDevice* devices;
    br_uint_32 i;
    VkResult result;

    result = pEnumeratePhysicalDevices(video->instance, &device_count, NULL);
    if (result != VK_SUCCESS || device_count == 0) {
        BrFailure("VKREND: no Vulkan physical devices");
        return BRE_FAIL;
    }

    devices = BrMemAllocate(sizeof(VkPhysicalDevice) * device_count, BR_MEMORY_APPLICATION);
    if (devices == NULL)
        return BRE_NO_MEMORY;

    result = pEnumeratePhysicalDevices(video->instance, &device_count, devices);
    if (result != VK_SUCCESS) {
        BrMemFree(devices);
        return video_fail("vkEnumeratePhysicalDevices", result);
    }

    for (i = 0; i < device_count; i++) {
        br_uint_32 queue_family_count = 0;
        VkQueueFamilyProperties* queue_families;
        br_uint_32 j;
        br_int_32 graphics_family = -1;
        br_boolean present = BR_FALSE;
        br_uint_32 extension_count = 0;
        VkExtensionProperties* extensions;
        br_boolean has_swapchain = BR_FALSE;
        VkBool32 supports_present = VK_FALSE;

        pGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_family_count, NULL);
        queue_families = BrMemAllocate(sizeof(VkQueueFamilyProperties) * queue_family_count, BR_MEMORY_APPLICATION);
        if (queue_families == NULL)
            continue;

        pGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_family_count, queue_families);
        for (j = 0; j < queue_family_count; j++) {
            if ((queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                graphics_family = (br_int_32)j;
                pGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, video->surface, &supports_present);
                present = supports_present != VK_FALSE;
                break;
            }
        }
        BrMemFree(queue_families);

        if (graphics_family < 0 || !present)
            continue;

        if (pEnumerateDeviceExtensionProperties(devices[i], VK_NULL_HANDLE, &extension_count, NULL) != VK_SUCCESS
            || extension_count == 0)
            continue;

        extensions = BrMemAllocate(sizeof(VkExtensionProperties) * extension_count, BR_MEMORY_APPLICATION);
        if (extensions == NULL)
            continue;

        if (pEnumerateDeviceExtensionProperties(devices[i], VK_NULL_HANDLE, &extension_count, extensions) == VK_SUCCESS) {
            for (j = 0; j < extension_count; j++) {
                if (strcmp(extensions[j].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    has_swapchain = BR_TRUE;
                    break;
                }
            }
        }
        BrMemFree(extensions);

        if (!has_swapchain)
            continue;

        video->physical_device = devices[i];
        video->queue_family = (br_uint_32)graphics_family;
        break;
    }

    BrMemFree(devices);

    if (video->physical_device == VK_NULL_HANDLE) {
        BrFailure("VKREND: no device with graphics/present queues and swapchain support");
        return BRE_FAIL;
    }

    return BRE_OK;
}

static br_error video_check_frame_format(br_vk_video* video) {
    VkFormatProperties properties;
    VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;

    pGetPhysicalDeviceFormatProperties(video->physical_device, VK_FORMAT_R5G6B5_UNORM_PACK16, &properties);
    if ((properties.optimalTilingFeatures & required) != required) {
        BrFailure("VKREND: R5G6B5_UNORM_PACK16 is not sampleable or transferable on this device");
        return BRE_FAIL;
    }

    return BRE_OK;
}

static br_error video_create_device(br_vk_video* video) {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info;
    VkDeviceCreateInfo device_info;
    const char* extensions[1];
    VkResult result;

    BrMemSet(&queue_info, 0, sizeof(queue_info));
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = video->queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    BrMemSet(&device_info, 0, sizeof(device_info));
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = extensions;

    result = pCreateDevice(video->physical_device, &device_info, NULL, &video->device);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateDevice", result);

    pGetDeviceQueue(video->device, video->queue_family, 0, &video->queue);

    return BRE_OK;
}

static br_error video_choose_surface_format(br_vk_video* video) {
    br_uint_32 count = 0;
    VkSurfaceFormatKHR* formats;
    br_uint_32 i;
    VkResult result;
    br_boolean found = BR_FALSE;

    result = pGetPhysicalDeviceSurfaceFormatsKHR(video->physical_device, video->surface, &count, NULL);
    if (result != VK_SUCCESS || count == 0) {
        BrFailure("VKREND: the surface has no formats");
        return BRE_FAIL;
    }

    formats = BrMemAllocate(sizeof(VkSurfaceFormatKHR) * count, BR_MEMORY_APPLICATION);
    if (formats == NULL)
        return BRE_NO_MEMORY;

    if (pGetPhysicalDeviceSurfaceFormatsKHR(video->physical_device, video->surface, &count, formats) != VK_SUCCESS) {
        BrMemFree(formats);
        BrFailure("VKREND: could not query the surface formats");
        return BRE_FAIL;
    }

    video->format = formats[0].format;
    video->color_space = formats[0].colorSpace;

    for (i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
            video->format = formats[i].format;
            video->color_space = formats[i].colorSpace;
            found = BR_TRUE;
            break;
        }
    }

    if (!found) {
        for (i = 0; i < count; i++) {
            if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
                video->format = formats[i].format;
                video->color_space = formats[i].colorSpace;
                break;
            }
        }
    }

    BrMemFree(formats);
    return BRE_OK;
}

static br_error video_get_extent(br_vk_video* video, VkExtent2D* extent) {
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result;

    result = pGetPhysicalDeviceSurfaceCapabilitiesKHR(video->physical_device, video->surface, &capabilities);
    if (result != VK_SUCCESS)
        return video_fail("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result);

    if (capabilities.currentExtent.width != 0xFFFFFFFFu) {
        *extent = capabilities.currentExtent;
        return BRE_OK;
    }

    extent->width = (br_uint_32)video->window_width;
    extent->height = (br_uint_32)video->window_height;
    if (extent->width < capabilities.minImageExtent.width)
        extent->width = capabilities.minImageExtent.width;
    if (extent->width > capabilities.maxImageExtent.width)
        extent->width = capabilities.maxImageExtent.width;
    if (extent->height < capabilities.minImageExtent.height)
        extent->height = capabilities.minImageExtent.height;
    if (extent->height > capabilities.maxImageExtent.height)
        extent->height = capabilities.maxImageExtent.height;

    return BRE_OK;
}

static void video_destroy_framebuffers_and_views(br_vk_video* video) {
    br_uint_32 i;

    if (video->framebuffers != NULL) {
        for (i = 0; i < video->image_count; i++) {
            if (video->framebuffers[i] != VK_NULL_HANDLE)
                pDestroyFramebuffer(video->device, video->framebuffers[i], NULL);
        }
        BrMemFree(video->framebuffers);
        video->framebuffers = NULL;
    }

    if (video->views != NULL) {
        for (i = 0; i < video->image_count; i++) {
            if (video->views[i] != VK_NULL_HANDLE)
                pDestroyImageView(video->device, video->views[i], NULL);
        }
        BrMemFree(video->views);
        video->views = NULL;
    }

    BrMemFree(video->images);
    video->images = NULL;
    video->image_count = 0;
}

static br_error video_create_swapchain(br_vk_video* video, VkSwapchainKHR old_swapchain) {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSwapchainCreateInfoKHR create_info;
    VkExtent2D extent;
    VkPresentModeKHR* present_modes;
    br_uint_32 present_mode_count = 0;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    br_uint_32 i;
    br_uint_32 image_count;
    VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkResult result;

    if (video_get_extent(video, &extent) != BRE_OK)
        return BRE_FAIL;
    if (extent.width == 0 || extent.height == 0) {
        BrLogPrintf("VKREND: zero-sized drawable\n");
        return BRE_OK;
    }

    if (pGetPhysicalDeviceSurfaceCapabilitiesKHR(video->physical_device, video->surface, &capabilities) != VK_SUCCESS) {
        BrFailure("VKREND: could not query the surface capabilities");
        return BRE_FAIL;
    }

    if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0)
        composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    present_modes = NULL;
    if (pGetPhysicalDeviceSurfacePresentModesKHR(video->physical_device, video->surface, &present_mode_count, NULL) == VK_SUCCESS
        && present_mode_count > 0) {
        present_modes = BrMemAllocate(sizeof(VkPresentModeKHR) * present_mode_count, BR_MEMORY_APPLICATION);
        if (present_modes != NULL) {
            if (pGetPhysicalDeviceSurfacePresentModesKHR(video->physical_device, video->surface, &present_mode_count,
                    present_modes) == VK_SUCCESS) {
                present_mode = present_modes[0];
                for (i = 0; i < present_mode_count; i++) {
                    if (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
                        present_mode = VK_PRESENT_MODE_FIFO_KHR;
                        break;
                    }
                }
            }
            BrMemFree(present_modes);
        }
    }

    image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    BrMemSet(&create_info, 0, sizeof(create_info));
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = video->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = video->format;
    create_info.imageColorSpace = video->color_space;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = composite;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = old_swapchain;

    result = pCreateSwapchainKHR(video->device, &create_info, NULL, &video->swapchain);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateSwapchainKHR", result);
    video->extent = extent;

    result = pGetSwapchainImagesKHR(video->device, video->swapchain, &image_count, NULL);
    if (result != VK_SUCCESS || image_count == 0)
        return video_fail("vkGetSwapchainImagesKHR", result);

    video->image_count = image_count;
    video->images = BrMemAllocate(sizeof(VkImage) * image_count, BR_MEMORY_APPLICATION);
    video->views = BrMemCalloc(image_count, sizeof(VkImageView), BR_MEMORY_APPLICATION);
    video->framebuffers = BrMemCalloc(image_count, sizeof(VkFramebuffer), BR_MEMORY_APPLICATION);
    if (video->images == NULL || video->views == NULL || video->framebuffers == NULL)
        return BRE_NO_MEMORY;

    result = pGetSwapchainImagesKHR(video->device, video->swapchain, &image_count, video->images);
    if (result != VK_SUCCESS || image_count > video->image_count)
        return video_fail("vkGetSwapchainImagesKHR", result);

    for (i = 0; i < video->image_count; i++) {
        VkImageViewCreateInfo view_info;

        BrMemSet(&view_info, 0, sizeof(view_info));
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = video->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = video->format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;

        result = pCreateImageView(video->device, &view_info, NULL, &video->views[i]);
        if (result != VK_SUCCESS)
            return video_fail("vkCreateImageView(swapchain)", result);
    }

    for (i = 0; i < video->image_count; i++) {
        VkFramebufferCreateInfo framebuffer_info;

        BrMemSet(&framebuffer_info, 0, sizeof(framebuffer_info));
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = video->render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = &video->views[i];
        framebuffer_info.width = video->extent.width;
        framebuffer_info.height = video->extent.height;
        framebuffer_info.layers = 1;

        result = pCreateFramebuffer(video->device, &framebuffer_info, NULL, &video->framebuffers[i]);
        if (result != VK_SUCCESS)
            return video_fail("vkCreateFramebuffer", result);
    }

    video->render_semaphores = BrMemCalloc(video->image_count, sizeof(VkSemaphore), BR_MEMORY_APPLICATION);
    if (video->render_semaphores == NULL)
        return BRE_NO_MEMORY;

    for (i = 0; i < video->image_count; i++) {
        VkSemaphoreCreateInfo semaphore_info;

        BrMemSet(&semaphore_info, 0, sizeof(semaphore_info));
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        result = pCreateSemaphore(video->device, &semaphore_info, NULL, &video->render_semaphores[i]);
        if (result != VK_SUCCESS)
            return video_fail("vkCreateSemaphore", result);
    }

    return BRE_OK;
}

static void video_destroy_swapchain_objects(br_vk_video* video) {
    br_uint_32 i;

    if (video->render_semaphores != NULL) {
        for (i = 0; i < video->image_count; i++) {
            if (video->render_semaphores[i] != VK_NULL_HANDLE)
                pDestroySemaphore(video->device, video->render_semaphores[i], NULL);
        }
        BrMemFree(video->render_semaphores);
        video->render_semaphores = NULL;
    }

    video_destroy_framebuffers_and_views(video);
}

static void video_destroy_swapchain(br_vk_video* video) {
    if (video->device == VK_NULL_HANDLE)
        return;

    video_destroy_swapchain_objects(video);

    if (video->swapchain != VK_NULL_HANDLE) {
        pDestroySwapchainKHR(video->device, video->swapchain, NULL);
        video->swapchain = VK_NULL_HANDLE;
    }
}

static br_error video_recreate_swapchain(br_vk_video* video) {
    VkExtent2D extent;
    VkSwapchainKHR old_swapchain;
    br_error r;

    if (video_get_extent(video, &extent) != BRE_OK || extent.width == 0 || extent.height == 0)
        return BRE_OK;

    pDeviceWaitIdle(video->device);

    old_swapchain = video->swapchain;
    video_destroy_swapchain_objects(video);
    video->swapchain = VK_NULL_HANDLE;

    r = video_create_swapchain(video, old_swapchain);

    if (old_swapchain != VK_NULL_HANDLE)
        pDestroySwapchainKHR(video->device, old_swapchain, NULL);

    return r;
}

static br_error video_create_render_pass(br_vk_video* video) {
    VkAttachmentDescription attachment;
    VkAttachmentReference color_reference;
    VkSubpassDescription subpass;
    VkRenderPassCreateInfo render_pass_info;
    VkResult result;

    BrMemSet(&attachment, 0, sizeof(attachment));
    attachment.format = video->format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    BrMemSet(&color_reference, 0, sizeof(color_reference));
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    BrMemSet(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;

    BrMemSet(&render_pass_info, 0, sizeof(render_pass_info));
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    result = pCreateRenderPass(video->device, &render_pass_info, NULL, &video->render_pass);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateRenderPass", result);

    return BRE_OK;
}

static br_error video_create_shader_module(br_vk_video* video, const uint32_t* code, br_size_t size, VkShaderModule* module) {
    VkShaderModuleCreateInfo module_info;
    VkResult result;

    BrMemSet(&module_info, 0, sizeof(module_info));
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = size;
    module_info.pCode = code;

    result = pCreateShaderModule(video->device, &module_info, NULL, module);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateShaderModule", result);

    return BRE_OK;
}

static br_error video_create_pipeline(br_vk_video* video) {
    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    VkPipelineShaderStageCreateInfo stages[2];
    VkPipelineVertexInputStateCreateInfo vertex_input;
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineMultisampleStateCreateInfo multisample;
    VkPipelineColorBlendAttachmentState blend_attachment;
    VkPipelineColorBlendStateCreateInfo blend;
    VkDynamicState dynamic_states[2];
    VkPipelineDynamicStateCreateInfo dynamic;
    VkPipelineLayoutCreateInfo layout_info;
    VkGraphicsPipelineCreateInfo pipeline_info;
    VkResult result;

    if (video_create_shader_module(video, VKREND_PRESENT_VERT_SPV, sizeof(VKREND_PRESENT_VERT_SPV), &vertex_module) != BRE_OK)
        return BRE_FAIL;

    if (video_create_shader_module(video, VKREND_PRESENT_FRAG_SPV, sizeof(VKREND_PRESENT_FRAG_SPV), &fragment_module) != BRE_OK) {
        pDestroyShaderModule(video->device, vertex_module, NULL);
        return BRE_FAIL;
    }

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

    BrMemSet(&blend_attachment, 0, sizeof(blend_attachment));
    blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    blend_attachment.blendEnable = VK_FALSE;

    BrMemSet(&blend, 0, sizeof(blend));
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attachment;

    BrMemSet(&layout_info, 0, sizeof(layout_info));
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &video->set_layout;

    result = pCreatePipelineLayout(video->device, &layout_info, NULL, &video->pipeline_layout);
    if (result != VK_SUCCESS) {
        pDestroyShaderModule(video->device, fragment_module, NULL);
        pDestroyShaderModule(video->device, vertex_module, NULL);
        return video_fail("vkCreatePipelineLayout", result);
    }

    BrMemSet(&pipeline_info, 0, sizeof(pipeline_info));
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pColorBlendState = &blend;
    pipeline_info.pDynamicState = &dynamic;
    pipeline_info.layout = video->pipeline_layout;
    pipeline_info.renderPass = video->render_pass;
    pipeline_info.subpass = 0;

    result = pCreateGraphicsPipelines(video->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &video->pipeline);
    if (result != VK_SUCCESS) {
        pDestroyShaderModule(video->device, fragment_module, NULL);
        pDestroyShaderModule(video->device, vertex_module, NULL);
        return video_fail("vkCreateGraphicsPipelines", result);
    }

    pDestroyShaderModule(video->device, fragment_module, NULL);
    pDestroyShaderModule(video->device, vertex_module, NULL);

    return BRE_OK;
}

static br_error video_create_descriptors(br_vk_video* video) {
    VkDescriptorSetLayoutBinding binding;
    VkDescriptorSetLayoutCreateInfo layout_info;
    VkDescriptorPoolSize pool_size;
    VkDescriptorPoolCreateInfo pool_info;
    VkDescriptorSetAllocateInfo allocate_info;
    VkSamplerCreateInfo sampler_info;
    VkResult result;

    BrMemSet(&binding, 0, sizeof(binding));
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    BrMemSet(&layout_info, 0, sizeof(layout_info));
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;

    result = pCreateDescriptorSetLayout(video->device, &layout_info, NULL, &video->set_layout);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateDescriptorSetLayout", result);

    BrMemSet(&pool_size, 0, sizeof(pool_size));
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = 1;

    BrMemSet(&pool_info, 0, sizeof(pool_info));
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    result = pCreateDescriptorPool(video->device, &pool_info, NULL, &video->descriptor_pool);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateDescriptorPool", result);

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = video->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &video->set_layout;

    result = pAllocateDescriptorSets(video->device, &allocate_info, &video->descriptor_set);
    if (result != VK_SUCCESS)
        return video_fail("vkAllocateDescriptorSets", result);

    BrMemSet(&sampler_info, 0, sizeof(sampler_info));
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.maxLod = 0.0f;

    result = pCreateSampler(video->device, &sampler_info, NULL, &video->sampler);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateSampler", result);

    return BRE_OK;
}

static br_error video_update_descriptors(br_vk_video* video) {
    VkDescriptorImageInfo frame_info;
    VkWriteDescriptorSet write;

    BrMemSet(&frame_info, 0, sizeof(frame_info));
    frame_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    frame_info.imageView = video->frame_view;
    frame_info.sampler = video->sampler;

    BrMemSet(&write, 0, sizeof(write));
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = video->descriptor_set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &frame_info;

    pUpdateDescriptorSets(video->device, 1, &write, 0, NULL);

    return BRE_OK;
}

static br_error video_create_sync_objects(br_vk_video* video) {
    VkCommandPoolCreateInfo pool_info;
    VkCommandBufferAllocateInfo allocate_info;
    VkFenceCreateInfo fence_info;
    VkSemaphoreCreateInfo semaphore_info;
    VkResult result;

    BrMemSet(&pool_info, 0, sizeof(pool_info));
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = video->queue_family;

    result = pCreateCommandPool(video->device, &pool_info, NULL, &video->command_pool);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateCommandPool", result);

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = video->command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    result = pAllocateCommandBuffers(video->device, &allocate_info, &video->command_buffer);
    if (result != VK_SUCCESS)
        return video_fail("vkAllocateCommandBuffers", result);

    BrMemSet(&fence_info, 0, sizeof(fence_info));
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result = pCreateFence(video->device, &fence_info, NULL, &video->fence);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateFence", result);

    BrMemSet(&semaphore_info, 0, sizeof(semaphore_info));
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = pCreateSemaphore(video->device, &semaphore_info, NULL, &video->acquire_semaphore);
    if (result != VK_SUCCESS)
        return video_fail("vkCreateSemaphore", result);

    return BRE_OK;
}

static br_error video_ensure_frame(br_vk_video* video, br_uint_32 width, br_uint_32 height) {
    VkDeviceSize size = (VkDeviceSize)width * 2 * height;

    if (video->frame_image != VK_NULL_HANDLE && video->frame_width == width && video->frame_height == height)
        return BRE_OK;
    if (width == 0 || height == 0)
        return BRE_FAIL;

    pDeviceWaitIdle(video->device);

    video_destroy_image(video, &video->frame_image, &video->frame_memory, &video->frame_view);
    video_destroy_buffer(video, &video->pixel_buffer, &video->pixel_memory);
    video->pixel_mapped = NULL;
    video->pixel_buffer_size = 0;
    video->frame_initialised = BR_FALSE;

    if (video_create_image(video, width, height, VK_FORMAT_R5G6B5_UNORM_PACK16,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, &video->frame_image, &video->frame_memory,
            &video->frame_view, &video->frame_initialised) != BRE_OK)
        return BRE_FAIL;

    if (video_create_buffer(video, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &video->pixel_buffer, &video->pixel_memory,
            &video->pixel_mapped) != BRE_OK)
        return BRE_FAIL;

    video->frame_width = width;
    video->frame_height = height;
    video->pixel_buffer_size = size;

    return video_update_descriptors(video);
}

static br_error video_recover_device_lost(br_vk_video* video) {
    br_device_vkrend_callback_procs callbacks = video->callbacks;
    br_int_32 width = video->window_width;
    br_int_32 height = video->window_height;
    br_int_32 attempts = video->recovery_attempted;

    if (attempts > 0) {
        BrFailure("VKREND: Vulkan device lost again after recovery");
        video->active = BR_FALSE;
        return BRE_FAIL;
    }

    BrLogPrintf("VKREND: Vulkan device lost, reinitialising\n");

    VIDEOVkDestroy(video);

    if (VIDEOVkCreate(video, &callbacks, width, height) != BRE_OK) {
        BrFailure("VKREND: reinitialisation after device loss failed");
        return BRE_FAIL;
    }

    video->recovery_attempted = attempts + 1;
    return BRE_OK;
}

static br_error video_present_frame(br_vk_video* video, const void* pixels, br_int_32 width, br_int_32 height,
    br_int_32 row_bytes) {
    VkResult result;
    br_uint_32 image_index = 0;
    VkCommandBufferBeginInfo begin_info;
    VkImageMemoryBarrier barrier;
    VkBufferMemoryBarrier buffer_barrier;
    VkRenderPassBeginInfo render_pass_info;
    VkClearValue clear_value;
    VkViewport viewport;
    VkRect2D scissor;
    VkSubmitInfo submit_info;
    VkPresentInfoKHR present_info;
    VkPipelineStageFlags wait_stage;
    br_uint_32 row;

    if (video->window_width <= 0 || video->window_height <= 0)
        return BRE_OK;

    if (video_ensure_frame(video, (br_uint_32)width, (br_uint_32)height) != BRE_OK)
        return BRE_FAIL;

    if (video->frame_image == VK_NULL_HANDLE)
        return BRE_OK;

    result = pWaitForFences(video->device, 1, &video->fence, VK_TRUE, UINT64_MAX);
    if (result == VK_ERROR_DEVICE_LOST) {
        video_recover_device_lost(video);
        return BRE_OK;
    }
    if (result != VK_SUCCESS)
        return video_fail("vkWaitForFences", result);

    for (row = 0; row < (br_uint_32)height; row++) {
        BrMemCpy((br_uint_8*)video->pixel_mapped + (size_t)row * (br_size_t)width * 2,
            (const br_uint_8*)pixels + (size_t)row * row_bytes, (br_size_t)width * 2);
    }

    result = pAcquireNextImageKHR(video->device, video->swapchain, UINT64_MAX, video->acquire_semaphore, VK_NULL_HANDLE,
        &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        if (video_recreate_swapchain(video) != BRE_OK)
            video->active = BR_FALSE;
        return BRE_OK;
    }
    if (result == VK_ERROR_DEVICE_LOST) {
        video_recover_device_lost(video);
        return BRE_OK;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        return video_fail("vkAcquireNextImageKHR", result);

    pResetFences(video->device, 1, &video->fence);
    pResetCommandBuffer(video->command_buffer, 0);

    BrMemSet(&begin_info, 0, sizeof(begin_info));
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = pBeginCommandBuffer(video->command_buffer, &begin_info);
    if (result != VK_SUCCESS)
        return video_fail("vkBeginCommandBuffer", result);

    BrMemSet(&buffer_barrier, 0, sizeof(buffer_barrier));
    buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buffer_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    buffer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buffer_barrier.buffer = video->pixel_buffer;
    buffer_barrier.offset = 0;
    buffer_barrier.size = VK_WHOLE_SIZE;

    pCmdPipelineBarrier(video->command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1,
        &buffer_barrier, 0, NULL);

    BrMemSet(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = video->frame_initialised ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = video->frame_initialised ? VK_ACCESS_SHADER_READ_BIT : 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = video->frame_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    pCmdPipelineBarrier(video->command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    {
        VkBufferImageCopy region;

        BrMemSet(&region, 0, sizeof(region));
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = video->frame_width;
        region.imageExtent.height = video->frame_height;
        region.imageExtent.depth = 1;

        pCmdCopyBufferToImage(video->command_buffer, video->pixel_buffer, video->frame_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    video->frame_initialised = BR_TRUE;

    BrMemSet(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = video->frame_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    pCmdPipelineBarrier(video->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, NULL, 0, NULL, 1, &barrier);

    BrMemSet(&clear_value, 0, sizeof(clear_value));
    clear_value.color.float32[3] = 1.0f;

    BrMemSet(&render_pass_info, 0, sizeof(render_pass_info));
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = video->render_pass;
    render_pass_info.framebuffer = video->framebuffers[image_index];
    render_pass_info.renderArea.extent = video->extent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_value;

    pCmdBeginRenderPass(video->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)video->extent.width;
    viewport.height = (float)video->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    pCmdSetViewport(video->command_buffer, 0, 1, &viewport);

    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = video->extent;
    pCmdSetScissor(video->command_buffer, 0, 1, &scissor);

    pCmdBindPipeline(video->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, video->pipeline);
    pCmdBindDescriptorSets(video->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, video->pipeline_layout, 0, 1,
        &video->descriptor_set, 0, NULL);
    pCmdDraw(video->command_buffer, 3, 1, 0, 0);
    pCmdEndRenderPass(video->command_buffer);

    result = pEndCommandBuffer(video->command_buffer);
    if (result != VK_SUCCESS)
        return video_fail("vkEndCommandBuffer", result);

    wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    BrMemSet(&submit_info, 0, sizeof(submit_info));
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &video->acquire_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &video->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &video->render_semaphores[image_index];

    result = pQueueSubmit(video->queue, 1, &submit_info, video->fence);
    if (result == VK_ERROR_DEVICE_LOST) {
        video_recover_device_lost(video);
        return BRE_OK;
    }
    if (result != VK_SUCCESS)
        return video_fail("vkQueueSubmit", result);

    BrMemSet(&present_info, 0, sizeof(present_info));
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &video->render_semaphores[image_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &video->swapchain;
    present_info.pImageIndices = &image_index;

    result = pQueuePresentKHR(video->queue, &present_info);
    if (result == VK_ERROR_DEVICE_LOST) {
        video_recover_device_lost(video);
        return BRE_OK;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        if (video_recreate_swapchain(video) != BRE_OK)
            video->active = BR_FALSE;
    } else if (result != VK_SUCCESS) {
        return video_fail("vkQueuePresentKHR", result);
    }

    return BRE_OK;
}

static void video_destroy_all(br_vk_video* video) {
    if (video->device != VK_NULL_HANDLE)
        pDeviceWaitIdle(video->device);

    if (video->device != VK_NULL_HANDLE) {
        video_destroy_swapchain(video);
        video_destroy_image(video, &video->frame_image, &video->frame_memory, &video->frame_view);
        video_destroy_buffer(video, &video->pixel_buffer, &video->pixel_memory);
        video->pixel_mapped = NULL;
        video->pixel_buffer_size = 0;

        if (video->fence != VK_NULL_HANDLE) {
            pDestroyFence(video->device, video->fence, NULL);
            video->fence = VK_NULL_HANDLE;
        }
        if (video->acquire_semaphore != VK_NULL_HANDLE) {
            pDestroySemaphore(video->device, video->acquire_semaphore, NULL);
            video->acquire_semaphore = VK_NULL_HANDLE;
        }
        if (video->command_pool != VK_NULL_HANDLE) {
            pDestroyCommandPool(video->device, video->command_pool, NULL);
            video->command_pool = VK_NULL_HANDLE;
        }
        if (video->sampler != VK_NULL_HANDLE) {
            pDestroySampler(video->device, video->sampler, NULL);
            video->sampler = VK_NULL_HANDLE;
        }
        if (video->pipeline != VK_NULL_HANDLE) {
            pDestroyPipeline(video->device, video->pipeline, NULL);
            video->pipeline = VK_NULL_HANDLE;
        }
        if (video->pipeline_layout != VK_NULL_HANDLE) {
            pDestroyPipelineLayout(video->device, video->pipeline_layout, NULL);
            video->pipeline_layout = VK_NULL_HANDLE;
        }
        if (video->descriptor_pool != VK_NULL_HANDLE) {
            pDestroyDescriptorPool(video->device, video->descriptor_pool, NULL);
            video->descriptor_pool = VK_NULL_HANDLE;
        }
        if (video->set_layout != VK_NULL_HANDLE) {
            pDestroyDescriptorSetLayout(video->device, video->set_layout, NULL);
            video->set_layout = VK_NULL_HANDLE;
        }
        if (video->render_pass != VK_NULL_HANDLE) {
            pDestroyRenderPass(video->device, video->render_pass, NULL);
            video->render_pass = VK_NULL_HANDLE;
        }

        pDestroyDevice(video->device, NULL);
        video->device = VK_NULL_HANDLE;
    }

    if (video->surface != VK_NULL_HANDLE && pDestroySurfaceKHR != NULL) {
        pDestroySurfaceKHR(video->instance, video->surface, NULL);
        video->surface = VK_NULL_HANDLE;
    }

    if (video->instance != VK_NULL_HANDLE && pDestroyInstance != NULL) {
        pDestroyInstance(video->instance, NULL);
        video->instance = VK_NULL_HANDLE;
    }

    if (video->callbacks.unload_library != NULL)
        video->callbacks.unload_library();

    video->active = BR_FALSE;
}

br_error VIDEOVkCreate(br_vk_video* video, const br_device_vkrend_callback_procs* callbacks, br_int_32 width,
    br_int_32 height) {
    BrMemSet(video, 0, sizeof(*video));

    if (!video_copy_callbacks(video, callbacks))
        return BRE_FAIL;

    if (video->callbacks.window == NULL || video->callbacks.load_library == NULL
        || video->callbacks.unload_library == NULL || video->callbacks.get_vk_get_instance_proc_addr == NULL
        || video->callbacks.get_instance_extensions == NULL || video->callbacks.create_surface == NULL) {
        BrFailure("VKREND: incomplete host callbacks");
        return BRE_FAIL;
    }

    video->window_width = width;
    video->window_height = height;

    if (video_host_failed(video->callbacks.load_library(NULL))) {
        BrFailure("VKREND: could not load a Vulkan loader");
        return BRE_FAIL;
    }

    gpa = (PFN_vkGetInstanceProcAddr)video->callbacks.get_vk_get_instance_proc_addr();
    if (gpa == NULL) {
        BrFailure("VKREND: host has no vkGetInstanceProcAddr");
        video->callbacks.unload_library();
        return BRE_FAIL;
    }

    if (video_create_instance(video) != BRE_OK || video_pick_physical_device(video) != BRE_OK
        || video_check_frame_format(video) != BRE_OK || video_create_device(video) != BRE_OK
        || video_choose_surface_format(video) != BRE_OK || video_create_render_pass(video) != BRE_OK
        || video_create_descriptors(video) != BRE_OK || video_create_pipeline(video) != BRE_OK
        || video_create_sync_objects(video) != BRE_OK || video_create_swapchain(video, VK_NULL_HANDLE) != BRE_OK) {
        video_destroy_all(video);
        BrMemSet(video, 0, sizeof(*video));
        return BRE_FAIL;
    }

    video->active = BR_TRUE;

    BrLogPrintf("VKREND: Vulkan swapchain %dx%d, format %d, %d images\n", (int)video->extent.width,
        (int)video->extent.height, (int)video->format, (int)video->image_count);

    return BRE_OK;
}

void VIDEOVkDestroy(br_vk_video* video) {
    if (video == NULL)
        return;

    if (video->instance == VK_NULL_HANDLE && video->device == VK_NULL_HANDLE) {
        BrMemSet(video, 0, sizeof(*video));
        return;
    }

    video_destroy_all(video);
    BrMemSet(video, 0, sizeof(*video));
}

br_error VIDEOVkResize(br_vk_video* video, br_int_32 width, br_int_32 height) {
    if (video == NULL || !video->active)
        return BRE_FAIL;

    if (width == video->window_width && height == video->window_height)
        return BRE_OK;

    video->window_width = width;
    video->window_height = height;

    if (width <= 0 || height <= 0)
        return BRE_OK;

    return video_recreate_swapchain(video);
}

br_error VIDEOVkPresent(br_vk_video* video, const void* pixels, br_int_32 width, br_int_32 height, br_int_32 row_bytes) {
    if (video == NULL || !video->active || pixels == NULL)
        return BRE_FAIL;
    if (width <= 0 || height <= 0 || row_bytes < width * 2)
        return BRE_FAIL;

    return video_present_frame(video, pixels, width, height, row_bytes);
}
