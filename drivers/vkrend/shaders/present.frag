#version 450

layout(set = 0, binding = 0) uniform sampler2D uFrame;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 oColor;

void main() {
    oColor = vec4(texture(uFrame, vUV).rgb, 1.0);
}
