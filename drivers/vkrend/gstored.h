#ifndef _GSTORED_H_
#define _GSTORED_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VKREND_DRAW_RECORD_CAPACITY 4096

typedef struct VkrendDrawRecord {
    br_uint_32 sequence;

    struct br_geometry_stored* geometry;
    struct v11model* model;
    struct br_renderer* renderer;
    void* state;

    br_uint_16 group_index;
    br_uint_16 group_count;

    br_uint_32 vertices;
    br_uint_32 faces;
    br_uint_32 triangles;

} VkrendDrawRecord;

VkrendDrawRecord* GeometryStoredVkRecords(br_uint_32* count);

VkrendDrawRecord* GeometryStoredVkRecordGet(br_uint_32 index);

br_uint_32 GeometryStoredVkDrawCount(void);

br_uint_32 GeometryStoredVkTriangleCount(void);

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
