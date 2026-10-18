#version 450

struct DrawState {
    mat4 v2s;
    mat4 m2v;
    vec4 colour;
    vec4 params;
    vec4 map_row0;
    vec4 map_row1;
};

layout(set = 0, binding = 0, std430) buffer vk_model_state {
    DrawState draws[];
} data;

layout(push_constant) uniform PushConstants {
    uint draw_index;
} pc;

layout(set = 0, binding = 1) uniform sampler2D tex0[512];

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColour;

layout(location = 0) out vec4 outColour;

void main() {
    vec4 params = data.draws[pc.draw_index].params;
    vec4 colour = data.draws[pc.draw_index].colour;
    vec4 map_row0 = data.draws[pc.draw_index].map_row0;
    vec4 map_row1 = data.draws[pc.draw_index].map_row1;
    vec2 mappedUV = vec2(dot(map_row0.xyz, vec3(vUV, 1.0)), dot(map_row1.xyz, vec3(vUV, 1.0)));
    int texture_index = int(map_row0.w + 0.5);
    vec4 tex = ((int(params.x) & 1) != 0) ? texture(tex0[texture_index], mappedUV) : vec4(1.0);
    if (((int(params.x) & 1) != 0) && params.z != 0.0 && tex.rgb == vec3(0.0)) discard;
    vec3 surface = (params.w != 0.0) ? vColour.rgb : colour.rgb;
    outColour = vec4(surface * tex.rgb, params.y * tex.a);
}
