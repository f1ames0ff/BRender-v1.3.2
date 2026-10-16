#include "drv.h"

#define S BRTV_SET
#define Q BRTV_QUERY
#define A BRTV_ALL

#define AX 0
#define AF BRTV_ALL

#define F(f) offsetof(vk_state_stack, f)

static br_tv_template_entry enable_template_entries[] = {
    { BRT(CULL_B), F(enable.flags), Q | S | A, BRTV_CONV_BIT, VK_ENBL_CULL,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(TRANSFORM_B), F(enable.flags), Q | S | A, BRTV_CONV_BIT, VK_ENBL_TRANSFORM,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(SURFACE_B), F(enable.flags), Q | S | A, BRTV_CONV_BIT, VK_ENBL_SURFACE,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(CLIPPING_B), F(enable.flags), Q | S | A, BRTV_CONV_BIT, VK_ENBL_CLIPPING,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(BOUNDS_B), F(enable.flags), Q | S | A, BRTV_CONV_BIT, VK_ENBL_BOUNDS,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(RENDER_B), F(enable.flags), Q | S | A, BRTV_CONV_BIT, VK_ENBL_RENDER,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
};

static br_tv_template_entry bounds_template_entries[] = {
    { BRT(MIN_V2_X), F(bounds.min), Q | S | AX, BRTV_CONV_V2_FIXED_SCALAR },
    { BRT(MIN_V2_F), F(bounds.min), Q | S | AF, BRTV_CONV_V2_FLOAT_SCALAR },
    { BRT(MAX_V2_X), F(bounds.max), Q | S | AX, BRTV_CONV_V2_FIXED_SCALAR },
    { BRT(MAX_V2_F), F(bounds.max), Q | S | AF, BRTV_CONV_V2_FLOAT_SCALAR },
};

#undef F

static const vk_state_enable enable_default_state = {
    .flags = VK_ENBL_TRANSFORM | VK_ENBL_CLIPPING | VK_ENBL_RENDER,
};

static const vk_state_bounds bounds_default_state = {
    .min = BR_VECTOR2(BR_SCALAR_MAX, BR_SCALAR_MAX),
    .max = BR_VECTOR2(BR_SCALAR_MIN, BR_SCALAR_MIN),
};

static void StateVkInitEnable(vk_state_all* state) {
    state->templates.enable = BrTVTemplateAllocate(state->res, enable_template_entries, BR_ASIZE(enable_template_entries));

    state->default_.enable = enable_default_state;
    state->default_.valid |= VK_MASK_STATE_ENABLE;
}

static void StateVkInitBounds(vk_state_all* state) {
    state->templates.bounds = BrTVTemplateAllocate(state->res, bounds_template_entries, BR_ASIZE(bounds_template_entries));

    state->default_.bounds = bounds_default_state;
    state->default_.valid |= VK_MASK_STATE_BOUNDS;
}

void StateVkInit(vk_state_all* state, void* res) {
    BrMemSet(state, 0, sizeof(*state));

    state->res = res;

    StateVkInitMatrix(state);
    StateVkInitClip(state);
    StateVkInitCull(state);
    StateVkInitSurface(state);
    StateVkInitPrimitive(state);
    StateVkInitOutput(state);
    StateVkInitHidden(state);
    StateVkInitLight(state);
    StateVkInitEnable(state);
    StateVkInitBounds(state);

    state->current = state->stack;
    StateVkCopy(state->current, &state->default_, ~0u);
}

void StateVkCopy(vk_state_stack* dst, vk_state_stack* src, br_uint_32 mask) {
    mask &= src->valid;

    dst->valid |= mask;

    if (mask & VK_MASK_STATE_MATRIX)
        dst->matrix = src->matrix;

    if (mask & VK_MASK_STATE_CLIP) {
        for (int i = 0; i < VK_MAX_STATE_CLIP_PLANES; ++i)
            dst->clip[i] = src->clip[i];
    }

    if (mask & VK_MASK_STATE_CULL)
        dst->cull = src->cull;

    if (mask & VK_MASK_STATE_SURFACE)
        dst->surface = src->surface;

    if (mask & VK_MASK_STATE_PRIMITIVE)
        dst->prim = src->prim;

    if (mask & VK_MASK_STATE_OUTPUT)
        dst->output = src->output;

    if (mask & VK_MASK_STATE_LIGHT) {
        for (int i = 0; i < VK_MAX_STATE_LIGHTS; ++i)
            dst->light[i] = src->light[i];
    }

    if (mask & VK_MASK_STATE_ENABLE)
        dst->enable = src->enable;

    if (mask & VK_MASK_STATE_BOUNDS)
        dst->bounds = src->bounds;
}

br_boolean StateVkPush(vk_state_all* state, br_uint_32 mask) {
    vk_state_stack* old;

    (void)mask;

    if (state->top >= VK_MAX_STATE_STACK)
        return BR_FALSE;

    old = state->current;
    ++state->top;
    ++state->current;

    *state->current = *old;
    return BR_TRUE;
}

br_boolean StateVkPop(vk_state_all* state, br_uint_32 mask) {
    (void)mask;

    if (state->top <= 0)
        return BR_FALSE;

    --state->top;
    --state->current;

    return BR_TRUE;
}

void StateVkDefault(vk_state_all* state, br_uint_32 mask) {
    StateVkCopy(state->current, &state->default_, mask);
}

void StateVkTemplateActions(vk_state_all* state, br_uint_32 mask) {
    (void)state;
    (void)mask;
}

struct br_tv_template* StateVkGetStateTemplate(vk_state_all* state, br_token part, br_int_32 index) {
    switch (part) {
    case BRT_MATRIX:
        return state->templates.matrix;

    case BRT_CLIP:
        if (index >= VK_MAX_STATE_CLIP_PLANES)
            return NULL;

        return state->templates.clip[index];

    case BRT_CULL:
        return state->templates.cull;

    case BRT_SURFACE:
        return state->templates.surface;

    case BRT_PRIMITIVE:
        return state->templates.prim;

    case BRT_HIDDEN_SURFACE:
        return state->templates.hidden;

    case BRT_ENABLE:
        return state->templates.enable;

    case BRT_BOUNDS:
        return state->templates.bounds;

    case BRT_LIGHT:
        if (index >= VK_MAX_STATE_LIGHTS)
            return NULL;

        return state->templates.light[index];

    case BRT_OUTPUT:
        return state->templates.output;

    default:
        break;
    }

    return NULL;
}
