#ifndef VK_STATE_H_
#define VK_STATE_H_

#include "drv.h"

#define VK_MAX_STATE_STACK 64
#define VK_MAX_STATE_LIGHTS BR_MAX_LIGHTS
#define VK_MAX_STATE_CLIP_PLANES BR_MAX_CLIP_PLANES

enum {
    VK_MASK_STATE_SURFACE = BR_STATE_SURFACE,
    VK_MASK_STATE_MATRIX = BR_STATE_MATRIX,
    VK_MASK_STATE_ENABLE = BR_STATE_ENABLE,
    VK_MASK_STATE_LIGHT = BR_STATE_LIGHT,
    VK_MASK_STATE_CLIP = BR_STATE_CLIP,
    VK_MASK_STATE_BOUNDS = BR_STATE_BOUNDS,
    VK_MASK_STATE_CULL = BR_STATE_CULL,
    VK_MASK_STATE_OUTPUT = BR_STATE_OUTPUT,
    VK_MASK_STATE_PRIMITIVE = BR_STATE_PRIMITIVE,
};

#define VK_TM_PART 0x00001
#define VK_TM_INDEX 0x00002
#define VK_TM_V2S_HINT 0x00004
#define VK_TM_M2V 0x00008
#define VK_TM_V2S 0x00010
#define VK_TM_COPY_CACHE 0x00020
#define VK_TM_CACHE 0x00040

#define VK_TM_CLEAR_M2V_HINT 0x00080
#define VK_TM_CLEAR_V2S_HINT 0x00100

#define VK_TM_INVALID_PS 0x01000
#define VK_TM_INVALID_PM 0x02000
#define VK_TM_INVALID_V2M 0x04000
#define VK_TM_INVALID_M2S 0x08000
#define VK_TM_INVALID_CC 0x10000

typedef struct vk_state_clip {
    br_token type;
    br_vector4 plane;
} vk_state_clip;

typedef struct vk_state_matrix {
    br_matrix34 model_to_view;
    br_matrix4 view_to_screen;
    br_matrix34 view_to_environment;

    br_token model_to_view_hint;
    br_token view_to_screen_hint;
    br_token view_to_environment_hint;

    br_scalar hither_z;
    br_scalar yon_z;
} vk_state_matrix;

typedef struct vk_state_cull {
    br_token type;
    br_token space;
} vk_state_cull;

typedef struct vk_state_surface {
    br_colour colour;

    br_scalar opacity;
    br_scalar ka;
    br_scalar kd;
    br_scalar ks;
    br_scalar power;

    br_boolean lighting;
    br_boolean prelighting;
    br_boolean force_front;
    br_boolean force_back;
    br_scalar depth_bias;

    br_token colour_source;
    br_token mapping_source;

    br_matrix23 map_transform;
} vk_state_surface;

typedef struct vk_state_light {
    br_token type;
    br_token lighting_space;

    br_vector3_f position;
    br_vector3_f direction;

    br_colour colour;

    br_scalar spot_outer;
    br_scalar spot_inner;

    br_scalar attenuation_l;
    br_scalar attenuation_c;
    br_scalar attenuation_q;
} vk_state_light;

typedef void vk_insert_cbfn(br_primitive* primitive, void* arg1, void* arg2, void* arg3, br_order_table* order_table,
    br_scalar* z);

typedef struct vk_state_hidden {
    br_token type;
    br_token divert;
    br_order_table* order_table;
    br_primitive_heap* heap;
    vk_insert_cbfn* insert_fn;
    void* insert_arg1;
    void* insert_arg2;
    void* insert_arg3;
} vk_state_hidden;

enum {
    VK_PRIMF_DEPTH_WRITE_BIT,
    VK_PRIMF_COLOUR_WRITE_BIT,

    VK_PRIMF_BLEND_BIT,
    VK_PRIMF_MODULATE_BIT,
    VK_PRIMF_COLOUR_KEY_BIT,
    VK_PRIMF_SMOOTH_BIT,
};

enum {
    VK_PRIMF_DEPTH_WRITE = (1 << VK_PRIMF_DEPTH_WRITE_BIT),
    VK_PRIMF_COLOUR_WRITE = (1 << VK_PRIMF_COLOUR_WRITE_BIT),
    VK_PRIMF_BLEND = (1 << VK_PRIMF_BLEND_BIT),
    VK_PRIMF_MODULATE = (1 << VK_PRIMF_MODULATE_BIT),
    VK_PRIMF_COLOUR_KEY = (1 << VK_PRIMF_COLOUR_KEY_BIT),
    VK_PRIMF_SMOOTH = (1 << VK_PRIMF_SMOOTH_BIT),
};

struct br_buffer_stored;

typedef struct vk_state_primitive {
    br_uint_32 flags;

    br_int_32 index_base;
    br_int_32 index_range;

    br_token colour_type;

    br_token depth_test;

    br_token blend_mode;
    float alpha_val;

    br_token perspective_type;
    br_int_32 subdivide_tolerance;

    struct br_buffer_stored* colour_map;
    struct br_buffer_stored* index_shade;
    struct br_buffer_stored* index_blend;
    struct br_buffer_stored* index_fog;
    struct br_buffer_stored* screendoor;
    struct br_buffer_stored* lighting;
    struct br_buffer_stored* bump;

    br_token filter;

    br_token mip_filter;

    br_boolean fog_enabled;
    br_colour fog_colour;
    br_scalar fog_min;
    br_scalar fog_max;

} vk_state_primitive;

typedef struct vk_state_output {
    struct br_device_pixelmap* colour;
    struct br_device_pixelmap* depth;
} vk_state_output;

typedef struct vk_state_stack {
    vk_state_matrix matrix;

    vk_state_clip clip[VK_MAX_STATE_CLIP_PLANES];
    vk_state_cull cull;
    vk_state_surface surface;
    vk_state_primitive prim;
    vk_state_output output;
    vk_state_light light[VK_MAX_STATE_LIGHTS];
    vk_state_hidden hidden;

    br_uint_32 valid;
} vk_state_stack;

typedef struct vk_state_all {
    vk_state_stack default_;
    vk_state_stack* current;
    vk_state_stack stack[VK_MAX_STATE_STACK];
    int top;

    struct {
        struct br_tv_template* clip[VK_MAX_STATE_CLIP_PLANES];
        struct br_tv_template* matrix;
        struct br_tv_template* cull;
        struct br_tv_template* surface;
        struct br_tv_template* prim;
        struct br_tv_template* output;
        struct br_tv_template* light[VK_MAX_STATE_LIGHTS];
        struct br_tv_template* hidden;
    } templates;

    void* res;

} vk_state_all;

#endif
