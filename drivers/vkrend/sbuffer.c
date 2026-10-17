#include <stddef.h>
#include <stdio.h>

#include "drv.h"

static struct br_buffer_stored_dispatch bufferStoredDispatch;

#define F(f) offsetof(struct br_buffer_stored, f)

static struct br_tv_template_entry bufferStoredTemplateEntries[] = {
    { BRT(IDENTIFIER_CSTR), F(identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
};
#undef F

#define VKREND_TEXTURE_MAX_DIMENSION 16384
#define VKREND_TEXTURE_PALETTE_ENTRIES 256

typedef struct vk_texture_functions {
    PFN_vkGetPhysicalDeviceFormatProperties GetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
    PFN_vkCreateImage CreateImage;
    PFN_vkDestroyImage DestroyImage;
    PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements;
    PFN_vkBindImageMemory BindImageMemory;
    PFN_vkCreateImageView CreateImageView;
    PFN_vkDestroyImageView DestroyImageView;
    PFN_vkCreateBuffer CreateBuffer;
    PFN_vkDestroyBuffer DestroyBuffer;
    PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements;
    PFN_vkBindBufferMemory BindBufferMemory;
    PFN_vkAllocateMemory AllocateMemory;
    PFN_vkFreeMemory FreeMemory;
    PFN_vkMapMemory MapMemory;
    PFN_vkUnmapMemory UnmapMemory;
    PFN_vkCmdPipelineBarrier CmdPipelineBarrier;
    PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage;
    PFN_vkBeginCommandBuffer BeginCommandBuffer;
    PFN_vkEndCommandBuffer EndCommandBuffer;
    PFN_vkCreateFence CreateFence;
    PFN_vkDestroyFence DestroyFence;
    PFN_vkWaitForFences WaitForFences;
    PFN_vkQueueSubmit QueueSubmit;
    PFN_vkCreateCommandPool CreateCommandPool;
    PFN_vkDestroyCommandPool DestroyCommandPool;
    PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
} vk_texture_functions;

static vk_texture_functions sbuffer_gpu;
static VkInstance sbuffer_instance;
static br_boolean sbuffer_loaded;
static br_int_32 sbuffer_format_supported = -1;

static br_boolean sbuffer_logged_source;
static br_boolean sbuffer_logged_type;
static br_boolean sbuffer_logged_palette;
static br_boolean sbuffer_logged_palette_type;
static br_boolean sbuffer_logged_clut_palette;
static br_boolean sbuffer_logged_dimensions;
static br_boolean sbuffer_logged_format;
static br_boolean sbuffer_logged_allocation;
static br_boolean sbuffer_logged_upload;
static br_boolean sbuffer_logged_device;

static void BufferTextureLogOnce(br_boolean* flag, const char* message) {
    if (*flag)
        return;

    *flag = BR_TRUE;
    BrLogPrintf("VKREND: %s\n", (char*)message);
}

static br_boolean BufferTextureLoadFunctions(VkInstance instance) {
    PFN_vkGetInstanceProcAddr gpa;

    if (sbuffer_loaded && sbuffer_instance == instance)
        return BR_TRUE;

    sbuffer_loaded = BR_FALSE;
    sbuffer_instance = instance;
    sbuffer_format_supported = -1;

    gpa = VIDEOVkGetInstanceProcAddr();
    if (gpa == NULL)
        return BR_FALSE;

#define VKREND_TEXTURE_LOAD(name)                                              \
    do {                                                                       \
        sbuffer_gpu.name = (PFN_vk##name)gpa(instance, "vk" #name);            \
        if (sbuffer_gpu.name == NULL)                                          \
            return BR_FALSE;                                                   \
    } while (0)

    VKREND_TEXTURE_LOAD(GetPhysicalDeviceFormatProperties);
    VKREND_TEXTURE_LOAD(GetPhysicalDeviceMemoryProperties);
    VKREND_TEXTURE_LOAD(CreateImage);
    VKREND_TEXTURE_LOAD(DestroyImage);
    VKREND_TEXTURE_LOAD(GetImageMemoryRequirements);
    VKREND_TEXTURE_LOAD(BindImageMemory);
    VKREND_TEXTURE_LOAD(CreateImageView);
    VKREND_TEXTURE_LOAD(DestroyImageView);
    VKREND_TEXTURE_LOAD(CreateBuffer);
    VKREND_TEXTURE_LOAD(DestroyBuffer);
    VKREND_TEXTURE_LOAD(GetBufferMemoryRequirements);
    VKREND_TEXTURE_LOAD(BindBufferMemory);
    VKREND_TEXTURE_LOAD(AllocateMemory);
    VKREND_TEXTURE_LOAD(FreeMemory);
    VKREND_TEXTURE_LOAD(MapMemory);
    VKREND_TEXTURE_LOAD(UnmapMemory);
    VKREND_TEXTURE_LOAD(CmdPipelineBarrier);
    VKREND_TEXTURE_LOAD(CmdCopyBufferToImage);
    VKREND_TEXTURE_LOAD(BeginCommandBuffer);
    VKREND_TEXTURE_LOAD(EndCommandBuffer);
    VKREND_TEXTURE_LOAD(CreateFence);
    VKREND_TEXTURE_LOAD(DestroyFence);
    VKREND_TEXTURE_LOAD(WaitForFences);
    VKREND_TEXTURE_LOAD(QueueSubmit);
    VKREND_TEXTURE_LOAD(CreateCommandPool);
    VKREND_TEXTURE_LOAD(DestroyCommandPool);
    VKREND_TEXTURE_LOAD(AllocateCommandBuffers);

#undef VKREND_TEXTURE_LOAD

    sbuffer_loaded = BR_TRUE;
    return BR_TRUE;
}

static br_boolean BufferTextureFindMemoryType(br_vk_video* video, br_uint_32 type_bits,
    VkMemoryPropertyFlags properties, br_uint_32* index) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    br_uint_32 i;

    sbuffer_gpu.GetPhysicalDeviceMemoryProperties(video->physical_device, &memory_properties);

    for (i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_bits & (1u << i)) != 0 && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            *index = i;
            return BR_TRUE;
        }
    }

