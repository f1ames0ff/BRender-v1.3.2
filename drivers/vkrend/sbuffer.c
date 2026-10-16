#include <stddef.h>

#include "drv.h"

static struct br_buffer_stored_dispatch bufferStoredDispatch;

#define F(f) offsetof(struct br_buffer_stored, f)

static struct br_tv_template_entry bufferStoredTemplateEntries[] = {
    { BRT(IDENTIFIER_CSTR), F(identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
};
#undef F

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

    self = BrResAllocate(renderer, sizeof(*self), BR_MEMORY_OBJECT);
    if (self == NULL)
        return NULL;

    self->dispatch = &bufferStoredDispatch;
    self->identifier = ident;
    self->device = ObjectDevice(renderer);
    self->renderer = renderer;
    self->source = (br_pixelmap*)pm;
    self->source_flags = pm->pm_flags;

    ObjectContainerAddFront(renderer, (br_object*)self);

    return self;
}

static br_error BR_CMETHOD_DECL(br_buffer_stored_vk, update)(struct br_buffer_stored* self,
    struct br_device_pixelmap* pm, br_token_value* tv) {
    (void)tv;

    self->source = (br_pixelmap*)pm;
    self->source_flags = pm->pm_flags;

    return BRE_OK;
}

static void BR_CMETHOD_DECL(br_buffer_stored_vk, free)(br_object* _self) {
    struct br_buffer_stored* self = (struct br_buffer_stored*)_self;

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
