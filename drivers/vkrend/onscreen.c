#include "drv.h"
#include "shortcut.h"

#define MAKE_EQN(sign, row)                                                        \
    {                                                                              \
        eqn.v[X] = model_to_screen->m[0][3] sign model_to_screen->m[0][row];       \
        eqn.v[Y] = model_to_screen->m[1][3] sign model_to_screen->m[1][row];       \
        eqn.v[Z] = model_to_screen->m[2][3] sign model_to_screen->m[2][row];       \
        eqn.v[W] = -(model_to_screen->m[3][3] sign model_to_screen->m[3][row]);    \
    }

#define TEST_NOT_IN                                                                           \
    (BR_MAC3(eqn.v[X], ((eqn.v[X] > 0) ? (bounds->min.v[X]) : (bounds->max.v[X])), eqn.v[Y],  \
             ((eqn.v[Y] > 0) ? (bounds->min.v[Y]) : (bounds->max.v[Y])), eqn.v[Z],            \
             ((eqn.v[Z] > 0) ? (bounds->min.v[Z]) : (bounds->max.v[Z]))) < eqn.v[W])

#define TEST_OUT                                                                             \
    (BR_MAC3(eqn.v[X], ((eqn.v[X] > 0) ? (bounds->max.v[X]) : (bounds->min.v[X])), eqn.v[Y], \
             ((eqn.v[Y] > 0) ? (bounds->max.v[Y]) : (bounds->min.v[Y])), eqn.v[Z],           \
             ((eqn.v[Z] > 0) ? (bounds->max.v[Z]) : (bounds->min.v[Z]))) < eqn.v[W])

br_token VkOnScreenCheck(br_renderer* self, const br_matrix4* model_to_screen, const br_bounds3_f* bounds) {
    int accept = 1;
    br_vector4 eqn;
    int c;

    MAKE_EQN(+, 0);

    if (TEST_OUT)
        return BRT_REJECT;

    if (TEST_NOT_IN)
        accept = 0;

    MAKE_EQN(-, 0);

    if (TEST_OUT)
        return BRT_REJECT;

    if (accept && TEST_NOT_IN)
        accept = 0;

    MAKE_EQN(+, 1);

    if (TEST_OUT)
        return BRT_REJECT;

    if (accept && TEST_NOT_IN)
        accept = 0;

    MAKE_EQN(-, 1);

    if (TEST_OUT)
        return BRT_REJECT;

    if (accept && TEST_NOT_IN)
        accept = 0;

    MAKE_EQN(+, 2);

    if (TEST_OUT)
        return BRT_REJECT;

    if (accept && TEST_NOT_IN)
        accept = 0;

    MAKE_EQN(-, 2);

    if (TEST_OUT)
        return BRT_REJECT;

    if (accept && TEST_NOT_IN)
        accept = 0;

    for (c = 0; c < VK_MAX_STATE_CLIP_PLANES; c++) {
        if (self->state.current->clip[c].type != BRT_PLANE)
            continue;

        BrMatrix4TApply(&eqn, &self->state.current->clip[c].plane, (br_matrix4*)model_to_screen);
        eqn.v[W] = -eqn.v[W];

        if (TEST_OUT)
            return BRT_REJECT;

        if (accept && TEST_NOT_IN)
            accept = 0;
    }

    return accept ? BRT_ACCEPT : BRT_PARTIAL;
}