    return BR_FALSE;
}

static br_boolean BufferTextureFormatSupported(br_vk_video* video) {
    VkFormatProperties properties;

    if (sbuffer_format_supported >= 0)
        return sbuffer_format_supported == 1;

    BrMemSet(&properties, 0, sizeof(properties));
    sbuffer_gpu.GetPhysicalDeviceFormatProperties(video->physical_device, VK_FORMAT_R8G8B8A8_UNORM, &properties);

    if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0
        || (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) == 0) {
        sbuffer_format_supported = 0;
        BufferTextureLogOnce(&sbuffer_logged_format, "device cannot use RGBA8 colour maps");
        return BR_FALSE;
    }

    sbuffer_format_supported = 1;
    return BR_TRUE;
}

static void BufferTextureDestroyImage(VkDevice device, VkImage* image, VkDeviceMemory* memory, VkImageView* view) {
    if (*view != VK_NULL_HANDLE) {
        sbuffer_gpu.DestroyImageView(device, *view, NULL);
        *view = VK_NULL_HANDLE;
    }
    if (*image != VK_NULL_HANDLE) {
        sbuffer_gpu.DestroyImage(device, *image, NULL);
        *image = VK_NULL_HANDLE;
    }
    if (*memory != VK_NULL_HANDLE) {
        sbuffer_gpu.FreeMemory(device, *memory, NULL);
        *memory = VK_NULL_HANDLE;
    }
}

static void BufferTextureDestroyBuffer(VkDevice device, VkBuffer* buffer, VkDeviceMemory* memory) {
    if (*buffer != VK_NULL_HANDLE) {
        sbuffer_gpu.DestroyBuffer(device, *buffer, NULL);
        *buffer = VK_NULL_HANDLE;
    }
    if (*memory != VK_NULL_HANDLE) {
        sbuffer_gpu.FreeMemory(device, *memory, NULL);
        *memory = VK_NULL_HANDLE;
    }
}

