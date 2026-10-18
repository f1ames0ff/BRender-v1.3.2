#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColour;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColour;

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

void main() {
    vUV = aUV;
    vColour = aColour;
    gl_Position = data.draws[pc.draw_index].v2s * data.draws[pc.draw_index].m2v * vec4(aPosition, 1.0);
}
