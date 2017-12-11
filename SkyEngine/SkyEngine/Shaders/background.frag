#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D texColor;
layout(set = 1, binding = 0) uniform sampler2D blurMask;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 col = texture(texColor, fragUV);

    outColor = col;
}