#version 450

layout(set = 0, binding = 0, std140) uniform vk_model_state {
    mat4 v2s;
    mat4 m2v;
    vec4 colour;
    vec4 params;
    vec4 map_row0;
    vec4 map_row1;
} model;

layout(set = 0, binding = 1) uniform sampler2D tex0;

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColour;

layout(location = 0) out vec4 outColour;

void main() {
    vec2 mappedUV = vec2(dot(model.map_row0.xyz, vec3(vUV, 1.0)), dot(model.map_row1.xyz, vec3(vUV, 1.0)));
    vec4 tex = ((int(model.params.x) & 1) != 0) ? texture(tex0, mappedUV) : vec4(1.0);
    if (((int(model.params.x) & 1) != 0) && model.params.z != 0.0 && tex.rgb == vec3(0.0)) discard;
    vec3 surface = (model.params.w != 0.0) ? vColour.rgb : model.colour.rgb;
    outColour = vec4(surface * tex.rgb, model.params.y * tex.a);
}
