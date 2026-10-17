#ifndef _DRV_IP_H_
#define _DRV_IP_H_

#ifndef NO_PROTOTYPES

#ifdef __cplusplus
extern "C" {
#endif

br_device* DeviceVkAllocate(const char* identifier, const char* arguments);

br_device_clut* DeviceClutVkAllocate(br_device* dev, const char* identifier);

br_output_facility* OutputFacilityVkInit(br_device* dev, br_renderer_facility* rendfcty);

br_renderer_facility* RendererFacilityVkInit(br_device* dev);

br_renderer* RendererVkAllocate(br_device* device, br_renderer_facility* facility, br_device_pixelmap* dest);

br_renderer_state_stored* RendererStateStoredVkAllocate(br_renderer* renderer, vk_state_stack* base_state, br_uint_32 m,
    br_token_value* tv);

struct br_buffer_stored* BufferStoredVkAllocate(br_renderer* renderer, br_token use, struct br_device_pixelmap* pm,
    br_token_value* tv);

br_geometry_v1_model* GeometryV1ModelVkAllocate(br_renderer_facility* type, const char* id);

br_geometry_stored* GeometryStoredVkAllocate(br_geometry_v1_model* gv1model, const char* id, br_renderer* r,
    struct v11model* model);

br_token VkOnScreenCheck(br_renderer* self, const br_matrix4* model_to_screen, const br_bounds3_f* bounds);

void StateVkInit(vk_state_all* state, void* res);

void StateVkInitMatrix(vk_state_all* state);

void StateVkInitCull(vk_state_all* state);

void StateVkInitClip(vk_state_all* state);

void StateVkInitSurface(vk_state_all* state);

void StateVkInitPrimitive(vk_state_all* state);

void StateVkInitOutput(vk_state_all* state);

void StateVkInitHidden(vk_state_all* state);

void StateVkInitLight(vk_state_all* state);

struct br_tv_template* StateVkGetStateTemplate(vk_state_all* state, br_token part, br_int_32 index);

void StateVkTemplateActions(vk_state_all* state, br_uint_32 mask);

br_boolean StateVkPush(vk_state_all* state, br_uint_32 mask);

br_boolean StateVkPop(vk_state_all* state, br_uint_32 mask);

void StateVkDefault(vk_state_all* state, br_uint_32 mask);

void StateVkCopy(vk_state_stack* dst, vk_state_stack* src, br_uint_32 mask);

br_geometry_lighting* GeometryLightingNullAllocate(br_renderer_facility* type, const char* id);

br_geometry_primitives* GeometryPrimitivesNullAllocate(br_renderer_facility* type, const char* id);

br_device_pixelmap* DevicePixelmapVkAllocateFront(br_device* dev, br_output_facility* outfcty, br_token_value* tv);

br_error VIDEOVkCreate(br_vk_video* video, const br_device_vkrend_callback_procs* callbacks, br_int_32 width,
    br_int_32 height);

void VIDEOVkDestroy(br_vk_video* video);

br_error VIDEOVkResize(br_vk_video* video, br_int_32 width, br_int_32 height);

br_error VIDEOVkPresent(br_vk_video* video, const void* pixels, br_int_32 width, br_int_32 height, br_int_32 row_bytes);

#ifdef __cplusplus
};
#endif

#endif
#endif
