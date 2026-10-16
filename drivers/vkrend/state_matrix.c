#include "drv.h"

#define S BRTV_SET
#define Q BRTV_QUERY
#define A BRTV_ALL

#define AX 0
#define AF BRTV_ALL

#define F(f) offsetof(vk_state_stack, f)

static br_tv_template_entry template_entries[] = {
    { BRT(MODEL_TO_VIEW_M34_X), F(matrix.model_to_view), Q | S | AX, BRTV_CONV_M34_FIXED_SCALAR, 0,
        VK_TM_PART | VK_TM_M2V | VK_TM_CLEAR_M2V_HINT | VK_TM_INVALID_PM | VK_TM_INVALID_V2M | VK_TM_INVALID_M2S },
    { BRT(MODEL_TO_VIEW_M34_F), F(matrix.model_to_view), Q | S | AF, BRTV_CONV_M34_FLOAT_SCALAR, 0,
        VK_TM_PART | VK_TM_M2V | VK_TM_CLEAR_M2V_HINT | VK_TM_INVALID_PM | VK_TM_INVALID_V2M | VK_TM_INVALID_M2S },
    { BRT(VIEW_TO_ENVIRONMENT_M34_X), F(matrix.view_to_environment), Q | S | AX, BRTV_CONV_M34_FIXED_SCALAR, 0,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(VIEW_TO_ENVIRONMENT_M34_F), F(matrix.view_to_environment), Q | S | AF, BRTV_CONV_M34_FLOAT_SCALAR, 0,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(VIEW_TO_SCREEN_M4_X), F(matrix.view_to_screen), Q | S | AX, BRTV_CONV_M4_FIXED_SCALAR, 0,
        VK_TM_PART | VK_TM_V2S | VK_TM_CLEAR_V2S_HINT | VK_TM_V2S_HINT | VK_TM_INVALID_PS | VK_TM_INVALID_PM |
            VK_TM_INVALID_M2S },
    { BRT(VIEW_TO_SCREEN_M4_F), F(matrix.view_to_screen), Q | S | AF, BRTV_CONV_M4_FLOAT_SCALAR, 0,
        VK_TM_PART | VK_TM_V2S | VK_TM_CLEAR_V2S_HINT | VK_TM_V2S_HINT | VK_TM_INVALID_PS | VK_TM_INVALID_PM |
            VK_TM_INVALID_M2S },
    { BRT(MODEL_TO_VIEW_HINT_T), F(matrix.model_to_view_hint), Q | S | A, BRTV_CONV_COPY, 0,
        VK_TM_PART | VK_TM_INVALID_PM | VK_TM_INVALID_V2M },
    { BRT(VIEW_TO_SCREEN_HINT_T), F(matrix.view_to_screen_hint), Q | S | A, BRTV_CONV_COPY, 0,
        VK_TM_PART | VK_TM_V2S_HINT | VK_TM_INVALID_PS | VK_TM_INVALID_PM | VK_TM_INVALID_M2S },
    { BRT(VIEW_TO_ENVIRONMENT_HINT_T), F(matrix.view_to_environment_hint), Q | S | A, BRTV_CONV_COPY, 0,
        VK_TM_PART | VK_TM_INVALID_PS | VK_TM_INVALID_PM },
    { BRT(HITHER_Z_X), F(matrix.hither_z), Q | S | A, BRTV_CONV_FIXED_SCALAR, 0, VK_TM_PART },
    { BRT(HITHER_Z_F), F(matrix.hither_z), Q | S | A, BRTV_CONV_FLOAT_SCALAR, 0, VK_TM_PART },
    { BRT(YON_Z_X), F(matrix.yon_z), Q | S | A, BRTV_CONV_FIXED_SCALAR, 0, VK_TM_PART },
    { BRT(YON_Z_F), F(matrix.yon_z), Q | S | A, BRTV_CONV_FLOAT_SCALAR, 0, VK_TM_PART },
};
#undef F

static vk_state_matrix default_state = {
    .model_to_view = { {
        BR_VECTOR3(1, 0, 0),
        BR_VECTOR3(0, 1, 0),
        BR_VECTOR3(0, 0, 1),
        BR_VECTOR3(0, 0, 0),
    } },
    .view_to_screen = { {
        BR_VECTOR4(1, 0, 0, 0),
        BR_VECTOR4(0, 1, 0, 0),
        BR_VECTOR4(0, 0, 1, 0),
        BR_VECTOR4(0, 0, 0, 1),
    } },
    .view_to_environment = { {
        BR_VECTOR3(1, 0, 0),
        BR_VECTOR3(0, 1, 0),
        BR_VECTOR3(0, 0, 1),
        BR_VECTOR3(0, 0, 0),
    } },
    .model_to_view_hint = BRT_LENGTH_PRESERVING,
    .view_to_screen_hint = BRT_PARALLEL,
};

void StateVkInitMatrix(vk_state_all* state) {
    state->templates.matrix = BrTVTemplateAllocate(state->res, template_entries, BR_ASIZE(template_entries));

    state->default_.matrix = default_state;
    state->default_.valid |= VK_MASK_STATE_MATRIX;
}
