#include "drv.h"

#define DEVICE_TITLE "Vulkan"
#define DEVICE_VERSION BR_VERSION(0, 1, 0)
#define DEVICE_CREATOR "dethrace"
#define DEVICE_PRODUCT "Vulkan"

static const struct br_device_dispatch deviceDispatch;

static const char deviceTitle[] = DEVICE_TITLE;
static const char deviceCreator[] = DEVICE_CREATOR;
static const char deviceProduct[] = DEVICE_PRODUCT;

#define F(f) offsetof(br_device, f)
#define A(a) ((br_uintptr_t)(a))

static struct br_tv_template_entry deviceTemplateEntries[] = {
    { BRT(IDENTIFIER_CSTR), F(identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY, 0 },
    { BRT(VERSION_U32), 0, BRTV_QUERY | BRTV_ALL, BRTV_CONV_DIRECT, DEVICE_VERSION },
    { BRT(BRENDER_VERSION_U32), 0, BRTV_QUERY | BRTV_ALL, BRTV_CONV_DIRECT, __BRENDER__ },
    { BRT(DDI_VERSION_U32), 0, BRTV_QUERY | BRTV_ALL, BRTV_CONV_DIRECT, __BRENDER_DDI__ },
    { BRT(CREATOR_CSTR), A(deviceCreator), BRTV_QUERY | BRTV_ALL | BRTV_ABS, BRTV_CONV_COPY, 0 },
    { BRT(TITLE_CSTR), A(deviceTitle), BRTV_QUERY | BRTV_ALL | BRTV_ABS, BRTV_CONV_COPY, 0 },
    { BRT(PRODUCT_CSTR), A(deviceProduct), BRTV_QUERY | BRTV_ALL | BRTV_ABS, BRTV_CONV_COPY, 0 },
};
#undef F
#undef A

static br_token insignificantMatchTokens[] = {
    BRT_WIDTH_I32,
    BRT_HEIGHT_I32,
    BRT_PIXEL_BITS_I32,
    BRT_PIXEL_TYPE_U8,
    BRT_WINDOW_MONITOR_I32,
    BRT_MSAA_SAMPLES_I32,
    BRT_WINDOW_HANDLE_H,
    BR_NULL_TOKEN,
};

struct token_match {
    br_token_value* original;
    br_token_value* query;
    br_int_32 n;
    void* extra;
    br_size_t extra_size;
    br_token* insignificant;
};

br_device* DeviceVkAllocate(const char* identifier, const char* arguments) {
    br_device* self;

    (void)arguments;

    self = BrResAllocate(NULL, sizeof(*self), BR_MEMORY_OBJECT);
    if (self == NULL)
        return NULL;

    self->res = BrResAllocate(self, 0, BR_MEMORY_DRIVER);
    self->identifier = identifier;
    self->dispatch = &deviceDispatch;
    self->device = self;
    self->object_list = BrObjectListAllocate(self);

    self->output_facility = OutputFacilityVkInit(self);

    if (self->output_facility == NULL) {
        BrResFreeNoCallback(self);
        return NULL;
    }

    return self;
}

static void BR_CMETHOD_DECL(br_device_vk, free)(struct br_object* _self) {
    br_device* self = (br_device*)_self;

    BrObjectContainerFree((br_object_container*)self, BR_NULL_TOKEN, NULL, NULL);
    BrResFreeNoCallback(self);
}

static const char* BR_CMETHOD_DECL(br_device_vk, identifier)(struct br_object* self) {
    return ((br_device*)self)->identifier;
}

static br_token BR_CMETHOD_DECL(br_device_vk, type)(struct br_object* self) {
    (void)self;
    return BRT_DEVICE;
}

static br_boolean BR_CMETHOD_DECL(br_device_vk, isType)(struct br_object* self, br_token t) {
    (void)self;
    return (t == BRT_DEVICE) || (t == BRT_OBJECT_CONTAINER) || (t == BRT_OBJECT);
}

static br_device* BR_CMETHOD_DECL(br_device_vk, device)(struct br_object* self) {
    return ((br_device*)self)->device;
}

static br_size_t BR_CMETHOD_DECL(br_device_vk, space)(struct br_object* self) {
    (void)self;
    return sizeof(br_device);
}

static struct br_tv_template* BR_CMETHOD_DECL(br_device_vk, templateQuery)(struct br_object* _self) {
    br_device* self = (br_device*)_self;

