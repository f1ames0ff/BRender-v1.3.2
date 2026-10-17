#include "drv.h"

#include <string.h>

static const struct br_device_pixelmap_dispatch devicePixelmapDispatch;
static const struct br_device_pixelmap_dispatch devicePixelmapFrontDispatch;

#define VKREND_PIXEL_BYTES 2

static br_uint_8 DeviceVkPixelTypeOrBits(br_uint_8 pixel_type, br_int_32 pixel_bits) {
    if (pixel_type != BR_PMT_MAX)
        return pixel_type;

    if (pixel_bits == 16)
        return BR_PMT_RGB_565;

    return BR_PMT_MAX;
}

static br_size_t DevicePixelmapVkShadowSize(br_int_32 width, br_int_32 height) {
    return (br_size_t)width * VKREND_PIXEL_BYTES * height;
}

static br_int_32 DevicePixelmapVkDepthRowBytes(br_device_pixelmap* self) {
    br_int_32 row_bytes = self->pm_row_bytes;

    if (row_bytes < 0)
        row_bytes = -row_bytes;

    return ((row_bytes + VKREND_PIXEL_BYTES - 1) / VKREND_PIXEL_BYTES) * VKREND_PIXEL_BYTES;
}

static br_error DevicePixelmapVkAllocateShadow(br_device_pixelmap* self, br_size_t size) {
    void* shadow;

    shadow = BrResAllocate(self, size, BR_MEMORY_PIXELS);
    if (shadow == NULL)
        return BRE_NO_MEMORY;

    if (self->shadow != NULL)
        BrResFree(self->shadow);

    self->shadow = shadow;
    self->pm_pixels = shadow;

    return BRE_OK;
}

static br_error DevicePixelmapVkResizeShadow(br_device_pixelmap* self, br_int_32 width, br_int_32 height) {
    if (DevicePixelmapVkAllocateShadow(self, DevicePixelmapVkShadowSize(width, height)) != BRE_OK)
        return BRE_NO_MEMORY;

    self->pm_flags = BR_PMF_LINEAR | BR_PMF_ROW_WHOLEPIXELS;

    return BRE_OK;
}

