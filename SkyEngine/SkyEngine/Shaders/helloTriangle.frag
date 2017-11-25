#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 col = texture(texColor, fragUV).xyz;
    vec3 N = normalize(fragNormal);
    col *= fragColor;

    float lighting = max(0.1, dot(N, vec3(0, 1, 0)));
    col *= lighting;

    col = 0.5 + 0.5 * N;

    outColor = vec4(col, 1.0);
}