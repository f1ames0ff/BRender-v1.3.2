#include <stddef.h>

#include "drv.h"

static struct br_device_clut_dispatch deviceClutDispatch;

#define F(f) offsetof(struct br_device_clut, f)

static struct br_tv_template_entry deviceClutTemplateEntries[] = {
    { BRT(IDENTIFIER_CSTR), F(identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
};
#undef F

br_device_clut* DeviceClutVkAllocate(br_device* dev, const char* identifier) {
    br_device_clut* self;
    br_uint_32 i;

    self = BrResAllocate(dev->res, sizeof(*self), BR_MEMORY_OBJECT);
    if (self == NULL)
        return NULL;

    self->dispatch = &deviceClutDispatch;
    self->identifier = identifier;
    self->device = dev;

    for (i = 0; i < CLUT_SIZE; i++)
        self->entries[i] = BR_COLOUR_RGB(i, i, i);

    self->revision = 1;

    ObjectContainerAddFront(dev, (br_object*)self);

    return self;
}

static void BR_CMETHOD_DECL(br_device_clut_vk, free)(br_device_clut* self) {
    ObjectContainerRemove(ObjectDevice(self), (br_object*)self);

    BrResFreeNoCallback(self);
}

static br_token BR_CMETHOD_DECL(br_device_clut_vk, type)(br_device_clut* self) {
    (void)self;
    return BRT_DEVICE_CLUT;
}

static br_boolean BR_CMETHOD_DECL(br_device_clut_vk, isType)(br_device_clut* self, br_token t) {
    (void)self;
    return (t == BRT_DEVICE_CLUT) || (t == BRT_OBJECT);
}

static br_size_t BR_CMETHOD_DECL(br_device_clut_vk, space)(br_device_clut* self) {
    (void)self;
    return sizeof(br_device_clut);
}

static const char* BR_CMETHOD_DECL(br_device_clut_vk, identifier)(br_object* self) {
    return ((br_device_clut*)self)->identifier;
}

static br_device* BR_CMETHOD_DECL(br_device_clut_vk, device)(br_object* self) {
    return ((br_device_clut*)self)->device;
}

static struct br_tv_template* BR_CMETHOD_DECL(br_device_clut_vk, templateQuery)(br_object* _self) {
    br_device_clut* self = (br_device_clut*)_self;

    if (self->device->templates.deviceClutTemplate == NULL) {
        self->device->templates.deviceClutTemplate = BrTVTemplateAllocate(self->device, deviceClutTemplateEntries,
            BR_ASIZE(deviceClutTemplateEntries));
    }

    return self->device->templates.deviceClutTemplate;
}

static br_error BR_CMETHOD_DECL(br_device_clut_vk, entrySet)(br_device_clut* self, br_int_32 index, br_colour entry) {
    if (index < 0 || index >= CLUT_SIZE)
        return BRE_OVERFLOW;

    if (self->entries[index] != entry) {
        self->entries[index] = entry;
        self->revision++;
    }

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_clut_vk, entryQuery)(br_device_clut* self, br_colour* entry,
    br_int_32 index) {
    if (index < 0 || index >= CLUT_SIZE)
        return BRE_OVERFLOW;

    *entry = self->entries[index];

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_clut_vk, entrySetMany)(br_device_clut* self, br_int_32 index,
    br_int_32 count, br_colour* entries) {
    br_int_32 i;
    br_boolean changed = BR_FALSE;

    if (index < 0 || index >= CLUT_SIZE)
        return BRE_OVERFLOW;

    if (index + count > CLUT_SIZE)
        return BRE_OVERFLOW;

    for (i = 0; i < count; i++) {
        if (self->entries[index + i] != entries[i]) {
            self->entries[index + i] = entries[i];
            changed = BR_TRUE;
        }
    }

    if (changed)
        self->revision++;

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_device_clut_vk, entryQueryMany)(br_device_clut* self, br_colour* entries,
    br_int_32 index, br_int_32 count) {
    br_int_32 i;

    if (index < 0 || index >= CLUT_SIZE)
        return BRE_OVERFLOW;

    if (index + count > CLUT_SIZE)
        return BRE_OVERFLOW;

    for (i = 0; i < count; i++)
        entries[i] = self->entries[index + i];

    return BRE_OK;
}

static struct br_device_clut_dispatch deviceClutDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD_REF(br_device_clut_vk, free),
    ._identifier = BR_CMETHOD_REF(br_device_clut_vk, identifier),
    ._type = BR_CMETHOD_REF(br_device_clut_vk, type),
    ._isType = BR_CMETHOD_REF(br_device_clut_vk, isType),
    ._device = BR_CMETHOD_REF(br_device_clut_vk, device),
    ._space = BR_CMETHOD_REF(br_device_clut_vk, space),

    ._templateQuery = BR_CMETHOD_REF(br_device_clut_vk, templateQuery),
    ._query = BR_CMETHOD_REF(br_object, query),
    ._queryBuffer = BR_CMETHOD_REF(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD_REF(br_object, queryMany),
    ._queryManySize = BR_CMETHOD_REF(br_object, queryManySize),
    ._queryAll = BR_CMETHOD_REF(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD_REF(br_object, queryAllSize),

    ._entrySet = BR_CMETHOD_REF(br_device_clut_vk, entrySet),
    ._entryQuery = BR_CMETHOD_REF(br_device_clut_vk, entryQuery),
    ._entrySetMany = BR_CMETHOD_REF(br_device_clut_vk, entrySetMany),
    ._entryQueryMany = BR_CMETHOD_REF(br_device_clut_vk, entryQueryMany),
};