#define F(f) offsetof(struct br_device_pixelmap, f)
static struct br_tv_template_entry devicePixelmapTemplateEntries[] = {
    { BRT(WIDTH_I32), F(pm_width), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U16, 0 },
    { BRT(HEIGHT_I32), F(pm_height), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U16, 0 },
    { BRT(PIXEL_TYPE_U8), F(pm_type), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U8, 0 },
    { BRT(OUTPUT_FACILITY_O), F(output_facility), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(FACILITY_O), F(output_facility), BRTV_QUERY, BRTV_CONV_COPY, 0 },
    { BRT(IDENTIFIER_CSTR), F(pm_identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(CLUT_O), F(clut), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { DEV(VKREND_CALLBACKS_P), 0, BRTV_QUERY | BRTV_ALL, BRTV_CONV_DIRECT, 0 },
};

static struct br_tv_template_entry devicePixelmapFrontTemplateEntries[] = {
    { BRT(WIDTH_I32), F(pm_width), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U16, 0 },
    { BRT(HEIGHT_I32), F(pm_height), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U16, 0 },
    { BRT(PIXEL_TYPE_U8), F(pm_type), BRTV_QUERY | BRTV_ALL, BRTV_CONV_I32_U8, 0 },
    { BRT(OUTPUT_FACILITY_O), F(output_facility), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(FACILITY_O), F(output_facility), BRTV_QUERY, BRTV_CONV_COPY, 0 },
    { BRT(IDENTIFIER_CSTR), F(pm_identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(CLUT_O), F(clut), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { DEV(VKREND_CALLBACKS_P), 0, BRTV_QUERY | BRTV_ALL, BRTV_CONV_DIRECT, 0 },
};
#undef F

struct pixelmapNewTokens {
    br_int_32 width;
    br_int_32 height;
    br_int_32 pixel_bits;
    br_uint_8 pixel_type;
    br_int_32 msaa_samples;
    br_device_vkrend_callback_procs* callbacks;
};

#define F(f) offsetof(struct pixelmapNewTokens, f)
static struct br_tv_template_entry pixelmapNewTemplateEntries[] = {
    { BRT(WIDTH_I32), F(width), BRTV_SET, BRTV_CONV_COPY },
    { BRT(HEIGHT_I32), F(height), BRTV_SET, BRTV_CONV_COPY },
    { BRT(PIXEL_BITS_I32), F(pixel_bits), BRTV_SET, BRTV_CONV_COPY },
    { BRT(PIXEL_TYPE_U8), F(pixel_type), BRTV_SET, BRTV_CONV_COPY },
    { BRT(MSAA_SAMPLES_I32), F(msaa_samples), BRTV_SET, BRTV_CONV_COPY },
    { DEV(VKREND_CALLBACKS_P), F(callbacks), BRTV_SET, BRTV_CONV_COPY },
};
#undef F

struct pixelmapMatchTokens {
    br_int_32 width;
    br_int_32 height;
    br_int_32 pixel_bits;
    br_uint_8 type;
    br_token use_type;
    br_int_32 msaa_samples;
};

#define F(f) offsetof(struct pixelmapMatchTokens, f)
static struct br_tv_template_entry pixelmapMatchTemplateEntries[] = {
    { BRT(WIDTH_I32), F(width), BRTV_SET, BRTV_CONV_COPY },
    { BRT(HEIGHT_I32), F(height), BRTV_SET, BRTV_CONV_COPY },
    { BRT(PIXEL_BITS_I32), F(pixel_bits), BRTV_SET, BRTV_CONV_COPY },
    { BRT(PIXEL_TYPE_U8), F(type), BRTV_SET, BRTV_CONV_COPY },
    { BRT(USE_T), F(use_type), BRTV_SET, BRTV_CONV_COPY },
    { BRT(MSAA_SAMPLES_I32), F(msaa_samples), BRTV_SET, BRTV_CONV_COPY },
};
#undef F

br_device_pixelmap* DevicePixelmapVkAllocateFront(br_device* dev, br_output_facility* outfcty, br_token_value* tv) {
    br_device_pixelmap* self;
    br_int_32 count;
    struct pixelmapNewTokens pt = {
        .width = -1,
        .height = -1,
        .pixel_bits = -1,
        .pixel_type = BR_PMT_MAX,
        .msaa_samples = 0,
        .callbacks = NULL,
    };
    char tmp[80];

    if (dev->templates.pixelmapNewTemplate == NULL) {
        dev->templates.pixelmapNewTemplate = BrTVTemplateAllocate(dev, pixelmapNewTemplateEntries,
            BR_ASIZE(pixelmapNewTemplateEntries));
    }

    if (dev->templates.pixelmapNewTemplate == NULL)
        return NULL;

    BrTokenValueSetMany(&pt, &count, NULL, tv, dev->templates.pixelmapNewTemplate);

    if (pt.callbacks == NULL || pt.width <= 0 || pt.height <= 0)
        return NULL;

    if ((pt.pixel_type = DeviceVkPixelTypeOrBits(pt.pixel_type, pt.pixel_bits)) != BR_PMT_RGB_565) {
        BrFailure("VKREND: unsupported screen pixel type %d", (int)pt.pixel_type);
        return NULL;
    }

    self = BrResAllocate(dev->res, sizeof(br_device_pixelmap), BR_MEMORY_OBJECT);
    if (self == NULL)
        return NULL;

    BrMemSet(self, 0, sizeof(*self));

    BrSprintfN(tmp, sizeof(tmp) - 1, "VkRend:Screen:%dx%d", (int)pt.width, (int)pt.height);
    self->pm_identifier = BrResStrDup(self, tmp);
    self->dispatch = &devicePixelmapFrontDispatch;
    self->device = dev;
    self->output_facility = outfcty;
    self->clut = dev->clut;
    self->use_type = BRT_NONE;
    self->screen = self;
    self->callbacks = *pt.callbacks;

    self->pm_type = pt.pixel_type;
    self->pm_width = pt.width;
    self->pm_height = pt.height;
    self->pm_row_bytes = pt.width * VKREND_PIXEL_BYTES;

    if (DevicePixelmapVkResizeShadow(self, pt.width, pt.height) != BRE_OK) {
        BrResFreeNoCallback(self);
        return NULL;
    }

    if (VIDEOVkCreate(&self->video, &self->callbacks, pt.width, pt.height) != BRE_OK) {
        BrResFreeNoCallback(self);
        return NULL;
    }

    ObjectContainerAddFront(self->output_facility, (br_object*)self);

    return self;
}

static void BR_CMETHOD_DECL(br_device_pixelmap_vk, free)(br_object* _self) {
    br_device_pixelmap* self = (br_device_pixelmap*)_self;

    if (self->sub_pixelmap) {
        GpuVkTargetDestroy(&self->gpu_target);
        BrResFreeNoCallback(self);
        return;
    }

    if (self->output_facility != NULL)
        ObjectContainerRemove(self->output_facility, (br_object*)self);

    GpuVkTargetDestroy(&self->gpu_target);

    VIDEOVkDestroy(&self->video);

    BrResFreeNoCallback(self);
}

static const char* BR_CMETHOD_DECL(br_device_pixelmap_vk, identifier)(br_object* self) {
    return ((br_device_pixelmap*)self)->pm_identifier;
}

static br_token BR_CMETHOD_DECL(br_device_pixelmap_vk, type)(br_object* self) {
    (void)self;
    return BRT_DEVICE_PIXELMAP;
}

static br_boolean BR_CMETHOD_DECL(br_device_pixelmap_vk, isType)(br_object* self, br_token t) {
    (void)self;
    return (t == BRT_DEVICE_PIXELMAP) || (t == BRT_OBJECT);
}

static br_device* BR_CMETHOD_DECL(br_device_pixelmap_vk, device)(br_object* self) {
    return ((br_device_pixelmap*)self)->device;
}

static br_size_t BR_CMETHOD_DECL(br_device_pixelmap_vk, space)(br_object* self) {
    (void)self;
    return sizeof(br_device_pixelmap);
}

static struct br_tv_template* BR_CMETHOD_DECL(br_device_pixelmap_vk, templateQuery)(br_object* _self) {
    br_device_pixelmap* self = (br_device_pixelmap*)_self;

    if (self->use_type == BRT_NONE) {
        if (self->device->templates.devicePixelmapFrontTemplate == NULL) {
            self->device->templates.devicePixelmapFrontTemplate = BrTVTemplateAllocate(self->device,
                devicePixelmapFrontTemplateEntries, BR_ASIZE(devicePixelmapFrontTemplateEntries));
        }

        return self->device->templates.devicePixelmapFrontTemplate;
    }

    if (self->device->templates.devicePixelmapTemplate == NULL) {
        self->device->templates.devicePixelmapTemplate = BrTVTemplateAllocate(self->device, devicePixelmapTemplateEntries,
            BR_ASIZE(devicePixelmapTemplateEntries));
    }

    return self->device->templates.devicePixelmapTemplate;
}

static br_error BR_CMETHOD_DECL(br_device_pixelmap_vk, resize)(br_device_pixelmap* self, br_int_32 width, br_int_32 height) {
    if (width <= 0 || height <= 0)
        return BRE_UNSUPPORTED;

    GpuVkTargetDestroy(&self->gpu_target);

    if (DevicePixelmapVkResizeShadow(self, width, height) != BRE_OK)
        return BRE_FAIL;

    self->pm_width = width;
    self->pm_height = height;
    self->pm_row_bytes = width * VKREND_PIXEL_BYTES;

    if (self->screen == self)
        return VIDEOVkResize(&self->video, width, height);

    return BRE_OK;
}

static br_boolean devicePixelmapVkLoggedOffscreenMatch = BR_FALSE;
static br_boolean devicePixelmapVkLoggedDepthMatch = BR_FALSE;

static br_error BR_CMETHOD_DECL(br_device_pixelmap_vk, match)(br_device_pixelmap* self, br_device_pixelmap** newpm,
    br_token_value* tv) {
    br_int_32 count;
    br_error err;
    br_device_pixelmap* pm;
    br_int_32 row_bytes;
    br_uint_8 type;
    struct pixelmapMatchTokens mt = {
        .width = self->pm_width,
        .height = self->pm_height,
        .pixel_bits = -1,
        .type = BR_PMT_MAX,
        .use_type = BRT_NONE,
        .msaa_samples = 0,
    };
    char tmp[80];

    if (self->device->templates.pixelmapMatchTemplate == NULL) {
        self->device->templates.pixelmapMatchTemplate = BrTVTemplateAllocate(self->device, pixelmapMatchTemplateEntries,
            BR_ASIZE(pixelmapMatchTemplateEntries));
    }

    if (self->device->templates.pixelmapMatchTemplate == NULL)
        return BRE_FAIL;

    err = BrTokenValueSetMany(&mt, &count, NULL, tv, self->device->templates.pixelmapMatchTemplate);
    if (err != BRE_OK)
        return err;

    if (mt.use_type == BRT_NO_RENDER)
        mt.use_type = BRT_OFFSCREEN;

    if (mt.width <= 0 || mt.height <= 0)
        return BRE_UNSUPPORTED;

    if (mt.use_type == BRT_DEPTH) {
        if (mt.pixel_bits != 16 && mt.type != BR_PMT_DEPTH_16)
            return BRE_UNSUPPORTED;

        type = BR_PMT_DEPTH_16;
        row_bytes = DevicePixelmapVkDepthRowBytes(self);
    } else if (mt.use_type == BRT_OFFSCREEN) {
        type = DeviceVkPixelTypeOrBits(mt.type, mt.pixel_bits);

        if (type == BR_PMT_MAX)
            type = self->pm_type;

        if (type != BR_PMT_RGB_565)
            return BRE_UNSUPPORTED;

        row_bytes = mt.width * VKREND_PIXEL_BYTES;
    } else {
        return BRE_UNSUPPORTED;
    }

    pm = BrResAllocate(self->device, sizeof(br_device_pixelmap), BR_MEMORY_OBJECT);
    if (pm == NULL)
        return BRE_NO_MEMORY;

    BrMemSet(pm, 0, sizeof(*pm));

    BrSprintfN(tmp, sizeof(tmp) - 1, "VkRend:%s:%dx%d", mt.use_type == BRT_DEPTH ? "Depth" : "Backbuffer",
        (int)mt.width, (int)mt.height);
    pm->pm_identifier = BrResStrDup(pm, tmp);
    pm->dispatch = &devicePixelmapDispatch;
    pm->device = self->device;
    pm->output_facility = self->output_facility;
    pm->clut = self->device->clut;
    pm->use_type = mt.use_type;
    pm->screen = self->screen;

    pm->pm_type = type;
    pm->pm_width = mt.width;
    pm->pm_height = mt.height;
    pm->pm_row_bytes = row_bytes;
    pm->pm_origin_x = self->pm_origin_x;
    pm->pm_origin_y = self->pm_origin_y;

    if (DevicePixelmapVkAllocateShadow(pm, (br_size_t)row_bytes * mt.height) != BRE_OK) {
        BrResFreeNoCallback(pm);
        return BRE_FAIL;
    }

    if (pm->pm_width * VKREND_PIXEL_BYTES == pm->pm_row_bytes)
        pm->pm_flags = BR_PMF_LINEAR | BR_PMF_ROW_WHOLEPIXELS;
    else
        pm->pm_flags = BR_PMF_ROW_WHOLEPIXELS;

    *newpm = pm;
    ObjectContainerAddFront(self->output_facility, (br_object*)pm);

    if (mt.use_type == BRT_DEPTH) {
        if (!devicePixelmapVkLoggedDepthMatch) {
            devicePixelmapVkLoggedDepthMatch = BR_TRUE;
            BrLogPrintf("VKREND: device depth match\n");
        }
    } else if (!devicePixelmapVkLoggedOffscreenMatch) {
        devicePixelmapVkLoggedOffscreenMatch = BR_TRUE;
        BrLogPrintf("VKREND: device offscreen match\n");
    }

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_pixelmap_vk, allocateSub)(br_device_pixelmap* self, br_device_pixelmap** newpm,
    br_rectangle* rect) {
    br_device_pixelmap* pm;
    br_rectangle out;

    pm = BrResAllocate(self->device, sizeof(*pm), BR_MEMORY_PIXELMAP);
    if (pm == NULL)
        return BRE_NO_MEMORY;

    *pm = *self;

    if (PixelmapRectangleClip(&out, rect, (br_pixelmap*)self) == BR_CLIP_REJECT) {
        BrResFreeNoCallback(pm);
        return BRE_FAIL;
    }

    pm->sub_pixelmap = BR_TRUE;
    pm->parent_height = self->pm_height;
    pm->gpu_target = NULL;

    if (out.w != self->pm_width)
        pm->pm_flags &= ~BR_PMF_LINEAR;

    pm->pm_base_x += out.x;
    pm->pm_base_y += out.y;
    pm->pm_width = out.w;
    pm->pm_height = out.h;
    pm->pm_origin_x = 0;
    pm->pm_origin_y = 0;

    *newpm = pm;

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_pixelmap_vkf, doubleBuffer)(br_device_pixelmap* self, br_device_pixelmap* src) {
    br_device_pixelmap* screen = self->screen;

    if (src == NULL || self == src)
        return BRE_OK;

    if (self->use_type != BRT_NONE)
        return BRE_UNSUPPORTED;

    if (screen == NULL)
        return BRE_FAIL;

    if (src->pm_type != BR_PMT_RGB_565 || src->pm_pixels == NULL || src->pm_width <= 0 || src->pm_height <= 0)
        return BRE_UNSUPPORTED;

    return VIDEOVkPresent(&screen->video, src->pm_pixels, src->pm_width, src->pm_height, src->pm_row_bytes);
}

static br_error BR_CMETHOD_DECL(br_device_pixelmap_vk, flush)(br_device_pixelmap* self) {
    (void)self;
    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_pixelmap_vk, directLock)(br_device_pixelmap* self, br_boolean block) {
    (void)block;

    if (self->shadow == NULL)
        return BRE_FAIL;

    self->pm_pixels = self->shadow;

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_pixelmap_vk, directUnlock)(br_device_pixelmap* self) {
    if (self->shadow == NULL)
        return BRE_FAIL;

    self->pm_pixels = self->shadow;

    return BRE_OK;
}

static const struct br_device_pixelmap_dispatch devicePixelmapDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD_REF(br_device_pixelmap_vk, free),
    ._identifier = BR_CMETHOD_REF(br_device_pixelmap_vk, identifier),
    ._type = BR_CMETHOD_REF(br_device_pixelmap_vk, type),
    ._isType = BR_CMETHOD_REF(br_device_pixelmap_vk, isType),
    ._device = BR_CMETHOD_REF(br_device_pixelmap_vk, device),
    ._space = BR_CMETHOD_REF(br_device_pixelmap_vk, space),

    ._templateQuery = BR_CMETHOD_REF(br_device_pixelmap_vk, templateQuery),
    ._query = BR_CMETHOD_REF(br_object, query),
    ._queryBuffer = BR_CMETHOD_REF(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD_REF(br_object, queryMany),
    ._queryManySize = BR_CMETHOD_REF(br_object, queryManySize),
    ._queryAll = BR_CMETHOD_REF(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD_REF(br_object, queryAllSize),

    ._validSource = BR_CMETHOD_REF(br_device_pixelmap_mem, validSource),
    ._resize = BR_CMETHOD_REF(br_device_pixelmap_vk, resize),
    ._match = BR_CMETHOD_REF(br_device_pixelmap_vk, match),
    ._allocateSub = BR_CMETHOD_REF(br_device_pixelmap_vk, allocateSub),

    ._copy = BR_CMETHOD_REF(br_device_pixelmap_gen, copy),
    ._copyTo = BR_CMETHOD_REF(br_device_pixelmap_mem, copyTo),
    ._copyFrom = BR_CMETHOD_REF(br_device_pixelmap_mem, copyFrom),
    ._fill = BR_CMETHOD_REF(br_device_pixelmap_mem, fill),
    ._doubleBuffer = BR_CMETHOD_REF(br_device_pixelmap_fail, doubleBuffer),

    ._copyDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, copyDirty),
    ._copyToDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, copyToDirty),
    ._copyFromDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, copyFromDirty),
    ._fillDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, fillDirty),
    ._doubleBufferDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, doubleBufferDirty),