static br_boolean BufferTextureCreateImage(br_vk_video* video, br_uint_32 width, br_uint_32 height, VkImage* image,
    VkDeviceMemory* memory, VkImageView* view) {
    VkImageCreateInfo image_info;
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocate_info;
    VkImageViewCreateInfo view_info;
    br_uint_32 memory_type;

    BrMemSet(&image_info, 0, sizeof(image_info));
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (sbuffer_gpu.CreateImage(video->device, &image_info, NULL, image) != VK_SUCCESS)
        return BR_FALSE;

    sbuffer_gpu.GetImageMemoryRequirements(video->device, *image, &requirements);
    if (!BufferTextureFindMemoryType(video, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &memory_type)) {
        BufferTextureDestroyImage(video->device, image, memory, view);
        return BR_FALSE;
    }

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = memory_type;

    if (sbuffer_gpu.AllocateMemory(video->device, &allocate_info, NULL, memory) != VK_SUCCESS) {
        BufferTextureDestroyImage(video->device, image, memory, view);
        return BR_FALSE;
    }
    if (sbuffer_gpu.BindImageMemory(video->device, *image, *memory, 0) != VK_SUCCESS) {
        BufferTextureDestroyImage(video->device, image, memory, view);
        return BR_FALSE;
    }

    BrMemSet(&view_info, 0, sizeof(view_info));
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = *image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    if (sbuffer_gpu.CreateImageView(video->device, &view_info, NULL, view) != VK_SUCCESS) {
        BufferTextureDestroyImage(video->device, image, memory, view);
        return BR_FALSE;
    }

    return BR_TRUE;
}

static br_boolean BufferTextureCreateStaging(br_vk_video* video, br_uint_32 bytes, VkBuffer* buffer,
    VkDeviceMemory* memory, void** mapped) {
    VkBufferCreateInfo buffer_info;
    VkMemoryRequirements requirements;
    VkMemoryAllocateInfo allocate_info;
    br_uint_32 memory_type;

    BrMemSet(&buffer_info, 0, sizeof(buffer_info));
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (sbuffer_gpu.CreateBuffer(video->device, &buffer_info, NULL, buffer) != VK_SUCCESS)
        return BR_FALSE;

    sbuffer_gpu.GetBufferMemoryRequirements(video->device, *buffer, &requirements);
    if (!BufferTextureFindMemoryType(video, requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memory_type)) {
        BufferTextureDestroyBuffer(video->device, buffer, memory);
        return BR_FALSE;
    }

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = memory_type;

    if (sbuffer_gpu.AllocateMemory(video->device, &allocate_info, NULL, memory) != VK_SUCCESS) {
        BufferTextureDestroyBuffer(video->device, buffer, memory);
        return BR_FALSE;
    }
    if (sbuffer_gpu.BindBufferMemory(video->device, *buffer, *memory, 0) != VK_SUCCESS) {
        BufferTextureDestroyBuffer(video->device, buffer, memory);
        return BR_FALSE;
    }

    if (sbuffer_gpu.MapMemory(video->device, *memory, 0, VK_WHOLE_SIZE, 0, mapped) != VK_SUCCESS) {
        BufferTextureDestroyBuffer(video->device, buffer, memory);
        return BR_FALSE;
    }

    return BR_TRUE;
}

