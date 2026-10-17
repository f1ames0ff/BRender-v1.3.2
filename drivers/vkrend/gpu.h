#ifndef _GPU_H_
#define _GPU_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vk_gpu_target vk_gpu_target;
typedef struct vk_gpu_geometry vk_gpu_geometry;

void GpuVkSceneBeginRenderer(struct br_renderer* renderer);

void GpuVkSceneEndRenderer(struct br_renderer* renderer);

void GpuVkRenderStored(struct br_geometry_stored* geometry, struct br_renderer* renderer,
    struct br_renderer_state_stored* default_state);

br_boolean GpuVkRenderV1Model(struct v11model* model, struct br_renderer* renderer,
    struct br_renderer_state_stored* default_state);

void GpuVkGeometryRelease(struct vk_gpu_geometry* geometry);

void GpuVkTargetDestroy(struct vk_gpu_target** target);

#ifdef __cplusplus
};
#endif
#endif