    ._rectangle = BR_CMETHOD_REF(br_device_pixelmap_gen, rectangle),
    ._rectangle2 = BR_CMETHOD_REF(br_device_pixelmap_gen, rectangle2),
    ._rectangleCopy = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleCopyTo),
    ._rectangleCopyTo = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleCopyTo),
    ._rectangleCopyFrom = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleCopyFrom),
    ._rectangleStretchCopy = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleStretchCopyTo),
    ._rectangleStretchCopyTo = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleStretchCopyTo),
    ._rectangleStretchCopyFrom = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleStretchCopyFrom),
    ._rectangleFill = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleFill),
    ._pixelSet = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelSet),
    ._line = BR_CMETHOD_REF(br_device_pixelmap_mem, line),
    ._copyBits = BR_CMETHOD_REF(br_device_pixelmap_mem, copyBits),

    ._text = BR_CMETHOD_REF(br_device_pixelmap_gen, text),
    ._textBounds = BR_CMETHOD_REF(br_device_pixelmap_gen, textBounds),

    ._rowSize = BR_CMETHOD_REF(br_device_pixelmap_mem, rowSize),
    ._rowQuery = BR_CMETHOD_REF(br_device_pixelmap_mem, rowQuery),
    ._rowSet = BR_CMETHOD_REF(br_device_pixelmap_mem, rowSet),

    ._pixelQuery = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelQuery),
    ._pixelAddressQuery = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelAddressQuery),

    ._pixelAddressSet = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelAddressSet),
    ._originSet = BR_CMETHOD_REF(br_device_pixelmap_mem, originSet),

    ._flush = BR_CMETHOD_REF(br_device_pixelmap_vk, flush),
    ._synchronise = BR_CMETHOD_REF(br_device_pixelmap_mem, synchronise),
    ._directLock = BR_CMETHOD_REF(br_device_pixelmap_vk, directLock),
    ._directUnlock = BR_CMETHOD_REF(br_device_pixelmap_vk, directUnlock),
    ._getControls = BR_CMETHOD_REF(br_device_pixelmap_fail, getControls),
    ._setControls = BR_CMETHOD_REF(br_device_pixelmap_fail, setControls),
};