static br_boolean BufferTextureUpload(br_vk_video* video, VkImage image, VkBuffer staging, br_uint_32 width,
    br_uint_32 height, br_boolean reuse) {
    VkCommandPoolCreateInfo pool_info;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo allocate_info;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkCommandBufferBeginInfo begin_info;
    VkImageMemoryBarrier barrier;
    VkBufferImageCopy region;
    VkFenceCreateInfo fence_info;
    VkFence fence = VK_NULL_HANDLE;
    VkSubmitInfo submit_info;
    br_boolean submitted = BR_FALSE;
    br_boolean waited = BR_FALSE;
    br_boolean ok = BR_FALSE;

    BrMemSet(&pool_info, 0, sizeof(pool_info));
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = video->queue_family;

    if (sbuffer_gpu.CreateCommandPool(video->device, &pool_info, NULL, &pool) != VK_SUCCESS)
        goto done;

    BrMemSet(&allocate_info, 0, sizeof(allocate_info));
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    if (sbuffer_gpu.AllocateCommandBuffers(video->device, &allocate_info, &command_buffer) != VK_SUCCESS)
        goto done;

    BrMemSet(&begin_info, 0, sizeof(begin_info));
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (sbuffer_gpu.BeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        goto done;

    BrMemSet(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = reuse ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = reuse ? VK_ACCESS_SHADER_READ_BIT : 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sbuffer_gpu.CmdPipelineBarrier(command_buffer, reuse ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                                        : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    BrMemSet(&region, 0, sizeof(region));
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    sbuffer_gpu.CmdCopyBufferToImage(command_buffer, staging, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    sbuffer_gpu.CmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, NULL, 0, NULL, 1, &barrier);

    if (sbuffer_gpu.EndCommandBuffer(command_buffer) != VK_SUCCESS)
        goto done;

    BrMemSet(&fence_info, 0, sizeof(fence_info));
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (sbuffer_gpu.CreateFence(video->device, &fence_info, NULL, &fence) != VK_SUCCESS)
        goto done;

    BrMemSet(&submit_info, 0, sizeof(submit_info));
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    if (sbuffer_gpu.QueueSubmit(video->queue, 1, &submit_info, fence) != VK_SUCCESS)
        goto done;

    submitted = BR_TRUE;
    waited = sbuffer_gpu.WaitForFences(video->device, 1, &fence, VK_TRUE, UINT64_MAX) == VK_SUCCESS;
    ok = waited;

done:
    if (fence != VK_NULL_HANDLE && (!submitted || waited))
        sbuffer_gpu.DestroyFence(video->device, fence, NULL);
    if (pool != VK_NULL_HANDLE && (!submitted || waited))
        sbuffer_gpu.DestroyCommandPool(video->device, pool, NULL);

    if (!ok)
        BufferTextureLogOnce(&sbuffer_logged_upload, "colour map upload failed");

    return ok;
}

static br_uint_8 BufferTextureExpand5(br_uint_32 value) {
    return (br_uint_8)((value << 3) | (value >> 2));
}

static br_uint_8 BufferTextureExpand6(br_uint_32 value) {
    return (br_uint_8)((value << 2) | (value >> 4));
}

static br_pixelmap* BufferTexturePalette(br_pixelmap* source) {
    if (source != NULL && source->type == BR_PMT_INDEX_8)
        return source->map;

    return NULL;
}

static br_boolean BufferTexturePaletteUsable(br_pixelmap* palette) {
    if (palette == NULL || palette->pixels == NULL)
        return BR_FALSE;

    if ((br_uint_32)palette->width * (br_uint_32)palette->height < VKREND_TEXTURE_PALETTE_ENTRIES)
        return BR_FALSE;

    switch (palette->type) {
    case BR_PMT_RGB_565:
    case BR_PMT_RGB_888:
    case BR_PMT_RGBX_888:
    case BR_PMT_RGBA_8888:
    case BR_PMT_ARGB_8888:
        return BR_TRUE;

    default:
        return BR_FALSE;
    }
}

static br_device_clut* BufferTextureClut(struct br_buffer_stored* self, br_pixelmap* source) {
    br_device_clut* queried = NULL;

    if (!self->clut_query_done) {
        self->clut_query_done = BR_TRUE;
        self->clut_queried = NULL;

        if (source != NULL && ObjectQuery(source, &queried, BRT_CLUT_O) == BRE_OK)
            self->clut_queried = queried;
    }

    if (self->clut_queried != NULL)
        return self->clut_queried;

    if (self->device != NULL)
        return self->device->clut;

    return NULL;
}

static br_device_clut* BufferTextureSourceClut(struct br_buffer_stored* self, br_pixelmap* source) {
    if (source == NULL || source->type != BR_PMT_INDEX_8)
        return NULL;

    if (BufferTexturePaletteUsable(BufferTexturePalette(source)))
        return NULL;

    return BufferTextureClut(self, source);
}

static void BufferTextureClutColours(br_device_clut* clut, br_uint_32* colours) {
    br_uint_32 i;

    for (i = 0; i < VKREND_TEXTURE_PALETTE_ENTRIES; i++)
        colours[i] = clut->entries[i];
}

static br_boolean BufferTextureReadPalette(br_pixelmap* palette, br_uint_32* colours) {
    br_uint_8* pixels;
    br_uint_8* base;
    br_uint_32 i;

    if (palette == NULL || palette->pixels == NULL)
        return BR_FALSE;

    if ((br_uint_32)palette->width * (br_uint_32)palette->height < VKREND_TEXTURE_PALETTE_ENTRIES)
        return BR_FALSE;

    pixels = (br_uint_8*)palette->pixels;

    switch (palette->type) {
    case BR_PMT_RGB_888:
        base = pixels + (br_int_32)palette->base_y * palette->row_bytes + palette->base_x * 3;
        for (i = 0; i < VKREND_TEXTURE_PALETTE_ENTRIES; i++) {
            colours[i] = ((br_uint_32)base[i * 3 + 2] << 16) | ((br_uint_32)base[i * 3 + 1] << 8)
                | (br_uint_32)base[i * 3];
        }
        return BR_TRUE;

    case BR_PMT_RGBX_888:
    case BR_PMT_RGBA_8888:
    case BR_PMT_ARGB_8888:
        base = pixels + (br_int_32)palette->base_y * palette->row_bytes + palette->base_x * 4;
        for (i = 0; i < VKREND_TEXTURE_PALETTE_ENTRIES; i++) {
            colours[i] = ((br_uint_32)base[i * 4 + 2] << 16) | ((br_uint_32)base[i * 4 + 1] << 8)
                | (br_uint_32)base[i * 4];
        }
        return BR_TRUE;

    case BR_PMT_RGB_565:
        base = pixels + (br_int_32)palette->base_y * palette->row_bytes + palette->base_x * 2;
        for (i = 0; i < VKREND_TEXTURE_PALETTE_ENTRIES; i++) {
            br_uint_32 value = (br_uint_32)base[i * 2] | ((br_uint_32)base[i * 2 + 1] << 8);

            colours[i] = ((br_uint_32)BufferTextureExpand5((value >> 11) & 31) << 16)
                | ((br_uint_32)BufferTextureExpand6((value >> 5) & 63) << 8)
                | (br_uint_32)BufferTextureExpand5(value & 31);
        }
        return BR_TRUE;

    default:
        return BR_FALSE;
    }
}

static br_boolean BufferTextureSourceSupported(br_pixelmap* source, br_pixelmap* palette, br_device_clut* clut) {
    switch (source->type) {
    case BR_PMT_INDEX_8:
        break;

    case BR_PMT_RGB_565:
    case BR_PMT_RGB_888:
    case BR_PMT_RGBX_888:
    case BR_PMT_RGBA_8888:
    case BR_PMT_ARGB_8888:
        return BR_TRUE;

    default:
        BufferTextureLogOnce(&sbuffer_logged_type, "colour map pixel type is not supported");
        return BR_FALSE;
    }

    if (BufferTexturePaletteUsable(palette))
        return BR_TRUE;

    if (palette != NULL && palette->pixels != NULL && clut == NULL) {
        BufferTextureLogOnce(&sbuffer_logged_palette_type, "palette pixel type is not supported");
        return BR_FALSE;
    }

    if (clut != NULL)
        return BR_TRUE;

    BufferTextureLogOnce(&sbuffer_logged_palette, "indexed colour map has no palette");
    return BR_FALSE;
}

static br_boolean BufferTextureConvert(br_pixelmap* source, br_pixelmap* palette, br_device_clut* clut,
    br_uint_8* dest) {
    br_uint_32 colours[VKREND_TEXTURE_PALETTE_ENTRIES];
    br_uint_32 x;
    br_uint_32 y;
    br_uint_32 width = source->width;
    br_uint_32 height = source->height;
    br_uint_8* pixels = (br_uint_8*)source->pixels;
    br_uint_8* row;
    br_uint_8* texel;
    br_uint_8* out = dest;

    switch (source->type) {
    case BR_PMT_INDEX_8:
        if (!BufferTextureReadPalette(palette, colours)) {
            if (clut == NULL)
                return BR_FALSE;

            BufferTextureClutColours(clut, colours);
            BufferTextureLogOnce(&sbuffer_logged_clut_palette, "colour map palette resolved through the device CLUT");
        }

        for (y = 0; y < height; y++) {
            row = pixels + ((br_int_32)source->base_y + (br_int_32)y) * source->row_bytes + source->base_x;

            for (x = 0; x < width; x++) {
                br_uint_32 colour = colours[row[x]];

                *out++ = (br_uint_8)BR_RED(colour);
                *out++ = (br_uint_8)BR_GRN(colour);
                *out++ = (br_uint_8)BR_BLU(colour);
                *out++ = 255;
            }
        }
        return BR_TRUE;

    case BR_PMT_RGB_565:
        for (y = 0; y < height; y++) {
            row = pixels + ((br_int_32)source->base_y + (br_int_32)y) * source->row_bytes + source->base_x * 2;

            for (x = 0; x < width; x++) {
                br_uint_32 value = (br_uint_32)row[x * 2] | ((br_uint_32)row[x * 2 + 1] << 8);

                *out++ = BufferTextureExpand5((value >> 11) & 31);
                *out++ = BufferTextureExpand6((value >> 5) & 63);
                *out++ = BufferTextureExpand5(value & 31);
                *out++ = 255;
            }
        }
        return BR_TRUE;

    case BR_PMT_RGB_888:
        for (y = 0; y < height; y++) {
            row = pixels + ((br_int_32)source->base_y + (br_int_32)y) * source->row_bytes + source->base_x * 3;

            for (x = 0; x < width; x++) {
                texel = row + x * 3;

                *out++ = texel[2];
                *out++ = texel[1];
                *out++ = texel[0];
                *out++ = 255;
            }
        }
        return BR_TRUE;

    case BR_PMT_RGBX_888:
    case BR_PMT_RGBA_8888:
    case BR_PMT_ARGB_8888:
        for (y = 0; y < height; y++) {
            row = pixels + ((br_int_32)source->base_y + (br_int_32)y) * source->row_bytes + source->base_x * 4;

            for (x = 0; x < width; x++) {
                texel = row + x * 4;

                *out++ = texel[2];
                *out++ = texel[1];
                *out++ = texel[0];
                *out++ = 255;
            }
        }
        return BR_TRUE;

    default:
        return BR_FALSE;
    }
}

static void BufferTextureAbandon(struct br_buffer_stored* self) {
    self->texture.image = VK_NULL_HANDLE;
    self->texture.view = VK_NULL_HANDLE;
    self->texture.width = 0;
    self->texture.height = 0;
    self->texture_memory = VK_NULL_HANDLE;
    self->texture_device = VK_NULL_HANDLE;
    self->staging_buffer = VK_NULL_HANDLE;
    self->staging_memory = VK_NULL_HANDLE;
    self->staging_mapped = NULL;
    self->staging_bytes = 0;
}

static void BufferTextureRelease(struct br_buffer_stored* self) {
    if (sbuffer_loaded && self->texture_device != VK_NULL_HANDLE && self->video != NULL && self->video->active
        && self->video->device == self->texture_device) {
        if (self->staging_mapped != NULL) {
            sbuffer_gpu.UnmapMemory(self->texture_device, self->staging_memory);
            self->staging_mapped = NULL;
        }

        BufferTextureDestroyBuffer(self->texture_device, &self->staging_buffer, &self->staging_memory);
        BufferTextureDestroyImage(self->texture_device, &self->texture.image, &self->texture_memory,
            &self->texture.view);
    }

    BufferTextureAbandon(self);
    self->video = NULL;
}

static br_boolean BufferTextureBuild(struct br_buffer_stored* self, br_vk_video* video, br_pixelmap* source,
    br_pixelmap* palette, br_device_clut* clut) {
    br_uint_32 width = source->width;
    br_uint_32 height = source->height;
    br_uint_32 bytes = width * height * 4;
    br_boolean reuse;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;

    if (width == 0 || height == 0 || width > VKREND_TEXTURE_MAX_DIMENSION || height > VKREND_TEXTURE_MAX_DIMENSION) {
        BufferTextureLogOnce(&sbuffer_logged_dimensions, "colour map size is not supported");
        return BR_FALSE;
    }

    if (!BufferTextureFormatSupported(video))
        return BR_FALSE;

    if (!BufferTextureSourceSupported(source, palette, clut))
        return BR_FALSE;

    self->video = video;

    if (self->texture.image != VK_NULL_HANDLE && self->texture_device != video->device) {
        BufferTextureLogOnce(&sbuffer_logged_device, "vulkan device changed, colour map recreated");
        BufferTextureAbandon(self);
    }

    reuse = self->texture.image != VK_NULL_HANDLE && self->texture.width == width && self->texture.height == height;

    if (!reuse
        && !BufferTextureCreateImage(video, width, height, &image, &image_memory, &view)) {
        BufferTextureLogOnce(&sbuffer_logged_allocation, "colour map image allocation failed");
        return BR_FALSE;
    }

    if (self->staging_buffer == VK_NULL_HANDLE || self->staging_bytes < bytes) {
        BufferTextureDestroyBuffer(video->device, &self->staging_buffer, &self->staging_memory);
        self->staging_mapped = NULL;
        self->staging_bytes = 0;

        if (!BufferTextureCreateStaging(video, bytes, &self->staging_buffer, &self->staging_memory,
                &self->staging_mapped)) {
            BufferTextureLogOnce(&sbuffer_logged_allocation, "colour map staging allocation failed");
            BufferTextureDestroyImage(video->device, &image, &image_memory, &view);
            return BR_FALSE;
        }

        self->staging_bytes = bytes;
    }

    self->texture_device = video->device;

    if (!BufferTextureConvert(source, palette, clut, (br_uint_8*)self->staging_mapped)) {
        BufferTextureDestroyImage(video->device, &image, &image_memory, &view);
        return BR_FALSE;
    }

    if (!BufferTextureUpload(video, reuse ? self->texture.image : image, self->staging_buffer, width, height, reuse)) {
        BufferTextureDestroyImage(video->device, &image, &image_memory, &view);

        if (reuse) {
            BufferTextureDestroyImage(self->texture_device, &self->texture.image, &self->texture_memory,
                &self->texture.view);
        }

        return BR_FALSE;
    }

    if (!reuse) {
        if (self->texture.image != VK_NULL_HANDLE) {
            BufferTextureDestroyImage(self->texture_device, &self->texture.image, &self->texture_memory,
                &self->texture.view);
        }

        self->texture.image = image;
        self->texture.view = view;
        self->texture_memory = image_memory;
    }

    self->texture.width = width;
    self->texture.height = height;
    self->texture.generation = self->texture.generation + 1;
    self->texture_device = video->device;
    self->built_revision = self->revision;
    self->built_source = source;
    self->built_palette = palette;
    self->built_clut = clut;
    self->built_clut_revision = clut != NULL ? clut->revision : 0;
    self->video = video;
    self->unavailable = BR_FALSE;

    return BR_TRUE;
}

br_boolean BufferStoredVkGetTexture(struct br_buffer_stored* stored, br_vk_video* video, vk_buffer_texture* texture) {
    br_pixelmap* source;
    br_pixelmap* palette;
    br_device_clut* clut;

    if (texture == NULL)
        return BR_FALSE;

    BrMemSet(texture, 0, sizeof(*texture));

    if (stored == NULL)
        return BR_FALSE;

    if (video == NULL || !video->active || video->instance == VK_NULL_HANDLE || video->device == VK_NULL_HANDLE
        || video->queue == VK_NULL_HANDLE)
        return BR_FALSE;

    source = stored->source;

    if (source == NULL || source->pixels == NULL || source->width == 0 || source->height == 0
        || (source->flags & BR_PMF_NO_ACCESS)) {
        BufferTextureLogOnce(&sbuffer_logged_source, "colour map has no accessible pixels");
        stored->unavailable = BR_TRUE;
        return BR_FALSE;
    }

    palette = BufferTexturePalette(source);
    clut = BufferTextureSourceClut(stored, source);

    if (!stored->unavailable
        && (stored->texture.image == VK_NULL_HANDLE || stored->built_revision != stored->revision
            || stored->built_source != source || stored->built_palette != palette || stored->built_clut != clut
            || (clut != NULL && stored->built_clut_revision != clut->revision))) {
        if (!BufferTextureLoadFunctions(video->instance))
            return BR_FALSE;

        if (!BufferTextureBuild(stored, video, source, palette, clut)) {
            stored->unavailable = BR_TRUE;
            return BR_FALSE;
        }
    }

    if (stored->texture.image == VK_NULL_HANDLE || stored->texture.view == VK_NULL_HANDLE)
        return BR_FALSE;

    *texture = stored->texture;

    return BR_TRUE;
}

struct br_buffer_stored* BufferStoredVkAllocate(br_renderer* renderer, br_token use, struct br_device_pixelmap* pm,
    br_token_value* tv) {
    struct br_buffer_stored* self;
    const char* ident;

    (void)tv;

    switch (use) {
    case BRT_TEXTURE_O:
    case BRT_COLOUR_MAP_O:
        ident = "Colour-Map";
        break;

    default:
        return NULL;
    }

    if (pm == NULL)
        return NULL;

    self = BrResAllocate(renderer, sizeof(*self), BR_MEMORY_OBJECT);
    if (self == NULL)
        return NULL;

    self->dispatch = &bufferStoredDispatch;
    self->identifier = ident;
    self->device = ObjectDevice(renderer);
    self->renderer = renderer;
    self->source = (br_pixelmap*)pm;
    self->source_flags = pm->pm_flags;

    self->texture.image = VK_NULL_HANDLE;
    self->texture.view = VK_NULL_HANDLE;
    self->texture.width = 0;
    self->texture.height = 0;
    self->texture.generation = 0;
    self->texture_memory = VK_NULL_HANDLE;
    self->texture_device = VK_NULL_HANDLE;
    self->video = NULL;
    self->staging_buffer = VK_NULL_HANDLE;
    self->staging_memory = VK_NULL_HANDLE;
    self->staging_mapped = NULL;
    self->staging_bytes = 0;
    self->revision = 1;
    self->built_revision = 0;
    self->built_source = NULL;
    self->built_palette = NULL;
    self->clut_queried = NULL;
    self->clut_query_done = BR_FALSE;
    self->built_clut = NULL;
    self->built_clut_revision = 0;
    self->unavailable = BR_FALSE;

    ObjectContainerAddFront(renderer, (br_object*)self);

    return self;
}

static br_error BR_CMETHOD_DECL(br_buffer_stored_vk, update)(struct br_buffer_stored* self,
    struct br_device_pixelmap* pm, br_token_value* tv) {
    (void)tv;

    if (pm == NULL)
        return BRE_FAIL;

    self->source = (br_pixelmap*)pm;
    self->source_flags = pm->pm_flags;
    self->revision++;
    self->clut_query_done = BR_FALSE;
    self->unavailable = BR_FALSE;

    return BRE_OK;
}

static void BR_CMETHOD_DECL(br_buffer_stored_vk, free)(br_object* _self) {
    struct br_buffer_stored* self = (struct br_buffer_stored*)_self;

    BufferTextureRelease(self);

    ObjectContainerRemove(self->renderer, (br_object*)self);

    BrResFreeNoCallback(self);
}

static const char* BR_CMETHOD_DECL(br_buffer_stored_vk, identifier)(br_object* self) {
    return ((struct br_buffer_stored*)self)->identifier;
}

static br_token BR_CMETHOD_DECL(br_buffer_stored_vk, type)(br_object* self) {
    (void)self;
    return BRT_BUFFER_STORED;
}

static br_boolean BR_CMETHOD_DECL(br_buffer_stored_vk, isType)(br_object* self, br_token t) {
    (void)self;
    return (t == BRT_BUFFER_STORED) || (t == BRT_OBJECT);
}

static br_device* BR_CMETHOD_DECL(br_buffer_stored_vk, device)(br_object* self) {
    return ((struct br_buffer_stored*)self)->device;
}

static br_size_t BR_CMETHOD_DECL(br_buffer_stored_vk, space)(br_object* self) {
    (void)self;
    return BrResSizeTotal(self);
}

static struct br_tv_template* BR_CMETHOD_DECL(br_buffer_stored_vk, templateQuery)(br_object* _self) {
    struct br_buffer_stored* self = (struct br_buffer_stored*)_self;

    if (self->device->templates.bufferStoredTemplate == NULL) {
        self->device->templates.bufferStoredTemplate = BrTVTemplateAllocate(self->device, bufferStoredTemplateEntries,
            BR_ASIZE(bufferStoredTemplateEntries));
    }

    return self->device->templates.bufferStoredTemplate;
}

static struct br_buffer_stored_dispatch bufferStoredDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD_REF(br_buffer_stored_vk, free),
    ._identifier = BR_CMETHOD_REF(br_buffer_stored_vk, identifier),
    ._type = BR_CMETHOD_REF(br_buffer_stored_vk, type),
    ._isType = BR_CMETHOD_REF(br_buffer_stored_vk, isType),
    ._device = BR_CMETHOD_REF(br_buffer_stored_vk, device),
    ._space = BR_CMETHOD_REF(br_buffer_stored_vk, space),

    ._templateQuery = BR_CMETHOD_REF(br_buffer_stored_vk, templateQuery),
    ._query = BR_CMETHOD_REF(br_object, query),
    ._queryBuffer = BR_CMETHOD_REF(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD_REF(br_object, queryMany),
    ._queryManySize = BR_CMETHOD_REF(br_object, queryManySize),
    ._queryAll = BR_CMETHOD_REF(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD_REF(br_object, queryAllSize),

    ._update = BR_CMETHOD_REF(br_buffer_stored_vk, update),
};
