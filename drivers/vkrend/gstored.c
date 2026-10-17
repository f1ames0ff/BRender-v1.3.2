#include "drv.h"

static const struct br_geometry_stored_dispatch geometryStoredDispatch;

#define F(f) offsetof(struct br_geometry_stored, f)

static struct br_tv_template_entry geometryStoredTemplateEntries[] = {
    { BRT(IDENTIFIER_CSTR), F(identifier), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
    { BRT(GEOMETRY_V1_MODEL), F(gv1model), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
    { BRT(SHARED_B), F(shared), BRTV_QUERY | BRTV_ALL, BRTV_CONV_COPY },
};
#undef F

static VkrendDrawRecord drawRecords[VKREND_DRAW_RECORD_CAPACITY];
static br_uint_32 drawRecordSequence;
static br_uint_32 drawTriangleTotal;
static br_boolean drawRecordsAnnounced;

static br_uint_32 GeometryStoredVkRetainedRecords(void) {
    if (drawRecordSequence < VKREND_DRAW_RECORD_CAPACITY)
        return drawRecordSequence;

    return VKREND_DRAW_RECORD_CAPACITY;
}

VkrendDrawRecord* GeometryStoredVkRecords(br_uint_32* count) {
    if (count != NULL)
        *count = GeometryStoredVkRetainedRecords();

    return drawRecords;
}

VkrendDrawRecord* GeometryStoredVkRecordGet(br_uint_32 index) {
    br_uint_32 retained = GeometryStoredVkRetainedRecords();

    if (index >= retained)
        return NULL;

    return &drawRecords[(drawRecordSequence - retained + index) % VKREND_DRAW_RECORD_CAPACITY];
}

br_uint_32 GeometryStoredVkDrawCount(void) {
    return drawRecordSequence;
}

br_uint_32 GeometryStoredVkTriangleCount(void) {
    return drawTriangleTotal;
}

static void GeometryStoredVkCaptureDraw(br_geometry_stored* self, br_renderer* renderer) {
    VkrendDrawRecord* record;
    br_uint_32 sequence;

    sequence = ++drawRecordSequence;
    record = &drawRecords[(sequence - 1) % VKREND_DRAW_RECORD_CAPACITY];

    record->sequence = sequence;
    record->geometry = self;
    record->model = self->model;
    record->renderer = renderer;
    record->state = renderer->state.current;
    record->group_index = 0;
    record->group_count = self->ngroups;
    record->vertices = self->total_vertices;
    record->faces = self->total_faces;
    record->triangles = self->total_faces;

    drawTriangleTotal += self->total_faces;

    if (drawRecordsAnnounced == BR_FALSE) {
        drawRecordsAnnounced = BR_TRUE;
        BrLogPrintf("VKREND: draw records active\n");
    }
}

br_geometry_stored* GeometryStoredVkAllocate(br_geometry_v1_model* gv1model, const char* id, br_renderer* r,
    struct v11model* model) {
    br_geometry_stored* self;

    (void)r;

    self = BrResAllocate(gv1model->renderer_facility->object_list, sizeof(*self), BR_MEMORY_OBJECT);
    if (self == NULL)
        return NULL;

    self->dispatch = &geometryStoredDispatch;
    self->identifier = id;
    self->device = gv1model->device;
    self->gv1model = gv1model;
    self->shared = BR_TRUE;
    self->model = model;
    self->gpu = NULL;

    self->ngroups = model->ngroups;
    self->total_faces = 0;
    self->total_vertices = 0;

    for (br_uint_16 i = 0; i < model->ngroups; ++i) {
        self->total_faces += model->groups[i].nfaces;
        self->total_vertices += model->groups[i].nvertices;
    }