static const struct br_device_pixelmap_dispatch devicePixelmapFrontDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD_REF(br_device_pixelmap_vk, free),
    ._identifier = BR_CMETHOD_REF(br_device_pixelmap_vk, identifier),
    ._type = BR_CMETHOD_REF(br_device_pixelmap_vk, type),
    ._isType = BR_CMETHOD_REF(br_device_pixelmap_vk, isType),
    ._device = BR_CMETHOD_REF(br_device_pixelmap_vk, device),
    ._space = BR_CMETHOD_REF(br_device_pixelmap_vk, space),

    ._templateQuery = BR_CMETHOD_REF(br_device_pixelmap_vk, templateQuery),
    ._query = BR_CMETHOD_REF(br_object, query),
    ._queryBuffer = BR_CMETHOD_REF(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD_REF(br_object, queryMany),
    ._queryManySize = BR_CMETHOD_REF(br_object, queryManySize),
    ._queryAll = BR_CMETHOD_REF(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD_REF(br_object, queryAllSize),

    ._validSource = BR_CMETHOD_REF(br_device_pixelmap_mem, validSource),
    ._resize = BR_CMETHOD_REF(br_device_pixelmap_vk, resize),
    ._match = BR_CMETHOD_REF(br_device_pixelmap_vk, match),
    ._allocateSub = BR_CMETHOD_REF(br_device_pixelmap_vk, allocateSub),

    ._copy = BR_CMETHOD_REF(br_device_pixelmap_gen, copy),
    ._copyTo = BR_CMETHOD_REF(br_device_pixelmap_mem, copyTo),
    ._copyFrom = BR_CMETHOD_REF(br_device_pixelmap_mem, copyFrom),
    ._fill = BR_CMETHOD_REF(br_device_pixelmap_mem, fill),
    ._doubleBuffer = BR_CMETHOD_REF(br_device_pixelmap_vkf, doubleBuffer),

    ._copyDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, copyDirty),
    ._copyToDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, copyToDirty),
    ._copyFromDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, copyFromDirty),
    ._fillDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, fillDirty),
    ._doubleBufferDirty = BR_CMETHOD_REF(br_device_pixelmap_gen, doubleBufferDirty),

    ._rectangle = BR_CMETHOD_REF(br_device_pixelmap_gen, rectangle),
    ._rectangle2 = BR_CMETHOD_REF(br_device_pixelmap_gen, rectangle2),
    ._rectangleCopy = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleCopyTo),
    ._rectangleCopyTo = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleCopyTo),
    ._rectangleCopyFrom = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleCopyFrom),
    ._rectangleStretchCopy = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleStretchCopyTo),
    ._rectangleStretchCopyTo = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleStretchCopyTo),
    ._rectangleStretchCopyFrom = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleStretchCopyFrom),
    ._rectangleFill = BR_CMETHOD_REF(br_device_pixelmap_mem, rectangleFill),
    ._pixelSet = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelSet),
    ._line = BR_CMETHOD_REF(br_device_pixelmap_mem, line),
    ._copyBits = BR_CMETHOD_REF(br_device_pixelmap_mem, copyBits),

    ._text = BR_CMETHOD_REF(br_device_pixelmap_gen, text),
    ._textBounds = BR_CMETHOD_REF(br_device_pixelmap_gen, textBounds),

    ._rowSize = BR_CMETHOD_REF(br_device_pixelmap_mem, rowSize),
    ._rowQuery = BR_CMETHOD_REF(br_device_pixelmap_mem, rowQuery),
    ._rowSet = BR_CMETHOD_REF(br_device_pixelmap_mem, rowSet),

    ._pixelQuery = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelQuery),
    ._pixelAddressQuery = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelAddressQuery),

    ._pixelAddressSet = BR_CMETHOD_REF(br_device_pixelmap_mem, pixelAddressSet),
    ._originSet = BR_CMETHOD_REF(br_device_pixelmap_mem, originSet),

    ._flush = BR_CMETHOD_REF(br_device_pixelmap_vk, flush),
    ._synchronise = BR_CMETHOD_REF(br_device_pixelmap_mem, synchronise),
    ._directLock = BR_CMETHOD_REF(br_device_pixelmap_vk, directLock),
    ._directUnlock = BR_CMETHOD_REF(br_device_pixelmap_vk, directUnlock),
    ._getControls = BR_CMETHOD_REF(br_device_pixelmap_fail, getControls),
    ._setControls = BR_CMETHOD_REF(br_device_pixelmap_fail, setControls),
};
