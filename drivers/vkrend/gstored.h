#ifndef _GSTORED_H_
#define _GSTORED_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BR_GEOMETRY_STORED_PRIVATE

typedef struct br_geometry_stored {
    const struct br_geometry_stored_dispatch* dispatch;

    const char* identifier;

    struct br_device* device;

    struct br_geometry_v1_model* gv1model;

    br_boolean shared;
    struct v11model* model;

    br_uint_16 ngroups;
    br_uint_32 total_faces;
    br_uint_32 total_vertices;

} br_geometry_stored;

#endif

#ifdef __cplusplus
};
#endif
#endif