    if (self->templates.deviceTemplate == NULL) {
        self->templates.deviceTemplate = BrTVTemplateAllocate(self, deviceTemplateEntries, BR_ASIZE(deviceTemplateEntries));
    }

    return self->templates.deviceTemplate;
}

static void* BR_CMETHOD_DECL(br_device_vk, listQuery)(struct br_object_container* self) {
    return ((br_device*)self)->object_list;
}

void* BR_CMETHOD_DECL(br_device_vk, tokensMatchBegin)(struct br_device* self, br_token t, br_token_value* tv) {
    struct token_match* tm;
    br_int_32 i;
    br_int_32 n;

    (void)t;

    if (tv == NULL)
        return NULL;

    tm = BrResAllocate(self->res, sizeof(*tm), BR_MEMORY_APPLICATION);
    tm->original = tv;

    for (i = 0; tv[i].t != BR_NULL_TOKEN; i++)
        ;

    tm->n = i + 1;
    tm->query = BrResAllocate(tm, tm->n * sizeof(br_token_value), BR_MEMORY_APPLICATION);
    BrMemCpy(tm->query, tv, tm->n * sizeof(br_token_value));

    n = BR_ASIZE(insignificantMatchTokens);
    tm->insignificant = BrResAllocate(tm, (n + 1) * sizeof(br_token), BR_MEMORY_APPLICATION);
    BrMemCpy(tm->insignificant, insignificantMatchTokens, n * sizeof(br_token));
    tm->insignificant[n - 1] = BrVkrendCallbacksToken();
    tm->insignificant[n] = BR_NULL_TOKEN;

    return (void*)tm;
}

br_boolean BR_CMETHOD_DECL(br_device_vk, tokensMatch)(struct br_object_container* self, br_object* h, void* arg) {
    struct token_match* tm = arg;
    br_size_t s;
    br_int_32 n;

    (void)self;

    if (arg == NULL)
        return BR_TRUE;

    ObjectQueryManySize(h, &s, tm->query);

    if (s > tm->extra_size) {
        if (tm->extra)
            BrResFree(tm->extra);
        tm->extra = BrResAllocate(tm, s, BR_MEMORY_APPLICATION);
        tm->extra_size = s;
    }

    ObjectQueryMany(h, tm->query, tm->extra, tm->extra_size, &n);

    if (tm->query[n].t != BR_NULL_TOKEN)
        return BR_FALSE;

    return BrTokenValueComparePartial(tm->original, tm->query, tm->insignificant);
}

void BR_CMETHOD_DECL(br_device_vk, tokensMatchEnd)(struct br_object_container* self, void* arg) {
    (void)self;

    if (arg)
        BrResFree(arg);
}

static const struct br_device_dispatch deviceDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD_REF(br_device_vk, free),
    ._identifier = BR_CMETHOD_REF(br_device_vk, identifier),
    ._type = BR_CMETHOD_REF(br_device_vk, type),
    ._isType = BR_CMETHOD_REF(br_device_vk, isType),
    ._device = BR_CMETHOD_REF(br_device_vk, device),
    ._space = BR_CMETHOD_REF(br_device_vk, space),

    ._templateQuery = BR_CMETHOD_REF(br_device_vk, templateQuery),
    ._query = BR_CMETHOD_REF(br_object, query),
    ._queryBuffer = BR_CMETHOD_REF(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD_REF(br_object, queryMany),
    ._queryManySize = BR_CMETHOD_REF(br_object, queryManySize),
    ._queryAll = BR_CMETHOD_REF(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD_REF(br_object, queryAllSize),

    ._listQuery = BR_CMETHOD_REF(br_device_vk, listQuery),
    ._tokensMatchBegin = BR_CMETHOD_REF(br_device_vk, tokensMatchBegin),
    ._tokensMatch = BR_CMETHOD_REF(br_device_vk, tokensMatch),
    ._tokensMatchEnd = BR_CMETHOD_REF(br_device_vk, tokensMatchEnd),
    ._addFront = BR_CMETHOD_REF(br_object_container, addFront),
    ._removeFront = BR_CMETHOD_REF(br_object_container, removeFront),
    ._remove = BR_CMETHOD_REF(br_object_container, remove),
    ._find = BR_CMETHOD_REF(br_object_container, find),
    ._findMany = BR_CMETHOD_REF(br_object_container, findMany),
    ._count = BR_CMETHOD_REF(br_object_container, count),
};
