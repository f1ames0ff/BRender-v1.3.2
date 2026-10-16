#include "drv.h"

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
