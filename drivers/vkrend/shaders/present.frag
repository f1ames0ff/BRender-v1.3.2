#version 450

layout(set = 0, binding = 0) uniform sampler2D uOverlay;

#ifdef VKREND_OVERLAY_PASS

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 oColor;

void main() {
    vec3 overlay = texture(uOverlay, vUV).rgb;
    if (all(equal(overlay, vec3(1.0, 0.0, 1.0))))
        discard;
    oColor = vec4(overlay, 1.0);
}

#else

layout(set = 0, binding = 1) uniform sampler2D uScene;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 oColor;

void main() {
    vec3 overlay = texture(uOverlay, vUV).rgb;
    vec3 colour = all(equal(overlay, vec3(1.0, 0.0, 1.0))) ? texture(uScene, vUV).rgb : overlay;
    oColor = vec4(colour, 1.0);
}

#endif
