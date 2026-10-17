#include "brassert.h"
#include "drv.h"

static const struct br_geometry_v1_model_dispatch geometryV1ModelDispatch;

#define F(f) offsetof(struct br_geometry_v1_model, f)
#define A(a) ((br_uintptr_t)(a))

static struct br_tv_template_entry geometryV1ModelTemplateEntries[] = {
    { BRT(IDENTIFIER_CSTR), F(identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
    { BRT(RENDERER_FACILITY_O), F(renderer_facility), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
    { BRT(FACILITY_O), F(renderer_facility), BRTV_QUERY, BRTV_CONV_COPY },
    { BRT(STORED_ONLY_B), A(BR_TRUE), BRTV_QUERY | BRTV_ALL | BRTV_ABS, BRTV_CONV_COPY },
};
#undef F
#undef A

br_geometry_v1_model* GeometryV1ModelVkAllocate(br_renderer_facility* type, const char* id) {
    br_geometry_v1_model* self;

    self = BrResAllocate(type, sizeof(*self), BR_MEMORY_OBJECT);
    if (self == NULL)
        return NULL;

    self->dispatch = &geometryV1ModelDispatch;
    self->identifier = id;
    self->device = type->device;
    self->renderer_facility = type;

    ObjectContainerAddFront(type, (br_object*)self);
    return self;
}

static void BR_CMETHOD_DECL(br_geometry_v1_model_vk, free)(br_object* _self) {
    br_geometry_v1_model* self = (br_geometry_v1_model*)_self;

    ObjectContainerRemove(self->renderer_facility, (br_object*)self);

    BrResFreeNoCallback(self);
}

static const char* BR_CMETHOD_DECL(br_geometry_v1_model_vk, identifier)(br_object* self) {
    return ((br_geometry_v1_model*)self)->identifier;
}

static br_token BR_CMETHOD_DECL(br_geometry_v1_model_vk, type)(br_object* self) {
    (void)self;
    return BRT_GEOMETRY_V1_MODEL;
}

static br_boolean BR_CMETHOD_DECL(br_geometry_v1_model_vk, isType)(br_object* self, br_token t) {
    (void)self;
    return (t == BRT_GEOMETRY_V1_MODEL) || (t == BRT_GEOMETRY) || (t == BRT_OBJECT);
}

static br_device* BR_CMETHOD_DECL(br_geometry_v1_model_vk, device)(br_object* self) {
    return ((br_geometry_v1_model*)self)->device;
}

static br_size_t BR_CMETHOD_DECL(br_geometry_v1_model_vk, space)(br_object* self) {
    (void)self;
    return sizeof(br_geometry_v1_model);
}

static struct br_tv_template* BR_CMETHOD_DECL(br_geometry_v1_model_vk, templateQuery)(br_object* _self) {
    br_geometry_v1_model* self = (br_geometry_v1_model*)_self;

    if (self->device->templates.geometryV1ModelTemplate == NULL) {
        self->device->templates.geometryV1ModelTemplate = BrTVTemplateAllocate(
            self->device, (br_tv_template_entry*)geometryV1ModelTemplateEntries, BR_ASIZE(geometryV1ModelTemplateEntries));
    }

    return self->device->templates.geometryV1ModelTemplate;
}

static br_error BR_CMETHOD_DECL(br_geometry_v1_model_vk, storedAvail)(struct br_geometry_v1_model* self,
    br_int_32* psize, br_token_value* tv) {
    (void)self;
    (void)psize;
    (void)tv;
    return BRE_FAIL;
}

static br_error BR_CMETHOD_DECL(br_geometry_v1_model_vk, storedNew)(struct br_geometry_v1_model* self,
    struct br_renderer* renderer, struct br_geometry_stored** psg, struct v11model_f* model, br_token type,
    br_token_value* tv) {
    (void)tv;

    VERIFY(type == BRT_TRIANGLE);

    if ((*psg = GeometryStoredVkAllocate(self, "MODEL", renderer, (struct v11model*)model)) == NULL)
        return BRE_FAIL;

    return BRE_OK;
}

static br_error RenderV1Model(br_geometry_v1_model* self, br_renderer* renderer, struct v11model_f* model,
    struct br_renderer_state_stored* default_state) {
    struct v11model* prepared;
    br_uint_32 triangles;
    br_uint_32 vertices;
    br_uint_16 group;

    (void)self;

    if (renderer == NULL || renderer->gpu_scene == NULL)
        return BRE_FAIL;

    prepared = (struct v11model*)model;

    if (prepared == NULL || prepared->groups == NULL || prepared->ngroups == 0)
        return BRE_FAIL;

    triangles = 0;
    vertices = 0;

    for (group = 0; group < prepared->ngroups; group++) {
        triangles += prepared->groups[group].nfaces;
        vertices += prepared->groups[group].nvertices;
    }

    if (GpuVkRenderV1Model(prepared, renderer, default_state) != BR_TRUE)
        return BRE_FAIL;

    renderer->scene_stats.face_group_count += prepared->ngroups;
    renderer->scene_stats.triangles_drawn_count += triangles;
    renderer->scene_stats.triangles_rendered_count += triangles;
    renderer->scene_stats.vertices_rendered_count += vertices;
    renderer->frame_stats.model_count++;

    return BRE_OK;
}

static br_error BR_CMETHOD_DECL(br_geometry_v1_model_vk, render)(struct br_geometry_v1_model* self,
    struct br_renderer* renderer, struct v11model_f* model, struct br_renderer_state_stored* default_state,
    br_token type) {
    (void)type;

    return RenderV1Model(self, renderer, model, default_state);
}

static const struct br_geometry_v1_model_dispatch geometryV1ModelDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD_REF(br_geometry_v1_model_vk, free),
    ._identifier = BR_CMETHOD_REF(br_geometry_v1_model_vk, identifier),
    ._type = BR_CMETHOD_REF(br_geometry_v1_model_vk, type),
    ._isType = BR_CMETHOD_REF(br_geometry_v1_model_vk, isType),
    ._device = BR_CMETHOD_REF(br_geometry_v1_model_vk, device),
    ._space = BR_CMETHOD_REF(br_geometry_v1_model_vk, space),

    ._templateQuery = BR_CMETHOD_REF(br_geometry_v1_model_vk, templateQuery),
    ._query = BR_CMETHOD_REF(br_object, query),
    ._queryBuffer = BR_CMETHOD_REF(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD_REF(br_object, queryMany),
    ._queryManySize = BR_CMETHOD_REF(br_object, queryManySize),
    ._queryAll = BR_CMETHOD_REF(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD_REF(br_object, queryAllSize),

    ._renderF = BR_CMETHOD_REF(br_geometry_v1_model_vk, render),
    ._renderOnScreenF = BR_CMETHOD_REF(br_geometry_v1_model_vk, render),
    ._storedNewF = BR_CMETHOD_REF(br_geometry_v1_model_vk, storedNew),

    ._storedAvail = BR_CMETHOD_REF(br_geometry_v1_model_vk, storedAvail),
};