    ObjectContainerAddFront(gv1model->renderer_facility, (br_object*)self);
    return (br_geometry_stored*)self;
}

static void BR_CMETHOD_DECL(br_geometry_stored_vk, free)(br_object* _self) {
    br_geometry_stored* self = (br_geometry_stored*)_self;

    ObjectContainerRemove(self->gv1model->renderer_facility, (br_object*)self);

    GpuVkGeometryRelease(self->gpu);
    self->gpu = NULL;

    BrResFreeNoCallback(self);
}

static char* BR_CMETHOD_DECL(br_geometry_stored_vk, identifier)(br_object* self) {
    return (char*)((br_geometry_stored*)self)->identifier;
}

static br_token BR_CMETHOD_DECL(br_geometry_stored_vk, type)(br_object* self) {
    (void)self;
    return BRT_GEOMETRY_STORED;
}

static br_boolean BR_CMETHOD_DECL(br_geometry_stored_vk, isType)(br_object* self, br_token t) {
    (void)self;
    return (t == BRT_GEOMETRY_STORED) || (t == BRT_GEOMETRY) || (t == BRT_OBJECT);
}

static br_device* BR_CMETHOD_DECL(br_geometry_stored_vk, device)(br_object* self) {
    return ((br_geometry_stored*)self)->device;
}

static br_int_32 BR_CMETHOD_DECL(br_geometry_stored_vk, space)(br_object* self) {
    (void)self;
    return sizeof(br_geometry_stored);
}

static struct br_tv_template* BR_CMETHOD_DECL(br_geometry_stored_vk, templateQuery)(br_object* _self) {
    br_geometry_stored* self = (br_geometry_stored*)_self;

    if (self->device->templates.geometryStoredTemplate == NULL) {
        self->device->templates.geometryStoredTemplate =
            BrTVTemplateAllocate(self->device, geometryStoredTemplateEntries, BR_ASIZE(geometryStoredTemplateEntries));
    }

    return self->device->templates.geometryStoredTemplate;
}

static br_error RenderStored(br_geometry_stored* self, br_renderer* renderer,
    struct br_renderer_state_stored* default_state) {
    GeometryStoredVkCaptureDraw(self, renderer);

    renderer->scene_stats.face_group_count += self->ngroups;
    renderer->scene_stats.triangles_drawn_count += self->total_faces;
    renderer->scene_stats.triangles_rendered_count += self->total_faces;
    renderer->scene_stats.vertices_rendered_count += self->total_vertices;
    renderer->frame_stats.model_count++;

    GpuVkRenderStored(self, renderer, default_state);

    return BRE_OK;
}

static br_error BR_CMETHOD(br_geometry_stored_vk, render)(br_geometry_stored* self, br_renderer* renderer,
    struct br_renderer_state_stored* default_state) {
    return RenderStored(self, renderer, default_state);
}

static br_error BR_CMETHOD(br_geometry_stored_vk, renderOnScreen)(br_geometry_stored* self, br_renderer* renderer,
    struct br_renderer_state_stored* default_state) {
    return RenderStored(self, renderer, default_state);
}

static const struct br_geometry_stored_dispatch geometryStoredDispatch = {
    .__reserved0 = NULL,
    .__reserved1 = NULL,
    .__reserved2 = NULL,
    .__reserved3 = NULL,
    ._free = BR_CMETHOD(br_geometry_stored_vk, free),
    ._identifier = BR_CMETHOD(br_geometry_stored_vk, identifier),
    ._type = BR_CMETHOD(br_geometry_stored_vk, type),
    ._isType = BR_CMETHOD(br_geometry_stored_vk, isType),
    ._device = BR_CMETHOD(br_geometry_stored_vk, device),
    ._space = BR_CMETHOD(br_geometry_stored_vk, space),

    ._templateQuery = BR_CMETHOD(br_geometry_stored_vk, templateQuery),
    ._query = BR_CMETHOD(br_object, query),
    ._queryBuffer = BR_CMETHOD(br_object, queryBuffer),
    ._queryMany = BR_CMETHOD(br_object, queryMany),
    ._queryManySize = BR_CMETHOD(br_object, queryManySize),
    ._queryAll = BR_CMETHOD(br_object, queryAll),
    ._queryAllSize = BR_CMETHOD(br_object, queryAllSize),

    ._render = BR_CMETHOD(br_geometry_stored_vk, render),
    ._renderOnScreen = BR_CMETHOD(br_geometry_stored_vk, renderOnScreen),
};
